#pragma once

#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"

namespace duckdb {
namespace snowflake {
class SnowflakeSchemaEntry : public SchemaCatalogEntry {
public:
	SnowflakeSchemaEntry(Catalog &catalog, const string &schema_name, CreateSchemaInfo &info)
	    : SchemaCatalogEntry(catalog, info) {
		name = schema_name;
	}
};
} // namespace snowflake
} // namespace duckdb
