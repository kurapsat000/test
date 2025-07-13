#include "duckdb/function/table_function.hpp"
#include "snowflake_attach.hpp"
#include "snowflake_connection.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<FunctionData> AttachBind(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	try {
        auto bind_data = make_uniq<SnowflakeAttachData>();
        auto connection_string = input.inputs[0].GetValue<string>();

        bind_data->config = SnowflakeConfig::ParseConnectionString(connection_string);

        return_types.emplace_back(LogicalType::BOOLEAN);
        names.emplace_back("Success");

        return move(bind_data);
	} catch (const std::exception &e) {
		throw BinderException("Failed to attach Snowflake Database: %s", e.what());
	}
}

static void AttachFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
    auto &attach_data = static_cast<SnowflakeAttachData&>(*data_p.bind_data);
    // TODO: connect to ADBC
    auto &manager = SnowflakeConnectionManager::GetInstance();
    auto connection = manager.GetConnection();
}

SnowflakeAttachFunction::SnowflakeAttachFunction()
    : TableFunction("snowflake_attach", {LogicalType::VARCHAR}, AttachFunction, AttachBind) {
}

} // namespace snowflake
} // namespace duckdb
