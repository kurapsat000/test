#include "snowflake_scan.hpp"
#include "storage/snowflake_table_entry.hpp"

#include "duckdb/function/table_function.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/function/table/arrow.hpp"
#include "snowflake_client_manager.hpp"
#include "snowflake_arrow_utils.hpp"
#include "snowflake_config.hpp"
#include "snowflake_secrets.hpp"
#include <arrow-adbc/adbc.h>

namespace duckdb {

struct SnowflakeScanBindData : public TableFunctionData {
	std::string connection_string;
	std::string query;
	vector<string> column_names;
	vector<LogicalType> column_types;
	std::shared_ptr<SnowflakeConnection> connection;
	arrow_column_map_t arrow_convert_data;
	ArrowSchemaWrapper schema_wrapper;

	SnowflakeScanBindData() {
	}
};

struct SnowflakeScanGlobalState : public GlobalTableFunctionState {
	std::unique_ptr<ArrowStreamWrapper> stream;
	unique_ptr<ArrowArrayWrapper> current_chunk;
	idx_t batch_index = 0;
	bool finished = false;
	AdbcStatement statement;
	bool statement_initialized = false;

	SnowflakeScanGlobalState() {
		std::memset(&statement, 0, sizeof(statement));
	}

	~SnowflakeScanGlobalState() {
		if (statement_initialized) {
			AdbcError error;
			AdbcStatementRelease(&statement, &error);
		}
	}

	idx_t MaxThreads() const override {
		return 1; // Single-threaded for now
	}
};

struct SnowflakeScanLocalState : public ArrowScanLocalState {
	SnowflakeScanLocalState(unique_ptr<ArrowArrayWrapper> current_chunk, ClientContext &context)
	    : ArrowScanLocalState(std::move(current_chunk), context) {
	}
};

static unique_ptr<FunctionData> SnowflakeScanBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
	auto bind_data = make_uniq<SnowflakeScanBindData>();

	// Validate parameters
	if (input.inputs.size() < 2) {
		throw BinderException("snowflake_scan requires at least 2 parameters: connection_string and query");
	}

	// Get connection string and query
	bind_data->connection_string = input.inputs[0].GetValue<string>();
	bind_data->query = input.inputs[1].GetValue<string>();

	// Parse connection and establish connection to get schema
	try {
		bind_data->connection = SnowflakeConnectionManager::GetInstance().GetConnection(bind_data->connection_string);

		// Prepare statement to get schema
		AdbcStatement statement;
		AdbcError error;
		std::memset(&statement, 0, sizeof(statement));
		std::memset(&error, 0, sizeof(error));

		AdbcStatusCode status = AdbcStatementNew(bind_data->connection->GetConnection(), &statement, &error);
		if (status != ADBC_STATUS_OK) {
			throw IOException("Failed to create statement");
		}

		// Set the query
		status = AdbcStatementSetSqlQuery(&statement, bind_data->query.c_str(), &error);
		if (status != ADBC_STATUS_OK) {
			AdbcStatementRelease(&statement, &error);
			throw IOException("Failed to set query");
		}

		// Execute with schema only to get column information
		ArrowSchema schema;
		std::memset(&schema, 0, sizeof(schema));
		status = AdbcStatementExecuteSchema(&statement, &schema, &error);
		if (status != ADBC_STATUS_OK) {
			AdbcStatementRelease(&statement, &error);
			std::string error_msg = "Failed to get schema: ";
			if (error.message) {
				error_msg += error.message;
				if (error.release) {
					error.release(&error);
				}
			}
			throw IOException(error_msg);
		}

		// Store the Arrow schema in the bind data
		bind_data->schema_wrapper.arrow_schema = schema;

		// Populate the arrow schema using ArrowTableFunction
		ArrowTableType arrow_table;
		ArrowTableFunction::PopulateArrowTableType(context.db->config, arrow_table, bind_data->schema_wrapper, names,
		                                           return_types);

		// Store column names and types
		bind_data->column_names = names;
		bind_data->column_types = return_types;

		// Create arrow conversion data
		bind_data->arrow_convert_data = arrow_table.GetColumns();

		AdbcStatementRelease(&statement, &error);

	} catch (const std::exception &e) {
		throw BinderException("Failed to bind snowflake_scan: %s", e.what());
	}

	return std::move(bind_data);
}

} // namespace snowflake

TableFunction GetSnowflakeScanFunction() {
	TableFunction snowflake_scan("snowflake_scan", {LogicalType::VARCHAR, LogicalType::VARCHAR}, SnowflakeScanExecute,
	                             SnowflakeScanBind, SnowflakeScanInitGlobal, SnowflakeScanInitLocal);

	snowflake_scan.cardinality = SnowflakeScanCardinality;
	snowflake_scan.projection_pushdown = false; // TODO: Implement in Phase 3
	snowflake_scan.filter_pushdown = false;     // TODO: Implement in Phase 3

	return snowflake_scan;
}

} // namespace duckdb