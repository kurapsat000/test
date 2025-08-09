#include "snowflake_scan.hpp"
#include "storage/snowflake_table_entry.hpp"

#include "duckdb/function/table_function.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/function/table/arrow.hpp"
#include "snowflake_client_manager.hpp"
#include "snowflake_config.hpp"
#include <arrow-adbc/adbc.h>

namespace duckdb {
namespace snowflake {

static unique_ptr<FunctionData> SnowflakeScanBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
	// Validate parameters
	if (input.inputs.size() < 2) {
		throw BinderException("snowflake_scan requires at least 2 parameters: connection_string and query");
	}

	// Get connection string and query
	auto connection_string = input.inputs[0].GetValue<string>();
	auto query = input.inputs[1].GetValue<string>();

	// Parse connection and establish connection
	auto &client_manager = SnowflakeClientManager::GetInstance();
	auto config = SnowflakeConfig::ParseConnectionString(connection_string);
	auto connection = client_manager.GetConnection(connection_string, config);

	// Create the factory that will manage the ADBC connection and statement
	// This factory will be kept alive throughout the scan operation
	auto factory = make_uniq<SnowflakeArrowStreamFactory>(connection, query);

	// Create the bind data that inherits from ArrowScanFunctionData
	// This allows us to use DuckDB's native Arrow scan implementation
	auto bind_data = make_uniq<SnowflakeScanBindData>(std::move(factory));
	bind_data->connection_string = connection_string;
	bind_data->query = query;

	// Get the schema from Snowflake using ADBC's ExecuteSchema
	// This executes the query with schema-only mode to get column information
	SnowflakeGetArrowSchema(reinterpret_cast<ArrowArrayStream *>(bind_data->factory.get()),
	                        bind_data->schema_root.arrow_schema);

	// Use DuckDB's Arrow integration to populate the table type information
	// This converts Arrow schema to DuckDB types and handles all type mappings
	ArrowTableFunction::PopulateArrowTableType(DBConfig::GetConfig(context), bind_data->arrow_table,
	                                           bind_data->schema_root, names, return_types);
	bind_data->all_types = return_types;

	return std::move(bind_data);
}

} // namespace snowflake

TableFunction GetSnowflakeScanFunction() {
	// Create a table function that uses DuckDB's native Arrow scan implementation
	// We only provide our own bind function to set up the Snowflake connection
	// All other operations (init_global, init_local, scan) use DuckDB's implementation
	TableFunction snowflake_scan("snowflake_scan", {LogicalType::VARCHAR, LogicalType::VARCHAR},
	                             ArrowTableFunction::ArrowScanFunction,   // Use DuckDB's scan
	                             snowflake::SnowflakeScanBind,            // Our bind function
	                             ArrowTableFunction::ArrowScanInitGlobal, // Use DuckDB's init
	                             ArrowTableFunction::ArrowScanInitLocal); // Use DuckDB's init

	// Enable projection and filter pushdown for optimization
	snowflake_scan.projection_pushdown = true;
	snowflake_scan.filter_pushdown = true;

	return snowflake_scan;
}

} // namespace duckdb