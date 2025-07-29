#include "snowflake_schema_set.hpp"

namespace duckdb {
	namespace snowflake {
		void SnowflakeSchemaSet::LoadEntries(ClientContext& context) {
			vector<string> schema_names = client.ListSchemas(context);

			for (const auto& schema_name : schema_names) {
				auto schema_entry = make_uniq<SnowflakeSchemaEntry>();
				entries.insert({ schema_name, std::move(schema_entry) });
			}
		}
	} // namespace snowflake
} // namespace duckdb

