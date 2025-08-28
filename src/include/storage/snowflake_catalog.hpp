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

	~SnowflakeCatalog();

	// Required overrides
	void Initialize(bool load_builtin) override;
	string GetCatalogType() override {
		return "snowflake";
	}

	// Schema operations (read-only)
	void ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) override;

	optional_ptr<SchemaCatalogEntry> LookupSchema(CatalogTransaction transaction, const string &schema_name,
	                                              OnEntryNotFound if_not_found) override;

	CatalogLookupBehavior CatalogTypeLookupRule(CatalogType type) const override {
		if (type == CatalogType::TABLE_ENTRY) {
			return CatalogLookupBehavior::STANDARD;
		}
		return CatalogLookupBehavior::NEVER_LOOKUP;
	}

	optional_ptr<CatalogEntry> CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) override;

	void DropSchema(ClientContext &context, DropInfo &info) override;

	DatabaseSize GetDatabaseSize(ClientContext &context) override;

	bool InMemory() override;

	string GetDBPath() override;

	// Plan operations (not supported yet, read-only)
	unique_ptr<PhysicalOperator> PlanCreateTableAs(ClientContext &context, LogicalCreateTable &op,
	                                               unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<PhysicalOperator> PlanInsert(ClientContext &context, LogicalInsert &op,
	                                        unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<PhysicalOperator> PlanDelete(ClientContext &context, LogicalDelete &op,
	                                        unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<PhysicalOperator> PlanUpdate(ClientContext &context, LogicalUpdate &op,
	                                        unique_ptr<PhysicalOperator> plan) override;

private:
	shared_ptr<SnowflakeClient> client;
	SnowflakeSchemaSet schemas;
};
} // namespace snowflake
} // namespace duckdb
