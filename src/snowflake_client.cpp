#include "duckdb/common/exception.hpp"
#include "duckdb/function/table/arrow.hpp"

#include "snowflake_client.hpp"
#include "snowflake_types.hpp"

#include <dlfcn.h>
#include <filesystem>

#ifndef SNOWFLAKE_ADBC_LIB
#define SNOWFLAKE_ADBC_LIB "libadbc_driver_snowflake.so"
#endif

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

const SnowflakeConfig &SnowflakeClient::GetConfig() const {
	return config;
}

void SnowflakeClient::InitializeDatabase(const SnowflakeConfig &config) {
	AdbcError error;
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

	if (error && error->release) {
		error->release(error);
	}

	throw IOException(error_message);
}

vector<string> SnowflakeClient::ListSchemas(ClientContext &context) {
	const string schema_query = "SELECT schema_name FROM " + config.database + ".INFORMATION_SCHEMA.SCHEMATA";
	return ExecuteAndGetStrings(context, schema_query, "schema_name");
}

vector<string> SnowflakeClient::ListAllTables(ClientContext &context) {
	fprintf(stderr, "[DEBUG] ListAllTables called for database: %s\n", config.database.c_str());
	const string table_name_query = "SELECT table_name FROM " + config.database + ".information_schema.tables";
	fprintf(stderr, "[DEBUG] Table query: %s\n", table_name_query.c_str());

	auto table_names = ExecuteAndGetStrings(context, table_name_query, "table_name");

	fprintf(stderr, "[DEBUG] ListAllTables returning %zu tables\n", table_names.size());
	for (const auto &table_name : table_names) {
		fprintf(stderr, "[DEBUG] Found table: %s\n", table_name.c_str());
	}
	return table_names;
}

vector<string> SnowflakeClient::ListTables(ClientContext &context, const string &schema) {
	fprintf(stderr, "[DEBUG] ListTables called for schema: %s in database: %s\n", schema.c_str(),
	        config.database.c_str());
	const string table_name_query = "SELECT table_name FROM " + config.database +
	                                ".information_schema.tables WHERE table_schema = '" + schema + "'";
	fprintf(stderr, "[DEBUG] Table query: %s\n", table_name_query.c_str());

	auto table_names = ExecuteAndGetStrings(context, table_name_query, "table_name");

	fprintf(stderr, "[DEBUG] ListTables returning %zu tables\n", table_names.size());
	for (const auto &table_name : table_names) {
		fprintf(stderr, "[DEBUG] Found table: %s\n", table_name.c_str());
	}
	return table_names;
}

vector<SnowflakeColumn> SnowflakeClient::GetTableInfo(ClientContext &context, const string &schema,
                                                      const string &table_name) {
	const string table_info_query = "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE FROM " + config.database +
	                                ".information_schema.columns WHERE table_schema = '" + schema +
	                                "' AND table_name = '" + table_name + "' ORDER BY ORDINAL_POSITION";

	const vector<string> expected_names = {"COLUMN_NAME", "DATA_TYPE", "IS_NULLABLE"};
	const vector<LogicalType> expected_types = {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR};

	auto chunk = ExecuteAndGetChunk(context, table_info_query, expected_types, expected_names);

	vector<SnowflakeColumn> col_data;

	for (idx_t chunk_idx = 0; chunk_idx < chunk->size(); chunk_idx++) {
		string nullable_str = chunk->GetValue(2, chunk_idx).ToString();
		bool is_nullable = (nullable_str == "YES");

		string type_str = chunk->GetValue(1, chunk_idx).ToString();
		LogicalType duckdb_type = SnowflakeTypeToLogicalType(type_str);

		SnowflakeColumn new_col = {chunk->GetValue(0, chunk_idx).ToString(), duckdb_type, is_nullable};
		col_data.emplace_back(new_col);
	}

	return col_data;
}

