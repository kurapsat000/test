#pragma once

#include "snowflake_client.hpp"
#include "snowflake_config.hpp"

#include <memory>
#include <unordered_map>
#include <mutex>

namespace duckdb {
namespace snowflake {

class SnowflakeClientManager {
public:
	static SnowflakeClientManager &GetInstance();

	std::shared_ptr<SnowflakeClient> GetConnection(const std::string &connection_string, const SnowflakeConfig &config);

	// Convenience wrapper that does not require config
	std::shared_ptr<SnowflakeClient> GetConnection(const std::string &connection_string);

	void ReleaseConnection(const std::string &connection_string);

private:
	SnowflakeClientManager() = default;
	std::unordered_map<std::string, std::shared_ptr<SnowflakeClient>> connections;
	std::mutex connection_mutex;
};

} // namespace snowflake
} // namespace duckdb