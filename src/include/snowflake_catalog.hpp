#pragma once

#include "duckdb/catalog/catalog.hpp"
#include "snowflake_client_manager.hpp"
#include "snowflake_options.hpp"
#include "snowflake_schema_set.hpp"

namespace duckdb {
namespace snowflake {

class SnowflakeCatalog : public Catalog {
public:
	// Constructor - connection info
	SnowflakeCatalog(AttachedDatabase &db_p, const SnowflakeConfig &config);

	~SnowflakeCatalog() = default;

	// Required overrides
	void Initialize(bool load_builtin) override;
	string GetCatalogType() override {
		return "snowflake";
	}

	// Schema operations (read-only)
	void ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) override;

	optional_ptr<SchemaCatalogEntry> LookupSchema(CatalogTransaction transaction, const EntryLookupInfo &schema_lookup,
	                                              OnEntryNotFound if_not_found) override;

	// Catalog type lookup
	CatalogLookupBehavior CatalogTypeLookupRule(CatalogType type) const override {
		if (type == CatalogType::TABLE_ENTRY) {
			return CatalogLookupBehavior::STANDARD;
		}
		return CatalogLookupBehavior::NEVER_LOOKUP; // No functions, views, etc.
	}

private:
	SnowflakeClient client;
	SnowflakeSchemaSet schemas;
};
// 	SnowflakeCatalog(AttachedDatabase& db_p, const SnowflakeConfig& config, SnowflakeOptions& options_p);
// 	SnowflakeCatalog(AttachedDatabase& db_p, const std::string& connection_str, SnowflakeOptions& options_p);
// 	~SnowflakeCatalog() = default;

// 	SnowflakeConfig config;
// 	SnowflakeOptions options;

// public:
// 	void Initialize(bool load_builtin) override;
// 	std::string GetCatalogType() override {
// 		return "snowflake";
// 	}

// private:
// };
} // namespace snowflake
} // namespace duckdb
