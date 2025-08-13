#include "storage/snowflake_storage.hpp"
#include "storage/snowflake_catalog.hpp"
#include "snowflake_transaction.hpp"

#include "duckdb.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<Catalog> SnowflakeAttach(StorageExtensionInfo *storage_info, ClientContext &context,
                                           AttachedDatabase &db, const string &name, AttachInfo &info,
                                           AccessMode access_mode) {
	fprintf(stderr, "[DEBUG] SnowflakeAttach called with name: %s\n", name.c_str());
	auto connection_string = info.path;
	auto config = SnowflakeConfig::ParseConnectionString(connection_string);
	fprintf(stderr, "[DEBUG] Parsed config - Database: %s\n", config.database.c_str());

	if (access_mode != AccessMode::READ_ONLY) {
		throw NotImplementedException("Snowflake currently only supports read-only access");
	}

	fprintf(stderr, "[DEBUG] Creating SnowflakeCatalog\n");
	return make_uniq<SnowflakeCatalog>(db, config);
}

SnowflakeStorageExtension::SnowflakeStorageExtension() {
	attach = SnowflakeAttach;
	create_transaction_manager = SnowflakeCreateTransactionManager;
}

} // namespace snowflake
} // namespace duckdb
