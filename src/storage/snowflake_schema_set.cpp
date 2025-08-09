#include "storage/snowflake_schema_set.hpp"
#include "storage/snowflake_table_set.hpp"

namespace duckdb {
namespace snowflake {
void SnowflakeSchemaSet::LoadEntries(ClientContext &context) {
	vector<string> schema_names = client.ListSchemas(context);

	for (const auto &schema_name : schema_names) {
		auto schema_info = make_uniq<CreateSchemaInfo>();
		schema_info->schema = schema_name;
		auto schema_entry = make_uniq<SnowflakeSchemaEntry>(catalog, schema_name, *schema_info);
		entries.insert({schema_name, std::move(schema_entry)});
	}
}
} // namespace snowflake
} // namespace duckdb
