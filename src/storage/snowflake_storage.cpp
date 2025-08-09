#include "storage/snowflake_storage.hpp"
#include "storage/snowflake_catalog.hpp"

#include "duckdb.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<Catalog> SnowflakeAttach(StorageExtensionInfo *storage_info, ClientContext &context,
                                           AttachedDatabase &db, const string &name, AttachInfo &info,
                                           AccessMode access_mode) {
	auto connection_string = info.path;
	auto config = SnowflakeConfig::ParseConnectionString(connection_string);

	if (access_mode != AccessMode::READ_ONLY) {
		throw NotImplementedException("Snowflake currently only supports read-only access");
	}

	return make_uniq<SnowflakeCatalog>(db, config);
}

SnowflakeStorageExtension::SnowflakeStorageExtension() {
	// TODO implement transaction manager in the future
	attach = SnowflakeAttach;
}

} // namespace snowflake
} // namespace duckdb
