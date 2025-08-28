#include "storage/snowflake_catalog.hpp"

#include "duckdb/storage/database_size.hpp"
#include "snowflake_debug.hpp"
#include "storage/snowflake_schema_entry.hpp"

namespace duckdb {
namespace snowflake {

SnowflakeCatalog::SnowflakeCatalog(AttachedDatabase &db_p, const SnowflakeConfig &config)
    : Catalog(db_p), client(SnowflakeClientManager::GetInstance().GetConnection(config)), schemas(*this, client) {
	DPRINT("SnowflakeCatalog constructor called\n");
	if (!client || !client->IsConnected()) {
		throw ConnectionException("Failed to connect to Snowflake");
	}
	DPRINT("SnowflakeCatalog connected successfully\n");
}

SnowflakeCatalog::~SnowflakeCatalog() {
	// TODO consider adding option to allow connections to persist if user wants to DETACH and ATTACH multiple times
	auto &client_manager = SnowflakeClientManager::GetInstance();
	client_manager.ReleaseConnection(client->GetConfig());
}

void SnowflakeCatalog::Initialize(bool load_builtin) {
	DPRINT("SnowflakeCatalog::Initialize called with load_builtin=%s\n", load_builtin ? "true" : "false");
}

void SnowflakeCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	DPRINT("SnowflakeCatalog::ScanSchemas called\n");
	schemas.Scan(context, [&](CatalogEntry &schema) {
		DPRINT("ScanSchemas callback for schema: %s\n", schema.name.c_str());
		callback(schema.Cast<SchemaCatalogEntry>());
	});
	DPRINT("SnowflakeCatalog::ScanSchemas completed\n");
}

optional_ptr<SchemaCatalogEntry> SnowflakeCatalog::LookupSchema(CatalogTransaction transaction,
                                                                const string &schema_name,
                                                                OnEntryNotFound if_not_found) {
	auto found_entry = schemas.GetEntry(transaction.GetContext(), schema_name);
	if (!found_entry && if_not_found == OnEntryNotFound::THROW_EXCEPTION) {
		throw BinderException("Schema with name \"%s\" not found", schema_name);
	}
	return dynamic_cast<SchemaCatalogEntry *>(found_entry.get());
}

optional_ptr<SchemaCatalogEntry> SnowflakeCatalog::GetSchema(CatalogTransaction transaction, const string &schema_name,
                                                             OnEntryNotFound if_not_found,
                                                             QueryErrorContext error_context) {
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
	// Return a descriptive path for the Snowflake database
	const auto &config = client->GetConfig();
	return config.account + "." + config.database;
}

unique_ptr<PhysicalOperator> SnowflakeCatalog::PlanCreateTableAs(ClientContext &context, LogicalCreateTable &op,
                                                                 unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

unique_ptr<PhysicalOperator> SnowflakeCatalog::PlanInsert(ClientContext &context, LogicalInsert &op,
                                                          unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

unique_ptr<PhysicalOperator> SnowflakeCatalog::PlanDelete(ClientContext &context, LogicalDelete &op,
                                                          unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

unique_ptr<PhysicalOperator> SnowflakeCatalog::PlanUpdate(ClientContext &context, LogicalUpdate &op,
                                                          unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

unique_ptr<LogicalOperator> SnowflakeCatalog::BindCreateIndex(Binder &binder, CreateStatement &stmt,
                                                              TableCatalogEntry &table,
                                                              unique_ptr<LogicalOperator> plan) {
	throw NotImplementedException("Snowflake catalog is read-only");
}

} // namespace snowflake
} // namespace duckdb
