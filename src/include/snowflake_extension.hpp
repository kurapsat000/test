#pragma once

#include "duckdb.hpp"
#include "duckdb/main/extension.hpp"

namespace duckdb {

class SnowflakeExtension : public Extension {
public:
	void Load(ExtensionLoader &db) override;
	std::string Name() override;
	std::string Version() const override;

	// ADBC integration methods
	static bool InitializeADBC();
	static std::string GetADBCVersion();
};

} // namespace duckdb
