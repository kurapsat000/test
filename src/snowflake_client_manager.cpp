#include "snowflake_client_manager.hpp"

namespace duckdb {
namespace snowflake {

SnowflakeClientManager &SnowflakeClientManager::GetInstance() {
	static SnowflakeClientManager instance;
	return instance;
}

std::shared_ptr<SnowflakeClient> SnowflakeClientManager::GetConnection(const std::string &connection_string,
                                                                    const SnowflakeConfig &config) {
	std::lock_guard<std::mutex> lock(connection_mutex);

	auto it = connections.find(connection_string);
	if (it != connections.end() && it->second->IsConnected()) {
		return it->second;
	}

	// Create new connection
	auto connection = std::make_shared<SnowflakeClient>();
	connection->Connect(config);

	connections[connection_string] = connection;
	return connection;
}

std::shared_ptr<SnowflakeClient> SnowflakeClientManager::GetConnection(const std::string &connection_string) {
	auto config = SnowflakeConfig::ParseConnectionString(connection_string);
	return this->GetConnection(connection_string, config);
}

void SnowflakeClientManager::ReleaseConnection(const std::string &connection_string) {
	std::lock_guard<std::mutex> lock(connection_mutex);
	connections.erase(connection_string);
}

} // namespace snowflake
} // namespace duckdb