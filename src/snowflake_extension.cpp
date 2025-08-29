#define DUCKDB_EXTENSION_MAIN

#include "snowflake_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

// ADBC headers (when available)
#ifdef ADBC_AVAILABLE
#include <adbc.h>
#include <adbc/driver_manager.h>
#endif

// ADBC driver path
#define ADBC_DRIVER_PATH "build/adbc/snowflake_driver.so"

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

inline void SnowflakeADBCVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		return StringVector::AddString(result,
		                               "Snowflake " + name.GetString() + " - " + SnowflakeExtension::GetADBCVersion());
	});
}

static void LoadInternal(DuckDB &db) {
	// Get the extension loader from the database
	auto &loader = db.GetExtensionLoader();

	// Register a scalar function
	auto snowflake_scalar_function =
	    ScalarFunction("snowflake", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SnowflakeScalarFun);
	loader.RegisterFunction(snowflake_scalar_function);

	// Register another scalar function
	auto snowflake_openssl_version_scalar_function = ScalarFunction(
	    "snowflake_openssl_version", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SnowflakeOpenSSLVersionScalarFun);
	loader.RegisterFunction(snowflake_openssl_version_scalar_function);

	// Register ADBC version function
	auto snowflake_adbc_version_scalar_function = ScalarFunction("snowflake_adbc_version", {LogicalType::VARCHAR},
	                                                             LogicalType::VARCHAR, SnowflakeADBCVersionScalarFun);
	loader.RegisterFunction(snowflake_adbc_version_scalar_function);

	// Initialize ADBC if available
	SnowflakeExtension::InitializeADBC();
}

void SnowflakeExtension::Load(DuckDB &db) {
	LoadInternal(db);
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
	// For now, we'll just check if the driver file exists
	FILE *file = fopen(ADBC_DRIVER_PATH, "r");
	if (file) {
		fclose(file);
		return true;
	}
	return false;
#else
	// ADBC not available, but extension can still function with basic features
	return false;
#endif
}

std::string SnowflakeExtension::GetADBCVersion() {
#ifdef ADBC_AVAILABLE
	FILE *file = fopen(ADBC_DRIVER_PATH, "r");
	if (file) {
		fclose(file);
		return "ADBC Available - Driver Found";
	}
	return "ADBC Available - Driver Not Found";
#else
	return "ADBC Not Available";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(snowflake, loader) {
	duckdb::LoadInternal(loader);
}

DUCKDB_EXTENSION_API const char *snowflake_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
