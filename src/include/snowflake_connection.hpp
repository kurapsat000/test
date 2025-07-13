#pragma once

#include "duckdb.hpp"
#include <arrow-adbc/adbc.h>
#include <arrow-adbc/adbc_driver_manager.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace duckdb {
namespace snowflake {

enum class SnowflakeAuthType {
    PASSWORD,
    OAUTH,
    KEY_PAIR
};

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
    int32_t query_timeout = 300;  // seconds
    bool keep_alive = true;
    
    static SnowflakeConfig ParseConnectionString(const std::string& connection_string);
};

class SnowflakeConnection {
public:
    SnowflakeConnection();
    ~SnowflakeConnection();
    
    void Connect(const SnowflakeConfig& config);
    void Disconnect();
    bool IsConnected() const;
    
    AdbcConnection* GetConnection() { return &connection; }
    AdbcDatabase* GetDatabase() { return &database; }
    
    std::string GetLastError() const { return last_error; }
    
private:
    AdbcDatabase database;
    AdbcConnection connection;
    AdbcError error;
    bool connected = false;
    std::string last_error;
    
    void InitializeDatabase(const SnowflakeConfig& config);
    void InitializeConnection();
    void CheckError(AdbcStatusCode status, const std::string& operation);
};

class SnowflakeConnectionManager {
public:
    static SnowflakeConnectionManager& GetInstance();
    
    std::shared_ptr<SnowflakeConnection> GetConnection(const std::string& connection_string, const SnowflakeConfig& config);
    
    // Convenience wrapper that does not require config
    std::shared_ptr<SnowflakeConnection> GetConnection(const std::string& connection_string);

    void ReleaseConnection(const std::string& connection_string);
    
private:
    SnowflakeConnectionManager() = default;
    std::unordered_map<std::string, std::shared_ptr<SnowflakeConnection>> connections;
    std::mutex connection_mutex;
};

} // namespace snowflake
} // namespace duckdb