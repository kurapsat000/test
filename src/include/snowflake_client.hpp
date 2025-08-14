#pragma once

#ifdef DEBUG_SNOWFLAKE
#define DPRINT(...) fprintf(stderr, "[DEBUG] " __VA_ARGS__)
#else
#define DPRINT(...) ((void)0)
#endif

#include "duckdb.hpp"
#include "snowflake_config.hpp"

#include <arrow-adbc/adbc.h>
#include <arrow-adbc/adbc_driver_manager.h>

namespace duckdb {
namespace snowflake {
struct SnowflakeColumn {
	string name;
	LogicalType type;
	bool is_nullable;
};

class SnowflakeClient {
public:
	SnowflakeClient();
	~SnowflakeClient();

	void Connect(const SnowflakeConfig &config);
	void Disconnect();
	bool IsConnected() const;

	AdbcConnection *GetConnection() {
		return &connection;
	}
	AdbcDatabase *GetDatabase() {
		return &database;
	}
	const SnowflakeConfig &GetConfig() const;

	vector<string> ListSchemas(ClientContext &context);
	vector<string> ListTables(ClientContext &context, const string &schema);
	vector<SnowflakeColumn> GetTableInfo(ClientContext &context, const string &schema, const string &table_name);

private:
	SnowflakeConfig config;
	AdbcDatabase database;
	AdbcConnection connection;
	bool connected = false;

	vector<vector<string>> ExecuteAndGetStrings(ClientContext &context, const string &query,
	                                            const vector<string> &expected_col_names);
	void InitializeDatabase(const SnowflakeConfig &config);
	void InitializeConnection();
	void CheckError(const AdbcStatusCode status, const std::string &operation, AdbcError *error);
};

} // namespace snowflake
} // namespace duckdb
