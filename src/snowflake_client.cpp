#include "duckdb/common/exception.hpp"
#include "duckdb/function/table/arrow.hpp"

#include "snowflake_client.hpp"

#include <dlfcn.h>
#include <filesystem>

namespace duckdb {
namespace snowflake {

// Get the directory where the current extension is located
static std::string GetExtensionDirectory() {
	Dl_info info;
	// Use a function from this library to get its path
	if (dladdr(reinterpret_cast<void *>(&GetExtensionDirectory), &info)) {
		std::filesystem::path extension_path(info.dli_fname);
		std::string dir = extension_path.parent_path().string();

#ifdef DEBUG
		fprintf(stderr, "[DEBUG] GetExtensionDirectory: dli_fname = %s\n", info.dli_fname);
		fprintf(stderr, "[DEBUG] GetExtensionDirectory: parent_path = %s\n", dir.c_str());
#endif

		return dir;
	}
	// Fallback to current directory
	return ".";
}

SnowflakeClient::SnowflakeClient() {
	std::memset(&database, 0, sizeof(database));
	std::memset(&connection, 0, sizeof(connection));
}

SnowflakeClient::~SnowflakeClient() {
	Disconnect();
}

void SnowflakeClient::Connect(const SnowflakeConfig &config) {
	if (connected) {
		Disconnect();
	}

	this->config = config;
	InitializeDatabase(config);
	InitializeConnection();
	connected = true;
}

void SnowflakeClient::Disconnect() {
	if (!connected) {
		return;
	}

	AdbcError error;
	std::memset(&error, 0, sizeof(error));
	AdbcStatusCode status;

	status = AdbcConnectionRelease(&connection, &error);
	CheckError(status, "Failed to release ADBC connection", &error);

	status = AdbcDatabaseRelease(&database, &error);
	CheckError(status, "Failed to release ADBC database", &error);

	connected = false;
}

bool SnowflakeClient::IsConnected() const {
	return connected;
}

void SnowflakeClient::InitializeDatabase(const SnowflakeConfig &config) {
	AdbcError error;
	std::memset(&error, 0, sizeof(error));
	AdbcStatusCode status = AdbcDatabaseNew(&database, &error);
	CheckError(status, "Failed to create ADBC database", &error);

	// Use ADBC driver manager to load the Snowflake driver
	// Try to load from the same directory as the extension first
	std::string extension_dir = GetExtensionDirectory();
	std::filesystem::path adbc_path = std::filesystem::path(extension_dir) / SNOWFLAKE_ADBC_LIB;

	// Check if the library exists in the extension directory
	std::string driver_path;
	if (std::filesystem::exists(adbc_path)) {
		driver_path = adbc_path.string();
	} else {
		// Try just the filename - let the system search for it
		driver_path = SNOWFLAKE_ADBC_LIB;
	}

#ifdef DEBUG
	fprintf(stderr, "[DEBUG] Snowflake ADBC Driver Loading:\n");
	fprintf(stderr, "  Extension directory: %s\n", extension_dir.c_str());
	fprintf(stderr, "  Looking for driver at: %s\n", adbc_path.string().c_str());
	fprintf(stderr, "  Driver filename: %s\n", SNOWFLAKE_ADBC_LIB);
	fprintf(stderr, "  Final driver path: %s\n", driver_path.c_str());
#endif

	status = AdbcDatabaseSetOption(&database, "driver", driver_path.c_str(), &error);
	CheckError(status, "Failed to set Snowflake driver path", &error);

	// Set connection parameters
	status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.account", config.account.c_str(), &error);
	CheckError(status, "Failed to set account", &error);

	if (!config.username.empty()) {
		status = AdbcDatabaseSetOption(&database, "username", config.username.c_str(), &error);
		CheckError(status, "Failed to set username", &error);
	}

	// Set authentication based on type
	switch (config.auth_type) {
	case SnowflakeAuthType::PASSWORD:
		if (!config.password.empty()) {
			status = AdbcDatabaseSetOption(&database, "password", config.password.c_str(), &error);
			CheckError(status, "Failed to set password", &error);
		}
		break;
	case SnowflakeAuthType::OAUTH:
		if (!config.oauth_token.empty()) {
			status =
			    AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.auth_token", config.oauth_token.c_str(), &error);
			CheckError(status, "Failed to set OAuth token", &error);
		}
		break;
	case SnowflakeAuthType::KEY_PAIR:
		if (!config.private_key.empty()) {
			status =
			    AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.private_key", config.private_key.c_str(), &error);
			CheckError(status, "Failed to set private key", &error);
		}
		break;
	}

	// Set optional parameters
	if (!config.warehouse.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.warehouse", config.warehouse.c_str(), &error);
		CheckError(status, "Failed to set warehouse", &error);
	}

	if (!config.database.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.database", config.database.c_str(), &error);
		CheckError(status, "Failed to set database", &error);
	}

	// if (!config.schema.empty()) {
	// 	status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.schema", config.schema.c_str(), &error);
	// 	CheckError(status, "Failed to set schema", &error);
	// }

	if (!config.role.empty()) {
		status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.role", config.role.c_str(), &error);
		CheckError(status, "Failed to set role", &error);
	}

	// Set query timeout
	std::string timeout_str = std::to_string(config.query_timeout);
	status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.client_session_keep_alive",
	                               config.keep_alive ? "true" : "false", &error);
	CheckError(status, "Failed to set keep alive", &error);

