#pragma once

namespace duckdb {
namespace snowflake {
    struct SnowflakeAttachData;

    void ParseConnectionString(const std::string &connection_string, SnowflakeAttachData &result);
} // namespace snowflake
} // namespace duckdb