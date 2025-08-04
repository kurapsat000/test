#pragma once

#include "duckdb.hpp"
#include "snowflake_config.hpp"

#include <arrow-adbc/adbc.h>
#include <arrow-adbc/adbc_driver_manager.h>

namespace duckdb {
namespace snowflake {

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


private:
	SnowflakeConfig config;
	AdbcDatabase database;
	AdbcConnection connection;
	bool connected = false;

	void InitializeDatabase(const SnowflakeConfig &config);
	void InitializeConnection();
	void CheckError(const AdbcStatusCode status, const std::string &operation, AdbcError *error);
};

} // namespace snowflake
} // namespace duckdb
