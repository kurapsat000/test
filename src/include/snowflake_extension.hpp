#pragma once

#include "duckdb.hpp"
#include "duckdb/main/extension.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

namespace duckdb {

class SnowflakeExtension : public Extension {
public:
	void Load(ExtensionLoader &loader) override;
	std::string Name() override;
	std::string Version() const override;
};

} // namespace duckdb

extern "C" {
DUCKDB_CPP_EXTENSION_ENTRY(snowflake, loader);
DUCKDB_EXTENSION_API const char *snowflake_version();
}