	// Set high precision mode (when false, DECIMAL(p,0) converts to INT64)
	status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.client_option.use_high_precision",
	                               config.use_high_precision ? "true" : "false", &error);
	CheckError(status, "Failed to set high precision mode", &error);

	// Initialize the database
	status = AdbcDatabaseInit(&database, &error);
	CheckError(status, "Failed to initialize database", &error);
}

void SnowflakeClient::InitializeConnection() {
	AdbcError error;
	std::memset(&error, 0, sizeof(error));
	AdbcStatusCode status = AdbcConnectionNew(&connection, &error);
	CheckError(status, "Failed to create connection", &error);

	status = AdbcConnectionInit(&connection, &database, &error);
	CheckError(status, "Failed to initialize connection", &error);
}

void SnowflakeClient::CheckError(const AdbcStatusCode status, const std::string &operation, AdbcError *error) {
	if (status == ADBC_STATUS_OK) {
		return;
	}

	std::string error_message = operation + ": " + ((error && error->message) ? error->message : "Unknown ADBC error.");
	
	// Print to console for debugging
	fprintf(stderr, "[Snowflake Connection Error] %s\n", error_message.c_str());
	
	// Add more context based on common error patterns
	if (error && error->message) {
		std::string msg(error->message);
		if (msg.find("authentication") != std::string::npos || msg.find("Authentication") != std::string::npos) {
			fprintf(stderr, "  → Check your username and password\n");
		} else if (msg.find("account") != std::string::npos || msg.find("Account") != std::string::npos) {
			fprintf(stderr, "  → Check your account identifier format (e.g., 'account-name.region')\n");
		} else if (msg.find("warehouse") != std::string::npos || msg.find("Warehouse") != std::string::npos) {
			fprintf(stderr, "  → Check your warehouse name and permissions\n");
		} else if (msg.find("database") != std::string::npos || msg.find("Database") != std::string::npos) {
			fprintf(stderr, "  → Check your database name and permissions\n");
		} else if (msg.find("network") != std::string::npos || msg.find("Network") != std::string::npos ||
		           msg.find("connection") != std::string::npos || msg.find("Connection") != std::string::npos) {
			fprintf(stderr, "  → Check your network connectivity and firewall settings\n");
		}
	}

	// Only release if the error has a release function and hasn't been released already
	if (error && error->release && error->message) {
		error->release(error);
	}

	throw IOException(error_message);
}

} // namespace snowflake
} // namespace duckdb
