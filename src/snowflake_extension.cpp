#define DUCKDB_EXTENSION_MAIN

#include "snowflake_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

// ADBC headers (when available)
#ifdef ADBC_AVAILABLE
#include <adbc.h>
#endif

namespace duckdb {

inline void SnowflakeScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		return StringVector::AddString(result, "Snowflake " + name.GetString() + " 🐥");
	});
}

inline void SnowflakeOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		return StringVector::AddString(result, "Snowflake " + name.GetString() + ", my linked OpenSSL version is " +
		                                           OPENSSL_VERSION_TEXT);
	});
}

static void LoadInternal(DatabaseInstance &instance) {
	// Register a scalar function
	auto snowflake_scalar_function = ScalarFunction("snowflake", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SnowflakeScalarFun);
	ExtensionUtil::RegisterFunction(instance, snowflake_scalar_function);

	// Register another scalar function
	auto snowflake_openssl_version_scalar_function = ScalarFunction("snowflake_openssl_version", {LogicalType::VARCHAR},
	                                                            LogicalType::VARCHAR, SnowflakeOpenSSLVersionScalarFun);
	ExtensionUtil::RegisterFunction(instance, snowflake_openssl_version_scalar_function);

	// Initialize ADBC if available
	SnowflakeExtension::InitializeADBC();
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

// ADBC integration implementation
bool SnowflakeExtension::InitializeADBC() {
#ifdef ADBC_AVAILABLE
	// Initialize ADBC driver manager
	// This would typically involve loading the ADBC driver and setting up connections
	return true;
#else
	// ADBC not available, but extension can still function with basic features
	return false;
#endif
}

std::string SnowflakeExtension::GetADBCVersion() {
#ifdef ADBC_AVAILABLE
	return "ADBC Available";
#else
	return "ADBC Not Available";
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
