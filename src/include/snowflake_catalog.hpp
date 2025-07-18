#pragma once

#include "duckdb/catalog/catalog.hpp"
#include "snowflake_client_manager.hpp"
#include "snowflake_options.hpp"

namespace duckdb {
namespace snowflake {

class SnowflakeCatalog : public Catalog {
public:
	explicit SnowflakeCatalog(AttachedDatabase &db_p, const SnowflakeConfig &config, SnowflakeOptions &options_p);
    explicit SnowflakeCatalog(AttachedDatabase &db_p, const std::string &connection_str, SnowflakeOptions &options_p);
    ~SnowflakeCatalog() = default;

    SnowflakeConfig config;
    SnowflakeOptions options;

public:
    std::string GetCatalogType() override {
        return "snowflake";
    }
    
private:
};
} // namespace snowflake
} // namespace duckdb