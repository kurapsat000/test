#include "snowflake_connection.hpp"
#include "duckdb/common/exception.hpp"
#include <sstream>
#include <regex>

namespace duckdb {
namespace snowflake {
SnowflakeConfig SnowflakeConfig::ParseConnectionString(const std::string &connection_string) {
	SnowflakeConfig config;

	// Parse key=value pairs separated by semicolons
	std::regex param_regex("([^=;]+)=([^;]*)");
	auto params_begin = std::sregex_iterator(connection_string.begin(), connection_string.end(), param_regex);
	auto params_end = std::sregex_iterator();

	for (auto it = params_begin; it != params_end; ++it) {
		std::string key = (*it)[1].str();
		std::string value = (*it)[2].str();

		if (key == "account") {
			config.account = value;
		} else if (key == "username" || key == "user") {
			config.username = value;
		} else if (key == "password") {
			config.password = value;
		} else if (key == "warehouse") {
			config.warehouse = value;
		} else if (key == "database") {
			config.database = value;
		} else if (key == "schema") {
			config.schema = value;
		} else if (key == "role") {
			config.role = value;
		} else if (key == "auth_type") {
			if (value == "password") {
				config.auth_type = SnowflakeAuthType::PASSWORD;
			} else if (value == "oauth") {
				config.auth_type = SnowflakeAuthType::OAUTH;
			} else if (value == "key_pair") {
				config.auth_type = SnowflakeAuthType::KEY_PAIR;
			}
		} else if (key == "token") {
			config.oauth_token = value;
		} else if (key == "private_key") {
			config.private_key = value;
		} else if (key == "query_timeout") {
			config.query_timeout = std::stoi(value);
		} else if (key == "keep_alive") {
			config.keep_alive = (value == "true" || value == "1");
		}
	}

	// Validate required fields
	if (config.account.empty()) {
		throw InvalidInputException("Snowflake connection string missing required 'account' parameter");
	}

	return config;
}

SnowflakeConnection::SnowflakeConnection() {
	std::memset(&database, 0, sizeof(database));
	std::memset(&connection, 0, sizeof(connection));
	std::memset(&error, 0, sizeof(error));
}

SnowflakeConnection::~SnowflakeConnection() {
	Disconnect();
}

void SnowflakeConnection::Connect(const SnowflakeConfig &config) {
	if (connected) {
		Disconnect();
	}

	InitializeDatabase(config);
	InitializeConnection();
	connected = true;
}

void SnowflakeConnection::Disconnect() {
	if (!connected) {
		return;
	}

	AdbcConnectionRelease(&connection, &error);
	AdbcDatabaseRelease(&database, &error);
	connected = false;
}

bool SnowflakeConnection::IsConnected() const {
	return connected;
}

void SnowflakeConnection::InitializeDatabase(const SnowflakeConfig &config) {
	AdbcStatusCode status = AdbcDatabaseNew(&database, &error);
	CheckError(status, "Failed to create ADBC database");

	// Use ADBC driver manager to load the Snowflake driver 
	// Load the specific driver path at runtime using the macro from CMakeLists.txt
	status = AdbcDatabaseSetOption(&database, "driver", SNOWFLAKE_ADBC_LIB_PATH, &error);
	CheckError(status, "Failed to set Snowflake driver path");

	// Set connection parameters
	status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.account", config.account.c_str(), &error);
	CheckError(status, "Failed to set account");

	if (!config.username.empty()) {
		status = AdbcDatabaseSetOption(&database, "username", config.username.c_str(), &error);
		CheckError(status, "Failed to set username");
	}

	// Set authentication based on type
	switch (config.auth_type) {
	case SnowflakeAuthType::PASSWORD:
		if (!config.password.empty()) {
			status = AdbcDatabaseSetOption(&database, "password", config.password.c_str(), &error);
			CheckError(status, "Failed to set password");
		}
		break;
	case SnowflakeAuthType::OAUTH:
		if (!config.oauth_token.empty()) {
			status =
			    AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.auth_token", config.oauth_token.c_str(), &error);
			CheckError(status, "Failed to set OAuth token");
		}
		break;
	case SnowflakeAuthType::KEY_PAIR:
		if (!config.private_key.empty()) {
			status =
			    AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.private_key", config.private_key.c_str(), &error);
			CheckError(status, "Failed to set private key");
		}
		break;
	}

	// Set optional parameters
	if (!config.warehouse.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.warehouse", config.warehouse.c_str(), &error);
		CheckError(status, "Failed to set warehouse");
	}

	if (!config.database.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.database", config.database.c_str(), &error);
		CheckError(status, "Failed to set database");
	}

	if (!config.schema.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.schema", config.schema.c_str(), &error);
		CheckError(status, "Failed to set schema");
	}

	if (!config.role.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.role", config.role.c_str(), &error);
		CheckError(status, "Failed to set role");
	}

	// Set query timeout
	std::string timeout_str = std::to_string(config.query_timeout);
	status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.client_session_keep_alive",
	                               config.keep_alive ? "true" : "false", &error);
	CheckError(status, "Failed to set keep alive");

	// Initialize the database
	status = AdbcDatabaseInit(&database, &error);
	CheckError(status, "Failed to initialize database");
}

void SnowflakeConnection::InitializeConnection() {
	AdbcStatusCode status = AdbcConnectionNew(&connection, &error);
	CheckError(status, "Failed to create connection");

	status = AdbcConnectionInit(&connection, &database, &error);
	CheckError(status, "Failed to initialize connection");
}

void SnowflakeConnection::CheckError(AdbcStatusCode status, const std::string &operation) {
	if (status != ADBC_STATUS_OK) {
		last_error = operation + ": ";
		if (error.message) {
			last_error += error.message;
		} else {
			last_error += "Unknown error";
		}

		// Clean up error
		if (error.release) {
			error.release(&error);
		}

		throw IOException(last_error);
	}
}

SnowflakeConnectionManager &SnowflakeConnectionManager::GetInstance() {
	static SnowflakeConnectionManager instance;
	return instance;
}

std::shared_ptr<SnowflakeConnection> SnowflakeConnectionManager::GetConnection(const std::string &connection_string) {
	std::lock_guard<std::mutex> lock(connection_mutex);

	auto it = connections.find(connection_string);
	if (it != connections.end() && it->second->IsConnected()) {
		return it->second;
	}

	// Create new connection
	auto config = SnowflakeConfig::ParseConnectionString(connection_string);
	auto connection = std::make_shared<SnowflakeConnection>();
	connection->Connect(config);

	connections[connection_string] = connection;
	return connection;
}

void SnowflakeConnectionManager::ReleaseConnection(const std::string &connection_string) {
	std::lock_guard<std::mutex> lock(connection_mutex);
	connections.erase(connection_string);
}

} // namespace snowflake
} // namespace duckdb