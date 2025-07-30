#pragma once

#include "snowflake_catalog_set.hpp"
#include "snowflake_client.hpp"

namespace duckdb {
namespace snowflake {
class SnowflakeTableSet : public SnowflakeCatalogSet {
public:
protected:
	void LoadEntries(ClientContext &context);

protected:
private:
	string schema_name;
};
} // namespace snowflake
} // namespace duckdb
