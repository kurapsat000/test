#include "duckdb/common/helper.hpp"
#define DUCKDB_EXTENSION_MAIN

#include "snowflake_extension.hpp"
// #include "snowflake_attach.hpp"
#include "storage/snowflake_storage.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include "duckdb/function/table_function.hpp"

namespace duckdb {

// Forward declaration
TableFunction GetSnowflakeScanFunction();

inline void SnowflakeVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	result.SetVectorType(VectorType::CONSTANT_VECTOR);
	auto val = Value("Snowflake Extension v0.1.0");
	result.SetValue(0, val);
}

static void LoadInternal(DatabaseInstance &instance) {
	auto snowflake_version_function =
	    ScalarFunction("snowflake_version", {}, LogicalType::VARCHAR, SnowflakeVersionScalarFun);
	ExtensionUtil::RegisterFunction(instance, snowflake_version_function);

	auto snowflake_scan_function = GetSnowflakeScanFunction();
	ExtensionUtil::RegisterFunction(instance, snowflake_scan_function);

	// duckdb::snowflake::SnowflakeAttachFunction snowflake_attach_function;
	// ExtensionUtil::RegisterFunction(instance, snowflake_attach_function);

	auto &config = DBConfig::GetConfig(instance);
	config.storage_extensions["snowflake"] = make_uniq<snowflake::SnowflakeStorageExtension>();
}

void SnowflakeExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string SnowflakeExtension::Name() {
	return "snowflake";
}

std::string SnowflakeExtension::Version() const {
#ifdef EXT_VERSION_SNOWFLAKE
	return EXT_VERSION_SNOWFLAKE;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void snowflake_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::SnowflakeExtension>();
}

DUCKDB_EXTENSION_API const char *snowflake_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
