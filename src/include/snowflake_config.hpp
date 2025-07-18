#include <string>

namespace duckdb {
namespace snowflake {

enum class SnowflakeAuthType { PASSWORD, OAUTH, KEY_PAIR };

struct SnowflakeConfig {
	std::string account;
	std::string username;
	std::string password;
	std::string warehouse;
	std::string database;
	std::string schema;
	std::string role;
	SnowflakeAuthType auth_type = SnowflakeAuthType::PASSWORD;
	std::string oauth_token;
	std::string private_key;
	int32_t query_timeout = 300; // seconds
	bool keep_alive = true;

	static SnowflakeConfig ParseConnectionString(const std::string &connection_string);
};

} // namespace snowflake
} // namespace duckdb