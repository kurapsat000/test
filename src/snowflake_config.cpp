#include "snowflake_config.hpp"
#include "duckdb/common/exception.hpp"

#include <sstream>
#include <regex>

namespace duckdb {
namespace snowflake {

SnowflakeConfig SnowflakeConfig::ParseConnectionString(const std::string &connection_string) {
	SnowflakeConfig config;
	config.connection_string = connection_string;

	// Parse key=value pairs separated by semicolons
	std::regex param_regex("([^=;]+)=([^;]*)");
	auto params_begin = std::sregex_iterator(connection_string.begin(), connection_string.end(), param_regex);
	auto params_end = std::sregex_iterator();

	for (auto it = params_begin; it != params_end; ++it) {
		std::string key = (*it)[1].str();
		std::string value = (*it)[2].str();

		if (key == "account") {
			config.account = value;
		} else if (key == "username" || key == "user") {
			config.username = value;
		} else if (key == "password") {
			config.password = value;
		} else if (key == "warehouse") {
			config.warehouse = value;
		} else if (key == "database") {
			config.database = value;
		}
		// else if (key == "schema") {
		// 	config.schema = value;
		// }
		else if (key == "role") {
			config.role = value;
		} else if (key == "auth_type") {
			if (value == "password") {
				config.auth_type = SnowflakeAuthType::PASSWORD;
			} else if (value == "oauth") {
				config.auth_type = SnowflakeAuthType::OAUTH;
			} else if (value == "key_pair") {
				config.auth_type = SnowflakeAuthType::KEY_PAIR;
			}
		} else if (key == "token") {
			config.oauth_token = value;
		} else if (key == "private_key") {
			config.private_key = value;
		} else if (key == "query_timeout") {
			config.query_timeout = std::stoi(value);
		} else if (key == "keep_alive") {
			config.keep_alive = (value == "true" || value == "1");
		}
	}

	// Validate required fields
	if (config.account.empty()) {
		throw InvalidInputException("Snowflake connection string missing required 'account' parameter");
	}

	return config;
}

bool SnowflakeConfig::operator==(const SnowflakeConfig &other) const {
	return (connection_string == other.connection_string && account == other.account && username == other.username &&
	        password == other.password && warehouse == other.warehouse && database == other.database &&
	        role == other.role && auth_type == other.auth_type && oauth_token == other.oauth_token &&
	        private_key == other.private_key && query_timeout == other.query_timeout && keep_alive == other.keep_alive);
}

} // namespace snowflake
} // namespace duckdb