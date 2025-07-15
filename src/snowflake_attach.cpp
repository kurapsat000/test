#include "duckdb/function/table_function.hpp"
#include "snowflake_attach.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<FunctionData> AttachBind(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	try {
		auto bind_data = make_uniq<SnowflakeAttachData>();
		auto connection_string = input.inputs[0].GetValue<string>();

		bind_data->connection_string = connection_string;
		bind_data->config = SnowflakeConfig::ParseConnectionString(connection_string);

		return_types.emplace_back(LogicalType::BOOLEAN);
		names.emplace_back("Success");

		return move(bind_data);
	} catch (const std::exception &e) {
		throw BinderException("Failed to attach Snowflake Database: %s", e.what());
	}
}

static void AttachFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &attach_data = (SnowflakeAttachData &)(*data_p.bind_data);

	auto &manager = SnowflakeConnectionManager::GetInstance();
	auto connection = manager.GetConnection(attach_data.connection_string, attach_data.config);

	auto table_names = connection->ListTables(context);

	auto duckdb_connection = Connection(*context.db);
	for (const auto &name : table_names) {
		auto safe_connection_string = StringUtil::Replace(attach_data.connection_string, "'", "''");

		string create_view_sql = StringUtil::Format(
		    "CREATE OR REPLACE VIEW \"%s\" AS SELECT * FROM snowflake_scan('%s', 'SELECT * FROM \"%s\"')", name,
		    safe_connection_string, name);

		duckdb_connection.Query(create_view_sql);
	}

    output.data[0].SetValue(0, true);
    output.SetCardinality(1);
}

SnowflakeAttachFunction::SnowflakeAttachFunction()
    : TableFunction("snowflake_attach", {LogicalType::VARCHAR}, AttachFunction, AttachBind) {
}

} // namespace snowflake
} // namespace duckdb
