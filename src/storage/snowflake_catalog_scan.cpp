// #include "storage/snowflake_catalog_scan.hpp"
// #include "storage/snowflake_table_entry.hpp"
// #include "duckdb/function/table/arrow.hpp"

// namespace duckdb {
// namespace snowflake {

// unique_ptr<FunctionData> SnowflakeCatalogScanBind(ClientContext &context, TableFunctionBindInput &input,
//                                                   vector<LogicalType> &return_types, vector<string> &names) {
// 	// First check if bind_data exists
// 	if (!input.bind_data) {
// 		throw InternalException("No bind data provided to SnowflakeCatalogScanBind");
// 	}

// 	// Use Cast method correctly
// 	auto &table_bind_data = input.bind_data->Cast<SnowflakeTableBindData>();

// 	// Now use the data
// 	return_types = table_bind_data.column_types;
// 	names = table_bind_data.column_names;

// 	// Return a copy of the bind data
// 	return input.bind_data->Copy();
// }

// TableFunction GetSnowflakeCatalogScanFunction() {
// 	TableFunction scan("snowflake_catalog_scan", {}, ArrowTableFunction::ArrowScanFunction, SnowflakeCatalogScanBind,
// 	                   ArrowTableFunction::ArrowScanInitGlobal, ArrowTableFunction::ArrowScanInitLocal);

// 	scan.projection_pushdown = true;
// 	scan.filter_pushdown = true;
// 	scan.get_bind_info = true;

// 	return scan;
// }

// } // namespace snowflake
// } // namespace duckdb