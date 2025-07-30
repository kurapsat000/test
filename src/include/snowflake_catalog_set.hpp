#pragma once

#include "duckdb/common/mutex.hpp"
#include "duckdb/catalog/catalog.hpp"

namespace duckdb {
namespace snowflake {
class SnowflakeCatalogSet {
public:
	SnowflakeCatalogSet(Catalog &catalog, bool is_loaded = false) : catalog(catalog), is_loaded(is_loaded) {};

	//! Get a single entry (schema/table)
	optional_ptr<CatalogEntry> GetEntry(ClientContext &context, const string &name);

	//! List all entries
	void Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback);

protected:
	//! Underlying function called by TryLoadEntries
	virtual void LoadEntries(ClientContext &context) = 0;

	//! Helper function to ensure loading happens only once
	void TryLoadEntries(ClientContext &context);

protected:
	Catalog &catalog;
	unordered_map<string, unique_ptr<CatalogEntry>> entries;

private:
	mutex entry_lock;
	mutex load_lock;
	atomic<bool> is_loaded;
};
} // namespace snowflake
} // namespace duckdb
