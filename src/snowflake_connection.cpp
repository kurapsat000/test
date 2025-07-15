#include "snowflake_connection.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/table/arrow.hpp"
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

	this->config = config;
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

vector<string> SnowflakeConnection::ListTables(ClientContext &context) {
	const string schema_query = "SELECT table_name FROM " + config.database +
	                            ".information_schema.tables WHERE table_schema = '" + config.schema + "'";

	if (!connected) {
		throw IOException("Connection must be created before ListTables is called");
	}

	AdbcStatement statement;
	AdbcStatusCode result = AdbcStatementNew(GetConnection(), &statement, &error);
	CheckError(result, string("Failed to create AdbcStatement") + error.message);

	CheckError(AdbcStatementSetSqlQuery(&statement, schema_query.c_str(), &error),
	           "Failed to set AdbcStatement with SQL query: " + schema_query);

	ArrowArrayStream stream = {};
	int64_t rows_affected = -1;
	CheckError(AdbcStatementExecuteQuery(&statement, &stream, &rows_affected, &error),
	           "Failed to execute AdbcStatement with SQL query: " + schema_query);

	ArrowSchema schema = {};
	stream.get_schema(&stream, &schema);

	if (schema.n_children != 1) {
		throw IOException("Expected table with 1 child, got " + schema.n_children);
	}

	if (strcmp(schema.children[0]->name, "table_name") != 0) {
		throw IOException(string("Expected table with column with name \"table_name\", got ") + schema.children[0]->name);
	}

	schema.release(&schema);

	vector<string> table_names;
	DataChunk output_chunk;
	output_chunk.Initialize(Allocator::DefaultAllocator(), {LogicalType::VARCHAR});
	arrow_column_map_t conversion_map;
	conversion_map[0] = make_shared_ptr<ArrowType>(LogicalType::VARCHAR);

	while (true) {
		ArrowArray chunk;
		int return_code = stream.get_next(&stream, &chunk);

		if (return_code != 0) {
			throw IOException("ArrowArrayStream returned error code: " + return_code);
		}

		if (chunk.release == nullptr) {
			break;
		}

		auto chunk_wrapper = make_uniq<ArrowArrayWrapper>();
		chunk_wrapper->arrow_array = chunk;
		ArrowScanLocalState local_state(std::move(chunk_wrapper), context);

		ArrowTableFunction::ArrowToDuckDB(local_state, conversion_map, output_chunk, 0);

		auto &names_vector = output_chunk.data[0];
		for (idx_t vector_idx = 0; vector_idx < output_chunk.size(); vector_idx++) {
			auto val = names_vector.GetValue(vector_idx);
			table_names.push_back(val.ToString());
		}
	}

	stream.release(&stream);
	CheckError(AdbcStatementRelease(&statement, &error), "Failed to release AdbcStatement");

	return table_names;
}

SnowflakeConnectionManager &SnowflakeConnectionManager::GetInstance() {
	static SnowflakeConnectionManager instance;
	return instance;
}

std::shared_ptr<SnowflakeConnection> SnowflakeConnectionManager::GetConnection(const std::string &connection_string,
                                                   const SnowflakeConfig &config) {
	std::lock_guard<std::mutex> lock(connection_mutex);

	auto it = connections.find(connection_string);
	if (it != connections.end() && it->second->IsConnected()) {
		return it->second;
	}

	// Create new connection
	auto connection = std::make_shared<SnowflakeConnection>();
	connection->Connect(config);

	connections[connection_string] = connection;
	return connection;
}

std::shared_ptr<SnowflakeConnection> SnowflakeConnectionManager::GetConnection(const std::string &connection_string) {
	auto config = SnowflakeConfig::ParseConnectionString(connection_string);
	return this->GetConnection(connection_string, config);
}

void SnowflakeConnectionManager::ReleaseConnection(const std::string &connection_string) {
	std::lock_guard<std::mutex> lock(connection_mutex);
	connections.erase(connection_string);
}

} // namespace snowflake
} // namespace duckdb