unique_ptr<DataChunk> SnowflakeClient::ExecuteAndGetChunk(ClientContext &context, const string &query,
                                                          const vector<LogicalType> &expected_types,
                                                          const vector<string> &expected_names) {
	fprintf(stderr, "[DEBUG] ExecuteAndGetChunk called with query: %s\n", query.c_str());
	if (!connected) {
		fprintf(stderr, "[DEBUG] ExecuteAndGetChunk: Not connected!\n");
		throw IOException("Connection must be created before ExecuteAndGetChunk is called");
	}
	fprintf(stderr, "[DEBUG] ExecuteAndGetChunk: Connection is active\n");

	AdbcStatement statement;
	AdbcError error;
	AdbcStatusCode status;

	fprintf(stderr, "[DEBUG] Creating ADBC statement...\n");
	status = AdbcStatementNew(GetConnection(), &statement, &error);
	CheckError(status, "Failed to create AdbcStatement", &error);
	fprintf(stderr, "[DEBUG] ADBC statement created successfully\n");

	fprintf(stderr, "[DEBUG] Setting SQL query on statement...\n");
	status = AdbcStatementSetSqlQuery(&statement, query.c_str(), &error);
	CheckError(status, "Failed to set AdbcStatement with SQL query: " + query, &error);
	fprintf(stderr, "[DEBUG] SQL query set successfully\n");

	ArrowArrayStream stream = {};
	int64_t rows_affected = -1;

	fprintf(stderr, "[DEBUG] Executing SQL query...\n");
	status = AdbcStatementExecuteQuery(&statement, &stream, &rows_affected, &error);
	CheckError(status, "Failed to execute AdbcStatement with SQL query: " + query, &error);
	fprintf(stderr, "[DEBUG] SQL query executed successfully, rows_affected: %ld\n", rows_affected);

	fprintf(stderr, "[DEBUG] Getting Arrow schema...\n");
	ArrowSchema schema = {};
	int schema_result = stream.get_schema(&stream, &schema);
	fprintf(stderr, "[DEBUG] Arrow schema obtained, result: %d\n", schema_result);

	if (schema.release == nullptr) {
		fprintf(stderr, "[ERROR] Arrow schema is NULL!\n");
		throw IOException("Failed to get Arrow schema from stream");
	}

	if (static_cast<size_t>(schema.n_children) != expected_types.size()) {
		throw IOException("Schema has " + std::to_string(schema.n_children) + " columns but expected " +
		                  std::to_string(expected_types.size()));
	}

	for (size_t name_idx = 0; name_idx < expected_names.size(); name_idx++) {
		if (!schema.children[name_idx]->name) {
			throw IOException("Column at position " + std::to_string(name_idx) + " has null name");
		}

		if (!StringUtil::CIEquals(schema.children[name_idx]->name, expected_names[name_idx])) {
			throw IOException("Expected column '" + expected_names[name_idx] + "' at position " +
			                  std::to_string(name_idx) + " but got '" + schema.children[name_idx]->name + "'");
		}
	}

	fprintf(stderr, "[DEBUG] Arrow schema: format=%s, n_children=%ld\n", schema.format ? schema.format : "NULL",
	        schema.n_children);
	for (int64_t i = 0; i < schema.n_children && i < (int64_t)expected_types.size(); i++) {
		fprintf(stderr, "[DEBUG] Arrow column %ld: format='%s', name='%s'\n", i,
		        schema.children[i]->format ? schema.children[i]->format : "NULL",
		        schema.children[i]->name ? schema.children[i]->name : "NULL");
	}

	vector<unique_ptr<DataChunk>> collected_chunks;

	fprintf(stderr, "[DEBUG] Building conversion map for %zu columns...\n", expected_types.size());
	arrow_column_map_t conversion_map;
	for (idx_t map_idx = 0; map_idx < expected_types.size(); map_idx++) {
		const auto &type = expected_types[map_idx];
		fprintf(stderr, "[DEBUG] Column %zu: type %s\n", map_idx, expected_types[map_idx].ToString().c_str());
		unique_ptr<ArrowTypeInfo> type_info;

		switch (type.id()) {
		case LogicalTypeId::VARCHAR:
		case LogicalTypeId::BLOB:
			type_info = make_uniq<ArrowStringInfo>(ArrowVariableSizeType::NORMAL);
			break;
		case LogicalTypeId::LIST:
			// TODO may need to implement ArrowListInfo (may need child info)
			break;
		case LogicalTypeId::STRUCT:
			// TODO may need to implement ArrowStructInfo
			break;
		case LogicalTypeId::DECIMAL:
			fprintf(stderr, "[DEBUG] MUST DECIMAL TIME TYPE INFO");
			// TODO may need to implement ArrowDecimalInfo
			// extract precision and scale from the LogicalType
			break;
		case LogicalTypeId::DATE:
		case LogicalTypeId::TIMESTAMP:
		case LogicalTypeId::TIME:
			fprintf(stderr, "[DEBUG] MUST IMPLEMENT TIME TYPE INFO");
			// TODO may need to implement ArrowDateTimeInfo
			// Check if needed based on Snowflake data
			break;
		default:
			type_info = nullptr;
			break;
		}

		conversion_map[map_idx] = make_shared_ptr<ArrowType>(type, std::move(type_info));
	}
	fprintf(stderr, "[DEBUG] Conversion map built with %zu entries\n", conversion_map.size());

	int batch_count = 0;
	while (true) {
		ArrowArray arrow_array;
		fprintf(stderr, "[DEBUG] Getting next Arrow batch %d...\n", batch_count);
		int return_code = stream.get_next(&stream, &arrow_array);

		if (return_code != 0) {
			fprintf(stderr, "[DEBUG] ArrowArrayStream returned error code: %d\n", return_code);
			throw IOException("ArrowArrayStream returned error code: " + std::to_string(return_code));
		}

		if (arrow_array.release == nullptr) {
			fprintf(stderr, "[DEBUG] No more Arrow batches\n");
			break;
		}
		fprintf(stderr, "[DEBUG] Got Arrow batch %d with %ld rows\n", batch_count, arrow_array.length);
		batch_count++;

		auto temp_chunk = make_uniq<DataChunk>();
		temp_chunk->Initialize(Allocator::DefaultAllocator(), expected_types);

		auto array_wrapper = make_uniq<ArrowArrayWrapper>();
		array_wrapper->arrow_array = arrow_array;

		fprintf(stderr, "[DEBUG] Arrow array details: n_buffers=%ld, n_children=%ld\n", arrow_array.n_buffers,
		        arrow_array.n_children);

		fprintf(stderr, "[DEBUG] Creating ArrowScanLocalState...\n");
		ArrowScanLocalState local_state(std::move(array_wrapper), context);

		fprintf(stderr, "[DEBUG] Calling ArrowToDuckDB with conversion_map size %zu...\n", conversion_map.size());
		try {
			ArrowTableFunction::ArrowToDuckDB(local_state, conversion_map, *temp_chunk, 0);
			fprintf(stderr, "[DEBUG] ArrowToDuckDB completed, chunk size: %zu\n", temp_chunk->size());
		} catch (const std::exception &e) {
			fprintf(stderr, "[ERROR] ArrowToDuckDB failed: %s\n", e.what());
			throw;
		}

		collected_chunks.emplace_back(std::move(temp_chunk));
	}

	auto result_chunk = make_uniq<DataChunk>();
	result_chunk->Initialize(Allocator::DefaultAllocator(), expected_types);

	fprintf(stderr, "[DEBUG] Collected %zu chunks, combining them...\n", collected_chunks.size());
	for (const auto &chunk : collected_chunks) {
		result_chunk->Append(*chunk);
	}
	fprintf(stderr, "[DEBUG] Final result chunk has %zu rows\n", result_chunk->size());

	if (schema.release) {
		schema.release(&schema);
	}

	stream.release(&stream);
	CheckError(AdbcStatementRelease(&statement, &error), "Failed to release AdbcStatement", &error);

	fprintf(stderr, "[DEBUG] ExecuteAndGetChunk completed successfully\n");
	return result_chunk;
}

