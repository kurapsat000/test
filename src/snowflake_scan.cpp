#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/function/table/arrow.hpp"
#include "duckdb/function/table/arrow/arrow_duck_schema.hpp"
#include "snowflake_connection.hpp"
#include "snowflake_arrow_utils.hpp"
#include <arrow-adbc/adbc.h>

namespace duckdb {
namespace snowflake {
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

static unique_ptr<GlobalTableFunctionState> SnowflakeScanInitGlobal(ClientContext &context,
                                                                    TableFunctionInitInput &input) {
	auto &bind_data = (SnowflakeScanBindData &)*input.bind_data;
	auto global_state = make_uniq<SnowflakeScanGlobalState>();

	try {
		// Initialize statement
		AdbcError error;
		std::memset(&error, 0, sizeof(error));

		AdbcStatusCode status =
		    AdbcStatementNew(bind_data.connection->GetConnection(), &global_state->statement, &error);
		if (status != ADBC_STATUS_OK) {
			throw IOException("Failed to create statement");
		}
		global_state->statement_initialized = true;

		// Set the query
		status = AdbcStatementSetSqlQuery(&global_state->statement, bind_data.query.c_str(), &error);
		if (status != ADBC_STATUS_OK) {
			std::string error_msg = "Failed to set query: ";
			if (error.message) {
				error_msg += error.message;
				if (error.release) {
					error.release(&error);
				}
			}
			throw IOException(error_msg);
		}

		// Initialize Arrow stream
		global_state->stream = make_uniq<ArrowStreamWrapper>();
		global_state->stream->InitializeFromADBC(&global_state->statement);

	} catch (const std::exception &e) {
		throw IOException("Failed to initialize Snowflake scan: %s", e.what());
	}

	return std::move(global_state);
}

static unique_ptr<LocalTableFunctionState> SnowflakeScanInitLocal(ExecutionContext &context,
                                                                  TableFunctionInitInput &input,
                                                                  GlobalTableFunctionState *global_state) {
	auto &gstate = (SnowflakeScanGlobalState &)*global_state;

	// Create an empty ArrowArrayWrapper for the local state initialization
	auto chunk = make_uniq<ArrowArrayWrapper>();
	return make_uniq<SnowflakeScanLocalState>(std::move(chunk), context.client);
}

static void SnowflakeScanExecute(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = (SnowflakeScanBindData &)*data.bind_data;
	auto &global_state = (SnowflakeScanGlobalState &)*data.global_state;
	auto &local_state = (SnowflakeScanLocalState &)*data.local_state;

	if (global_state.finished) {
		output.SetCardinality(0);
		return;
	}

	// Check if we need to get a new chunk
	if (!local_state.chunk || local_state.chunk_offset >= (idx_t)local_state.chunk->arrow_array.length) {
		// Get next chunk from stream
		global_state.current_chunk = global_state.stream->GetNextChunk();

		if (!global_state.current_chunk) {
			global_state.finished = true;
			output.SetCardinality(0);
			return;
		}

		// Update local state with new chunk
		local_state.chunk = std::move(global_state.current_chunk);
		local_state.chunk_offset = 0;
		local_state.batch_index = global_state.batch_index++;
	}

	// Initialize column IDs if not done yet
	if (local_state.column_ids.empty()) {
		for (idx_t i = 0; i < bind_data.column_types.size(); i++) {
			local_state.column_ids.push_back(i);
		}
	}

	// Determine how many rows to read
	idx_t remaining_in_chunk = local_state.chunk->arrow_array.length - local_state.chunk_offset;
	idx_t to_scan = MinValue<idx_t>(remaining_in_chunk, STANDARD_VECTOR_SIZE);

	if (to_scan == 0) {
		output.SetCardinality(0);
		return;
	}

	// Use DuckDB's native Arrow to DuckDB conversion
	ArrowTableFunction::ArrowToDuckDB(local_state, bind_data.arrow_convert_data, output, local_state.chunk_offset,
	                                  true);

	// Set the output cardinality to the number of rows we just processed
	output.SetCardinality(to_scan);
	local_state.chunk_offset += to_scan;
}

static unique_ptr<NodeStatistics> SnowflakeScanCardinality(ClientContext &context, const FunctionData *bind_data) {
	// Cannot determine cardinality ahead of time
	return nullptr;
}

} // namespace snowflake

TableFunction GetSnowflakeScanFunction() {
	TableFunction snowflake_scan("snowflake_scan", {LogicalType(LogicalTypeId::VARCHAR), LogicalType(LogicalTypeId::VARCHAR)}, 
	                             snowflake::SnowflakeScanExecute,
	                             snowflake::SnowflakeScanBind, 
	                             snowflake::SnowflakeScanInitGlobal, 
	                             snowflake::SnowflakeScanInitLocal);

	snowflake_scan.cardinality = snowflake::SnowflakeScanCardinality;
	snowflake_scan.projection_pushdown = false; // TODO: Implement in Phase 3
	snowflake_scan.filter_pushdown = false;     // TODO: Implement in Phase 3

	return snowflake_scan;
}

} // namespace duckdb