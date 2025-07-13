#include "duckdb/function/table_function.hpp"
#include "snowflake_attach.hpp"
#include "snowflake_utils.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<FunctionData> AttachBind(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	try {
        auto result = make_uniq<SnowflakeAttachData>();
		auto table_string = input.inputs[0].GetValue<string>();
        ParseConnectionString(table_string, *result);
        return result;
	} catch (const std::exception &e) {
		throw BinderException("Failed to attach Snowflake Database: %s", e.what());
	}
}

static void AttachFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
}

SnowflakeAttachFunction::SnowflakeAttachFunction()
    : TableFunction("snowflake_attach", {LogicalType::VARCHAR}, AttachFunction, AttachBind) {
}

} // namespace snowflake
} // namespace duckdb
