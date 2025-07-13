#include "snowflake_utils.hpp"
#include "snowflake_attach.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/exception.hpp"

namespace duckdb {
namespace snowflake {
    void ParseConnectionString(const string &connection_string, SnowflakeAttachData &result) {
        auto entries = StringUtil::Split(connection_string, ';');
        for (const auto &entry : entries) {
            if (entry.empty()) {
                continue;
            }
            auto parts = StringUtil::Split(entry, '=');
            if (parts.size() != 2) {
                throw InvalidInputException("Invalid connection string entry, expected format 'key=value': %s", entry);
            }

            auto key = parts[0];
            auto value = parts[1];

            StringUtil::Trim(key);
            StringUtil::Trim(value);
            key = StringUtil::Lower(key);

            // TODO in the future password will be available through configuration/secrets
            if (key == "account") {
                result.account = value;
            } else if (key == "user") {
                result.user = value;
            } else if (key == "password") {
                result.password = value;
            } else if (key == "warehouse") {
                result.warehouse = value;
            } else if (key == "database") {
                result.database = value;
            } else {
                throw InvalidInputException("Unknown connection string parameter: %s", key);
            }
        }

        if (result.account.empty()) {
            throw InvalidInputException("Connection string must include parameter 'account'");
        }
        if (result.user.empty()) {
            throw InvalidInputException("Connection string must include parameter 'user'");
        }
        if (result.password.empty()) {
            throw InvalidInputException("Connection string must include parameter 'password'");
        }
    }
} // namespace snowflake
} // namespace duckdb