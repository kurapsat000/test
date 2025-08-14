#include "snowflake_secrets.hpp"
#include "snowflake_secret_provider.hpp"
#include "snowflake_client.hpp"
#include "snowflake_client_manager.hpp"
#include "snowflake_config.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/database.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include "duckdb/main/secret/secret.hpp"
#include "duckdb/common/types/value.hpp"
#include <arrow-adbc/adbc.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>

namespace duckdb {

// Store Snowflake credentials as a secret using DuckDB's secrets manager
void SnowflakeSecretsHelper::StoreCredentials(ClientContext &context, const std::string &profile_name,
                                              const std::string &username, const std::string &password,
                                              const std::string &account, const std::string &warehouse,
                                              const std::string &database, const std::string &schema) {

	// Create a snowflake secret with all the Snowflake credentials
	CreateSecretInput input;
	input.type = "snowflake";
	input.provider = "config";
	input.name = profile_name;
	input.persist_type = SecretPersistType::PERSISTENT;

	// Store all credentials as snowflake-specific fields
	input.options["user"] = Value(username);
	input.options["password"] = Value(password);
	input.options["account"] = Value(account);
	input.options["warehouse"] = Value(warehouse);
	input.options["database"] = Value(database);
	input.options["schema"] = Value(schema);

	// Create the secret
	SecretManager::Get(context).CreateSecret(context, input);
}

// Retrieve Snowflake credentials from a secret
std::unordered_map<std::string, std::string> SnowflakeSecretsHelper::GetCredentials(ClientContext &context,
                                                                                    const std::string &profile_name) {
	std::unordered_map<std::string, std::string> credentials;

	try {
		auto &secret_manager = SecretManager::Get(context);
		auto transaction = CatalogTransaction::GetSystemCatalogTransaction(context);

		// Look up the secret by name
		auto secret_entry = secret_manager.GetSecretByName(transaction, profile_name);
		if (!secret_entry) {
			throw InvalidInputException("Snowflake profile not found: " + profile_name);
		}

		// Cast to SnowflakeSecret to access the snowflake-specific fields
		auto *snowflake_secret = dynamic_cast<const SnowflakeSecret *>(secret_entry->secret.get());
		if (!snowflake_secret) {
			throw InvalidInputException("Invalid secret type for profile: " + profile_name);
		}

		// Extract all the credential values using our custom methods
		credentials["username"] = snowflake_secret->GetUser();
		credentials["password"] = snowflake_secret->GetPassword();
		credentials["account"] = snowflake_secret->GetAccount();
		credentials["warehouse"] = snowflake_secret->GetWarehouse();
		credentials["database"] = snowflake_secret->GetDatabase();
		credentials["schema"] = snowflake_secret->GetSchema();
		credentials["profile_name"] = profile_name;

	} catch (const std::exception &e) {
		throw InvalidInputException("Failed to retrieve credentials for profile '" + profile_name + "': " + e.what());
	}

	return credentials;
}

// Delete a Snowflake credentials secret
bool SnowflakeSecretsHelper::DeleteCredentials(ClientContext &context, const std::string &profile_name) {
	try {
		auto &secret_manager = SecretManager::Get(context);
		auto transaction = CatalogTransaction::GetSystemCatalogTransaction(context);

		// Delete the secret by name
		secret_manager.DropSecretByName(transaction, profile_name, OnEntryNotFound::RETURN_NULL,
		                                SecretPersistType::PERSISTENT);
		return true;

	} catch (const std::exception &e) {
		// Log error but don't throw
		std::cerr << "Failed to delete credentials for profile '" << profile_name << "': " << e.what() << '\n';
		return false;
	}
}

// List all Snowflake profile names
std::vector<std::string> SnowflakeSecretsHelper::ListProfiles(ClientContext &context) {
	std::vector<std::string> profiles;

	try {
		auto &secret_manager = SecretManager::Get(context);
		auto transaction = CatalogTransaction::GetSystemCatalogTransaction(context);

		// Get all secrets
		auto all_secrets = secret_manager.AllSecrets(transaction);

		// Filter for Snowflake profile secrets
		for (const auto &secret_entry : all_secrets) {
			const auto &secret = *secret_entry.secret;

			// Check if this is a Snowflake secret
			if (secret.GetType() == "snowflake") {
				profiles.push_back(secret.GetName());
			}
		}

	} catch (const std::exception &e) {
		// Log error but don't throw
		std::cerr << "Failed to list profiles: " << e.what() << '\n';
	}

	return profiles;
}

// Build connection string from credentials
std::string
SnowflakeSecretsHelper::BuildConnectionString(const std::unordered_map<std::string, std::string> &credentials) {
	std::ostringstream oss;

	// Build the connection string in the format expected by Snowflake
	oss << "account=" << credentials.at("account") << ";";
	oss << "user=" << credentials.at("username") << ";";
	oss << "password=" << credentials.at("password") << ";";
	oss << "database=" << credentials.at("database") << ";";

	// Add optional fields if they exist
	if (credentials.find("warehouse") != credentials.end() && !credentials.at("warehouse").empty()) {
		oss << "warehouse=" << credentials.at("warehouse") << ";";
	}

	if (credentials.find("schema") != credentials.end() && !credentials.at("schema").empty()) {
		oss << "schema=" << credentials.at("schema") << ";";
	}

	return oss.str();
}

// Legacy implementation for backward compatibility
std::string SnowflakeSecrets::StoreCredentials(const std::string &profile_name) {
	// This is deprecated - users should use the new secrets manager approach
	return "Deprecated: Use CREATE SECRET instead of this function";
}

std::string SnowflakeSecrets::ListProfiles() {
	// This is deprecated - users should use the new secrets manager approach
	return "Deprecated: Use SELECT * FROM duckdb_secrets() WHERE name LIKE 'snowflake_profile_%' instead";
}

std::string SnowflakeSecrets::GetConnectionString(const std::string &profile_name) {
	// This is deprecated - users should use the new secrets manager approach
	return "Deprecated: Use the new secrets manager approach instead";
}

bool SnowflakeSecrets::DeleteProfile(const std::string &profile_name) {
	// This is deprecated - users should use the new secrets manager approach
	return false;
}

// Validate Snowflake credentials by testing connection
bool SnowflakeSecretsHelper::ValidateCredentials(ClientContext &context, const std::string &profile_name,
                                                 int timeout_seconds) {
	try {
		// Get credentials from secrets manager
		auto credentials = GetCredentials(context, profile_name);

		// Validate with the retrieved credentials
		return ValidateCredentials(context, credentials["username"], credentials["password"], credentials["account"],
		                           credentials["warehouse"], credentials["database"], credentials["schema"],
		                           timeout_seconds);
	} catch (const std::exception &e) {
		// Log the error but don't throw
		std::cerr << "Failed to validate credentials for profile '" << profile_name << "': " << e.what() << '\n';
		return false;
	}
}

// Validate credentials with explicit parameters
bool SnowflakeSecretsHelper::ValidateCredentials(ClientContext &context, const std::string &username,
                                                 const std::string &password, const std::string &account,
                                                 const std::string &warehouse, const std::string &database,
                                                 const std::string &schema, int timeout_seconds) {
	try {
		// Build connection string - same as what scan uses
		std::unordered_map<std::string, std::string> creds;
		creds["username"] = username;
		creds["password"] = password;
		creds["account"] = account;
		creds["warehouse"] = warehouse;
		creds["database"] = database;
		creds["schema"] = schema;
		
		std::string connection_string = BuildConnectionString(creds);
		
		// Parse config and connect - exactly like scan does
		auto config = snowflake::SnowflakeConfig::ParseConnectionString(connection_string);
		
		// Use SnowflakeClientManager like scan does
		auto &client_manager = snowflake::SnowflakeClientManager::GetInstance();
		
		try {
			// Try to get a connection - this will validate the credentials
			auto connection = client_manager.GetConnection(connection_string, config);
			
			// If we got here, connection succeeded
			// Test with a simple query to be sure
			AdbcStatement statement;
			AdbcError error_obj;
			std::memset(&error_obj, 0, sizeof(error_obj));
			std::memset(&statement, 0, sizeof(statement));
			
			AdbcStatusCode status = AdbcStatementNew(connection->GetConnection(), &statement, &error_obj);
			if (status != ADBC_STATUS_OK) {
				if (error_obj.release) {
					error_obj.release(&error_obj);
				}
				return false;
			}

			// Prepare and execute simple test query
			status = AdbcStatementSetSqlQuery(&statement, "SELECT 1", &error_obj);
			if (status != ADBC_STATUS_OK) {
				AdbcStatementRelease(&statement, &error_obj);
				if (error_obj.release) {
					error_obj.release(&error_obj);
				}
				return false;
			}

			// Execute query
			ArrowArrayStream stream;
			std::memset(&stream, 0, sizeof(stream));
			status = AdbcStatementExecuteQuery(&statement, &stream, nullptr, &error_obj);
			bool success = (status == ADBC_STATUS_OK);
			
			// Clean up
			if (stream.release) {
				stream.release(&stream);
			}
			AdbcStatementRelease(&statement, &error_obj);
			if (error_obj.release) {
				error_obj.release(&error_obj);
			}
			
			return success;
		} catch (const IOException &inner_e) {
			// Connection failed - this is expected for invalid credentials
			fprintf(stderr, "[Snowflake Validation] Connection test failed for profile\n");
			fprintf(stderr, "  Error: %s\n", inner_e.what());
			return false;
		} catch (const std::exception &inner_e) {
			// Other connection failures
			fprintf(stderr, "[Snowflake Validation] Unexpected error during connection test\n");
			fprintf(stderr, "  Error: %s\n", inner_e.what());
			return false;
		}
	} catch (const std::exception &e) {
		// Log the error but don't throw
		fprintf(stderr, "[Snowflake Validation] Failed to validate credentials: %s\n", e.what());
		return false;
	}
}

} // namespace duckdb