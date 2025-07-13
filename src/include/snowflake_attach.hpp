#pragma once

#include "duckdb.hpp"

namespace duckdb {
namespace snowflake {

struct SnowflakeAttachData : public TableFunctionData {
	string account;
	string user;
	string password;
	string warehouse;
	string database;
};

class SnowflakeAttachFunction : public TableFunction {
public:
	SnowflakeAttachFunction();
};

} // namespace snowflake
} // namespace duckdb