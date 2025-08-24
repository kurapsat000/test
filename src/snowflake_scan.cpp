#include "snowflake_scan.hpp"
#include "duckdb.hpp"
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
#include "snowflake_debug.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<FunctionData> SnowflakeScanBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
	DPRINT("SnowflakeScanBind invoked\n");

	SnowflakeConfig config;
	string query;

	// Handle different parameter patterns
	if (input.inputs.size() == 2) {
		// Pattern 1: snowflake_scan(connection_string, query)
		// Pattern 2: snowflake_scan(query, profile)
		auto param1 = input.inputs[0].GetValue<string>();
		auto param2 = input.inputs[1].GetValue<string>();

		// Check if param1 looks like a connection string (contains 'account=' or 'user=')
		if (param1.find("account=") != string::npos || param1.find("user=") != string::npos) {
			// Pattern 1: (connection_string, query)
			config = SnowflakeConfig::ParseConnectionString(param1);
			query = param2;
		} else {
			// Pattern 2: (query, profile)
			query = param1;
			try {
				config = SnowflakeSecretsHelper::GetCredentials(context, param2);
			} catch (const std::exception &e) {
				throw BinderException("Failed to retrieve credentials for profile '%s': %s", param2.c_str(), e.what());
			}
		}
	} else {
		throw BinderException("snowflake_scan requires exactly 2 parameters: (connection_string, query) or (query, profile)");
	}

	// Get client manager
	auto &client_manager = SnowflakeClientManager::GetInstance();

	shared_ptr<SnowflakeClient> connection;
	try {
		connection = client_manager.GetConnection(config);
	} catch (const std::exception &e) {
		throw BinderException("Failed to initialize connection: %s", e.what());
	}

	// Create the factory that will manage the ADBC connection and statement
	// This factory will be kept alive throughout the scan operation
	auto factory = make_uniq<SnowflakeArrowStreamFactory>(connection, query);

	// Create the bind data that inherits from ArrowScanFunctionData
	// This allows us to use DuckDB's native Arrow scan implementation
	auto bind_data = make_uniq<SnowflakeScanBindData>(std::move(factory));
	// TODO remove below line after implementing projection pushdown
	bind_data->projection_pushdown_enabled = false;

	// Get the schema from Snowflake using ADBC's ExecuteSchema
	// This executes the query with schema-only mode to get column information
	SnowflakeGetArrowSchema(reinterpret_cast<ArrowArrayStream *>(bind_data->factory.get()),
	                        bind_data->schema_root.arrow_schema);

	// Use DuckDB's Arrow integration to populate the table type information
	// This converts Arrow schema to DuckDB types and handles all type mappings
	ArrowTableFunction::PopulateArrowTableType(DBConfig::GetConfig(context), bind_data->arrow_table,
	                                           bind_data->schema_root, names, return_types);
	bind_data->all_types = return_types;

	DPRINT("SnowflakeScanBind returning bind data\n");
	return std::move(bind_data);
}

} // namespace snowflake

TableFunction GetSnowflakeScanFunction() {
	// Create a table function that uses DuckDB's native Arrow scan implementation
	// We only provide our own bind function to set up the Snowflake connection
	// All other operations (init_global, init_local, scan) use DuckDB's implementation
	// Parameters: (connection_string, query) or (query, profile)
	TableFunction snowflake_scan("snowflake_scan", {LogicalType::VARCHAR, LogicalType::VARCHAR},
	                             ArrowTableFunction::ArrowScanFunction,   // Use DuckDB's scan
	                             snowflake::SnowflakeScanBind,            // Our bind function
	                             ArrowTableFunction::ArrowScanInitGlobal, // Use DuckDB's init
	                             ArrowTableFunction::ArrowScanInitLocal); // Use DuckDB's init

	// TODO Enable projection and filter pushdown for optimization
	snowflake_scan.projection_pushdown = false;
	snowflake_scan.filter_pushdown = false;

	return snowflake_scan;
}

} // namespace duckdb
