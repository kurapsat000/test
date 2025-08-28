#include "storage/snowflake_schema_entry.hpp"

#include "storage/snowflake_table_set.hpp"

namespace duckdb {
namespace snowflake {

SnowflakeSchemaEntry::SnowflakeSchemaEntry(Catalog &catalog, const string &schema_name, CreateSchemaInfo &info,
                                           shared_ptr<SnowflakeClient> client)
    : SchemaCatalogEntry(catalog, info), client(client) {
	name = schema_name;
	tables = make_uniq<SnowflakeTableSet>(*this, client, schema_name);
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::LookupEntry(CatalogTransaction transaction, const string &name,
                                                             OnEntryNotFound if_not_found) {
	if (!CatalogTypeIsSupported(CatalogType::TABLE_ENTRY)) {
		return nullptr;
	}

	auto entry = tables->GetEntry(transaction.GetContext(), name);
	if (!entry && if_not_found == OnEntryNotFound::THROW_EXCEPTION) {
		throw BinderException("Table with name \"%s\" not found", name);
	}
	return entry;
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::GetEntry(CatalogTransaction transaction, CatalogType type,
                                                          const string &name) {
	if (!CatalogTypeIsSupported(type)) {
		return nullptr;
	}
	return tables->GetEntry(transaction.GetContext(), name);
}

void SnowflakeSchemaEntry::Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) {
	throw NotImplementedException("Snowflake does not support context-less scan");
}

void SnowflakeSchemaEntry::Scan(ClientContext &context, CatalogType type,
                                const std::function<void(CatalogEntry &)> &callback) {
	if (!CatalogTypeIsSupported(CatalogType::TABLE_ENTRY)) {
		return;
	}

	tables->Scan(context, callback);
}

bool SnowflakeSchemaEntry::CatalogTypeIsSupported(CatalogType type) {
	switch (type) {
	case CatalogType::TABLE_ENTRY:
		return true;
	case CatalogType::INDEX_ENTRY:
	case CatalogType::TYPE_ENTRY:
	case CatalogType::VIEW_ENTRY:
	default:
		return false;
	}
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateIndex(CatalogTransaction transaction, CreateIndexInfo &info,
                                                             TableCatalogEntry &table) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateFunction(CatalogTransaction transaction,
                                                                CreateFunctionInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateTable(CatalogTransaction transaction,
                                                             BoundCreateTableInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateView(CatalogTransaction transaction, CreateViewInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateSequence(CatalogTransaction transaction,
                                                                CreateSequenceInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateTableFunction(CatalogTransaction transaction,
                                                                     CreateTableFunctionInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateCopyFunction(CatalogTransaction transaction,
                                                                    CreateCopyFunctionInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreatePragmaFunction(CatalogTransaction transaction,
                                                                      CreatePragmaFunctionInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateCollation(CatalogTransaction transaction,
                                                                 CreateCollationInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

optional_ptr<CatalogEntry> SnowflakeSchemaEntry::CreateType(CatalogTransaction transaction, CreateTypeInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

void SnowflakeSchemaEntry::DropEntry(ClientContext &context, DropInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

void SnowflakeSchemaEntry::Alter(CatalogTransaction transaction, AlterInfo &info) {
	throw NotImplementedException("Snowflake schema is read-only");
}

} // namespace snowflake
} // namespace duckdb
