#pragma once

#include "duckdb.hpp"
#include "snowflake_config.hpp"

#include <arrow-adbc/adbc.h>
#include <arrow-adbc/adbc_driver_manager.h>

namespace duckdb {
namespace snowflake {

struct SnowflakeColumn {
	string name;
	string type; // The raw Snowflake type, e.g., "NUMBER(38,0)"
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

	vector<string> ListSchemas();
	vector<string> ListTables(ClientContext &context);
	vector<SnowflakeColumn> GetTableInfo(const string &schema, const string &table);

	private:
	SnowflakeConfig config;
	AdbcDatabase database;
	AdbcConnection connection;
	bool connected = false;
	
	unique_ptr<DataChunk> ExecuteAndGetChunk(ClientContext &context, const string &query, const vector<string> &expected_names, const vector<LogicalType> &expected_types);
	void InitializeDatabase(const SnowflakeConfig &config);
	void InitializeConnection();
	void CheckError(const AdbcStatusCode status, const std::string &operation, AdbcError *error);
};

} // namespace snowflake
} // namespace duckdb