vector<string> SnowflakeClient::ExecuteAndGetStrings(ClientContext &context, const string &query,
                                                     const string &expected_col_name) {
	if (!connected) {
		throw IOException("Connection must be created before ListTables is called");
	}

	AdbcStatement statement;
	AdbcError error;
	AdbcStatusCode status;

	status = AdbcStatementNew(GetConnection(), &statement, &error);
	CheckError(status, "Failed to create AdbcStatement", &error);

	status = AdbcStatementSetSqlQuery(&statement, query.c_str(), &error);
	CheckError(status, "Failed to set AdbcStatement with SQL query: " + query, &error);

	ArrowArrayStream stream = {};
	int64_t rows_affected = -1;

	status = AdbcStatementExecuteQuery(&statement, &stream, &rows_affected, &error);
	CheckError(status, "Failed to execute AdbcStatement with SQL query: " + query, &error);

	ArrowSchema schema = {};
	int schema_result = stream.get_schema(&stream, &schema);
	if (schema_result != 0 || schema.release == nullptr) {
		throw IOException("Failed to get Arrow schema from stream");
	}

	if (!expected_col_name.empty()) {
		if (schema.n_children > 0 && schema.children[0]->name) {
			if (!StringUtil::CIEquals(schema.children[0]->name, expected_col_name)) {
				throw IOException("Expected column '" + expected_col_name + "' but got '" + schema.children[0]->name +
				                  "'");
			}
		}
	}

	vector<string> results;

	while (true) {
		ArrowArray arrow_array;
		int return_code = stream.get_next(&stream, &arrow_array);

		if (return_code != 0) {
			throw IOException("ArrowArrayStream returned error code: " + std::to_string(return_code));
		}

		if (arrow_array.release == nullptr) {
			break;
		}

		if (arrow_array.n_children > 0) {
			ArrowArray *column = arrow_array.children[0];
			if (column && column->buffers && column->n_buffers >= 3) {
				// For string columns: buffer[0] is validity, buffer[1] is offsets, buffer[2] is data
				const int32_t *offsets = (const int32_t *)column->buffers[1];
				const char *data = (const char *)column->buffers[2];

				for (int64_t i = 0; i < column->length; i++) {
					int32_t start = offsets[i];
					int32_t end = offsets[i + 1];
					std::string value(data + start, end - start);
					results.push_back(value);
				}
			}
		}

		if (arrow_array.release) {
			arrow_array.release(&arrow_array);
		}
	}

	if (schema.release) {
		schema.release(&schema);
	}
	if (stream.release) {
		stream.release(&stream);
	}
	CheckError(AdbcStatementRelease(&statement, &error), "Failed to release AdbcStatement", &error);

	return results;
}

} // namespace snowflake
} // namespace duckdb
