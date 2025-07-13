#pragma once

#include "duckdb.hpp"
#include "snowflake_connection.hpp"

namespace duckdb {
namespace snowflake {

struct SnowflakeAttachData : public TableFunctionData {
	SnowflakeConfig config;
};

class SnowflakeAttachFunction : public TableFunction {
public:
	SnowflakeAttachFunction();
};

} // namespace snowflake
} // namespace duckdb