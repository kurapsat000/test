// #include "duckdb/function/table_function.hpp"
// #include "snowflake_attach.hpp"

// namespace duckdb {
// namespace snowflake {

// static unique_ptr<FunctionData> AttachBind(ClientContext &context, TableFunctionBindInput &input,
//                                            vector<LogicalType> &return_types, vector<string> &names) {
// 	try {
// 		auto bind_data = make_uniq<SnowflakeAttachData>();
// 		auto connection_string = input.inputs[0].GetValue<string>();

// 		bind_data->connection_string = connection_string;
// 		bind_data->config = SnowflakeConfig::ParseConnectionString(connection_string);

// 		return_types.emplace_back(LogicalType(LogicalTypeId::BOOLEAN));
// 		names.emplace_back("Success");

// 		return std::move(bind_data);
// 	} catch (const std::exception &e) {
// 		throw BinderException("Failed to attach Snowflake Database: %s", e.what());
// 	}
// }

// static void AttachFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
// 	fprintf(stderr, "[DEBUG] AttachFunction called\n");
// 	const auto &attach_data = dynamic_cast<const SnowflakeAttachData &>(*data_p.bind_data);
// 	fprintf(stderr, "[DEBUG] Database: %s\n", attach_data.config.database.c_str());

// 	auto &manager = SnowflakeClientManager::GetInstance();
// 	auto connection = manager.GetConnection(attach_data.connection_string, attach_data.config);

// 	fprintf(stderr, "[DEBUG] Calling ListAllTables\n");
// 	auto table_names = connection->ListAllTables(context);
// 	fprintf(stderr, "[DEBUG] ListAllTables returned %zu tables\n", table_names.size());

// 	auto duckdb_connection = Connection(*context.db);
// 	for (const auto &name : table_names) {
// 		fprintf(stderr, "[DEBUG] Creating view for table: %s\n", name.c_str());
// 		auto safe_connection_string = StringUtil::Replace(attach_data.connection_string, "'", "''");

// 		string create_view_sql = StringUtil::Format(
// 		    "CREATE OR REPLACE VIEW \"%s\" AS SELECT * FROM snowflake_scan('%s', 'SELECT * FROM \"%s\"')", name,
// 		    safe_connection_string, name);

// 		fprintf(stderr, "[DEBUG] SQL: %s\n", create_view_sql.c_str());
// 		auto result = duckdb_connection.Query(create_view_sql);
// 		if (result->HasError()) {
// 			fprintf(stderr, "[DEBUG] Error creating view: %s\n", result->GetError().c_str());
// 		}
// 	}

// 	fprintf(stderr, "[DEBUG] AttachFunction completed, created %zu views\n", table_names.size());
// 	output.data[0].SetValue(0, true);
// 	output.SetCardinality(1);
// }

// SnowflakeAttachFunction::SnowflakeAttachFunction()
//     : TableFunction("snowflake_attach", {LogicalType(LogicalTypeId::VARCHAR)}, AttachFunction, AttachBind) {
// }

// } // namespace snowflake
// } // namespace duckdb
