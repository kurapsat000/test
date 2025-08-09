// #pragma once

// #include "duckdb.hpp"

// namespace duckdb {
// namespace snowflake {

// TableFunction GetSnowflakeCatalogScanFunction();

// //! Scan function to be used internally in the catalog, gets connection and query strings from bind data rather than
// //! input parameters
// unique_ptr<FunctionData> SnowflakeCatalogScanBind(ClientContext &context, TableFunctionBindInput &input,
//                                                   vector<LogicalType> &return_types, vector<string> &names);

// } // namespace snowflake
// } // namespace duckdb