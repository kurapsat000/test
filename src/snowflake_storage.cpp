#include "snowflake_storage.hpp"

#include "duckdb.hpp"

namespace duckdb {
namespace snowflake {

static unique_ptr<Catalog> SnowflakeAttach(StorageExtensionInfo *storage_info, ClientContext &context,
                                        AttachedDatabase &db, const string &name, AttachInfo &info,
                                        AccessMode access_mode) {
    Connection duckdb_connection(db.GetDatabase());

    auto safe_path = StringUtil::Replace(info.path, "'", "''");
    auto query = StringUtil::Format("CALL snowflake_attach('%s')", safe_path);

    duckdb_connection.Query(query);

    // No catalog to be returned
    return nullptr;
}

SnowflakeStorageExtension::SnowflakeStorageExtension() {
    // TODO implement transaction manager in the future
	attach = SnowflakeAttach;
}

} // namespace snowflake
} // namespace duckdb
