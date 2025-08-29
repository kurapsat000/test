#include "storage/snowflake_schema_set.hpp"
#include "storage/snowflake_table_set.hpp"

namespace duckdb {
namespace snowflake {
void SnowflakeSchemaSet::LoadEntries(ClientContext &context) {
	fprintf(stderr, "[DEBUG] SnowflakeSchemaSet::LoadEntries called\n");
	vector<string> schema_names = client->ListSchemas(context);
	fprintf(stderr, "[DEBUG] Got %zu schemas from ListSchemas\n", schema_names.size());

	for (const auto &schema_name : schema_names) {
		fprintf(stderr, "[DEBUG] Creating schema entry for: %s\n", schema_name.c_str());
		auto schema_info = make_uniq<CreateSchemaInfo>();
		schema_info->schema = schema_name;
		auto schema_entry = make_uniq<SnowflakeSchemaEntry>(catalog, schema_name, *schema_info, client);
		entries.insert({schema_name, std::move(schema_entry)});
	}
	fprintf(stderr, "[DEBUG] SnowflakeSchemaSet::LoadEntries completed with %zu entries\n", entries.size());
}
} // namespace snowflake
} // namespace duckdb
