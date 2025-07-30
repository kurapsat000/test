#pragma once

#include "snowflake_catalog_set.hpp"
#include "snowflake_schema_entry.hpp"
#include "snowflake_client.hpp"

namespace duckdb {
namespace snowflake {
class SnowflakeSchemaSet : public SnowflakeCatalogSet {
public:
	SnowflakeSchemaSet(Catalog &catalog, SnowflakeClient &client) : SnowflakeCatalogSet(catalog), client(client) {
	}

	//! Fetches all schemas from Snowflake and creates SnowflakeSchemaEntry objects for each
	void LoadEntries(ClientContext &context) override;

private:
	SnowflakeClient &client;
};
} // namespace snowflake
} // namespace duckdb
