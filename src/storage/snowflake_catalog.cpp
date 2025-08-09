#include "storage/snowflake_catalog.hpp"
#include "storage/snowflake_schema_entry.hpp"
#include "duckdb/storage/database_size.hpp"

namespace duckdb {
namespace snowflake {

SnowflakeCatalog::SnowflakeCatalog(AttachedDatabase &db_p, const SnowflakeConfig &config)
    : Catalog(db_p), client(SnowflakeClientManager::GetInstance().GetConnection(config.connection_string, config)),
      schemas(*this, *client) {
	if (!client || !client->IsConnected()) {
		throw ConnectionException("Failed to connect to Snowflake");
	}
}

SnowflakeCatalog::~SnowflakeCatalog() {
	// TODO consider adding option to allow connections to persist if user wants to DETACH and ATTACH multiple times
	auto &client_manager = SnowflakeClientManager::GetInstance();
	client_manager.ReleaseConnection(client->GetConfig().connection_string);
}

void SnowflakeCatalog::Initialize(bool load_builtin) {
}

void SnowflakeCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	schemas.Scan(context, [&](CatalogEntry &schema) { callback(schema.Cast<SchemaCatalogEntry>()); });
}

optional_ptr<SchemaCatalogEntry> SnowflakeCatalog::LookupSchema(CatalogTransaction transaction,
                                                                const EntryLookupInfo &schema_lookup,
                                                                OnEntryNotFound if_not_found) {
	auto schema_name = schema_lookup.GetEntryName();

	auto found_entry = schemas.GetEntry(transaction.GetContext(), schema_name);
	if (!found_entry && if_not_found == OnEntryNotFound::THROW_EXCEPTION) {
		throw BinderException("Schema with name \"%s\" not found", schema_name);
	}
	return dynamic_cast<SchemaCatalogEntry *>(found_entry.get());
}

optional_ptr<CatalogEntry> SnowflakeCatalog::CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

void SnowflakeCatalog::DropSchema(ClientContext &context, DropInfo &info) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

DatabaseSize SnowflakeCatalog::GetDatabaseSize(ClientContext &context) {
	throw NotImplementedException("Snowflake catalog does not support getting database size");
}

bool SnowflakeCatalog::InMemory() {
	return false;
}

string SnowflakeCatalog::GetDBPath() {
	return client->GetConfig().connection_string;
}

PhysicalOperator &SnowflakeCatalog::PlanCreateTableAs(ClientContext &context, PhysicalPlanGenerator &planner,
                                                      LogicalCreateTable &op, PhysicalOperator &plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

PhysicalOperator &SnowflakeCatalog::PlanInsert(ClientContext &context, PhysicalPlanGenerator &planner,
                                               LogicalInsert &op, optional_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

PhysicalOperator &SnowflakeCatalog::PlanDelete(ClientContext &context, PhysicalPlanGenerator &planner,
                                               LogicalDelete &op, PhysicalOperator &plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

PhysicalOperator &SnowflakeCatalog::PlanUpdate(ClientContext &context, PhysicalPlanGenerator &planner,
                                               LogicalUpdate &op, PhysicalOperator &plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

} // namespace snowflake
} // namespace duckdb
