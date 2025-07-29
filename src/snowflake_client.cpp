#include "duckdb/common/exception.hpp"
#include "duckdb/function/table/arrow.hpp"

#include "snowflake_client.hpp"
#include "snowflake_types.hpp"

namespace duckdb {
	namespace snowflake {

		SnowflakeClient::SnowflakeClient() {
			std::memset(&database, 0, sizeof(database));
			std::memset(&connection, 0, sizeof(connection));
		}

		SnowflakeClient::~SnowflakeClient() {
			Disconnect();
		}

		void SnowflakeClient::Connect(const SnowflakeConfig& config) {
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

		void SnowflakeClient::InitializeDatabase(const SnowflakeConfig& config) {
			AdbcError error;
			AdbcStatusCode status = AdbcDatabaseNew(&database, &error);
			CheckError(status, "Failed to create ADBC database", &error);

			// Use ADBC driver manager to load the Snowflake driver
			// Load the specific driver path at runtime using the macro from CMakeLists.txt
			status = AdbcDatabaseSetOption(&database, "driver", SNOWFLAKE_ADBC_LIB_PATH, &error);
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

			if (!config.schema.empty()) {
				status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.schema", config.schema.c_str(), &error);
				CheckError(status, "Failed to set schema", &error);
			}

			if (!config.role.empty()) {
				status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.role", config.role.c_str(), &error);
				CheckError(status, "Failed to set role", &error);
			}

			// Set query timeout
			std::string timeout_str = std::to_string(config.query_timeout);
			status = AdbcDatabaseSetOption(&database, "adbc.snowflake.sql.client_session_keep_alive",
				config.keep_alive ? "true" : "false", &error);
			CheckError(status, "Failed to set keep alive", &error);

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

		void SnowflakeClient::CheckError(const AdbcStatusCode status, const std::string& operation, AdbcError* error) {
			if (status == ADBC_STATUS_OK) {
				return;
			}

			std::string error_message = operation + ": " + ((error && error->message) ? error->message : "Unknown ADBC error.");

			if (error && error->release) {
				error->release(error);
			}

			throw IOException(error_message);
		}

		vector<string> SnowflakeClient::ListSchemas(ClientContext& context) {
			const string schema_query = "SHOW SCHEMAS";
			// TODO these column names might be wrong, have to check during testing
			const vector<string> expected_names = { "created_on",
												   "name",
												   "is_default",
												   "is_current",
												   "database_name",
												   "owner",
												   "comment",
												   "options",
												   "retention_time",
												   "owner_role_type",
												   "classification_profile_database",
												   "classification_profile_schema",
												   "classification_profile",
												   "object_visibility" };
			// TODO verify these types are correct
			const vector<LogicalType> expected_types = { LogicalType::TIMESTAMP_TZ, LogicalType::VARCHAR, LogicalType::VARCHAR,
														LogicalType::VARCHAR,      LogicalType::VARCHAR, LogicalType::VARCHAR,
														LogicalType::VARCHAR,      LogicalType::VARCHAR, LogicalType::VARCHAR,
														LogicalType::VARCHAR,      LogicalType::VARCHAR, LogicalType::VARCHAR,
														LogicalType::VARCHAR,      LogicalType::VARCHAR };

			auto chunk = ExecuteAndGetChunk(context, schema_query, expected_names, expected_types);

			vector<string> schema_names;

			for (idx_t chunk_idx = 0; chunk_idx < chunk->size(); chunk_idx++) {
				schema_names.push_back(chunk->GetValue(1, chunk_idx).ToString());
			}

			return schema_names;
		}

		vector<string> SnowflakeClient::ListTables(ClientContext& context) {
			const string table_name_query = "SELECT table_name FROM " + config.database +
				".information_schema.tables WHERE table_schema = '" + config.schema + "'";
			const vector<string> expected_names = { "table_name" };
			const vector<LogicalType> expected_types = { LogicalType::VARCHAR };

			auto chunk = ExecuteAndGetChunk(context, table_name_query, expected_names, expected_types);

			vector<string> table_names;

			for (idx_t chunk_idx = 0; chunk_idx < chunk->size(); chunk_idx++) {
				table_names.push_back(chunk->GetValue(0, chunk_idx).ToString());
			}

			return table_names;
		}

		vector<SnowflakeColumn> SnowflakeClient::GetTableInfo(ClientContext& context, const string& schema, const string& table_name) {
			const string table_info_query = "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE FROM "
				+ config.database + ".information_schema.columns WHERE table_schema = '" + config.schema +
				"' AND table_name = '" + table_name + "' ORDER BY ORDINAL_POSITION";

			const vector<string> expected_names = { "COLUMN_NAME", "DATA_TYPE", "IS_NULLABLE" };
			const vector<LogicalType> expected_types = { LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR };

			auto chunk = ExecuteAndGetChunk(context, table_info_query, expected_names, expected_types);

			vector<SnowflakeColumn> col_data;

			for (idx_t chunk_idx = 0; chunk_idx < chunk->size(); chunk_idx++) {
				string nullable_str = chunk->GetValue(2, chunk_idx).ToString();
				bool is_nullable = (nullable_str == "YES");

				string type_str = chunk->GetValue(1, chunk_idx).ToString();
				LogicalType duckdb_type = SnowflakeTypeToLogicalType(type_str);

				SnowflakeColumn new_col = { chunk->GetValue(0, chunk_idx).ToString(), duckdb_type, is_nullable };
				col_data.emplace_back(new_col);
			}

			return col_data;
		}

		unique_ptr<DataChunk> SnowflakeClient::ExecuteAndGetChunk(ClientContext& context, const string& query,
			const vector<string>& expected_names,
			const vector<LogicalType>& expected_types) {
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
			stream.get_schema(&stream, &schema);

			vector<unique_ptr<DataChunk>> collected_chunks;

			arrow_column_map_t conversion_map;
			for (idx_t map_idx = 0; map_idx < expected_types.size(); map_idx++) {
				conversion_map[map_idx] = make_shared_ptr<ArrowType>(expected_types[map_idx]);
			}

			while (true) {
				ArrowArray arrow_array;
				int return_code = stream.get_next(&stream, &arrow_array);

				if (return_code != 0) {
					throw IOException("ArrowArrayStream returned error code: " + return_code);
				}

				if (arrow_array.release == nullptr) {
					break;
				}

				auto temp_chunk = make_uniq<DataChunk>();
				temp_chunk->Initialize(Allocator::DefaultAllocator(), expected_types);

				auto array_wrapper = make_uniq<ArrowArrayWrapper>();
				array_wrapper->arrow_array = arrow_array;
				ArrowScanLocalState local_state(std::move(array_wrapper), context);

				ArrowTableFunction::ArrowToDuckDB(local_state, conversion_map, *temp_chunk, 0);

				collected_chunks.emplace_back(std::move(temp_chunk));
			}

			auto result_chunk = make_uniq<DataChunk>();
			result_chunk->Initialize(Allocator::DefaultAllocator(), expected_types);

			for (const auto& chunk : collected_chunks) {
				result_chunk->Append(*chunk);
			}

			stream.release(&stream);
			CheckError(AdbcStatementRelease(&statement, &error), "Failed to release AdbcStatement", &error);

			return result_chunk;
		}

	} // namespace snowflake
} // namespace duckdb
