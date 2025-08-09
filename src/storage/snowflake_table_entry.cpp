#include "storage/snowflake_table_entry.hpp"
#include "storage/snowflake_catalog_scan.hpp"
#include "snowflake_client_manager.hpp"
#include "snowflake_scan.hpp"
#include "duckdb/storage/table_storage_info.hpp"

namespace duckdb {
namespace snowflake {

TableFunction SnowflakeTableEntry::GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) {
	EnsureColumnsLoaded(context);

	auto &config = client->GetConfig();
	string query = "SELECT * FROM " + config.database + "." + schema.name + "." + name;

	// TODO consider maintaining a thread-safe pool of connections in client, so we can use the client within
	// SnowflakeTableEntry instead of creating a new client
	auto &client_manager = SnowflakeClientManager::GetInstance();
	auto connection = client_manager.GetConnection(config.connection_string);

	auto factory = make_uniq<SnowflakeArrowStreamFactory>(connection, query);

	auto snowflake_bind_data = make_uniq<SnowflakeScanBindData>(std::move(factory));
	snowflake_bind_data->connection_string = config.connection_string;
	snowflake_bind_data->query = query;
	SnowflakeGetArrowSchema(reinterpret_cast<ArrowArrayStream *>(snowflake_bind_data->factory.get()),
	                        snowflake_bind_data->schema_root.arrow_schema);
	vector<string> names;
	vector<LogicalType> return_types;

	ArrowTableFunction::PopulateArrowTableType(DBConfig::GetConfig(context), snowflake_bind_data->arrow_table,
	                                           snowflake_bind_data->schema_root, names, return_types);
	snowflake_bind_data->all_types = return_types;

	bind_data = std::move(snowflake_bind_data);

	return GetSnowflakeScanFunction();
}

unique_ptr<BaseStatistics> SnowflakeTableEntry::GetStatistics(ClientContext &context, column_t column_id) {
	// TODO get statistics from snowflake
	throw NotImplementedException("Snowflake does not support getting statistics for tables");
}

TableStorageInfo SnowflakeTableEntry::GetStorageInfo(ClientContext &context) {
	TableStorageInfo result;
	result.cardinality = 100000; // TODO get actual storage info from snowflake
	result.index_info = vector<IndexInfo>();
	return result;
}

void SnowflakeTableEntry::EnsureColumnsLoaded(ClientContext &context) {
	if (columns_loaded)
		return;

	auto col_info = client->GetTableInfo(context, schema.name, name);

	for (const auto &col : col_info) {
		columns.AddColumn(ColumnDefinition(col.name, col.type));
	}

	columns_loaded = true;
}

} // namespace snowflake
} // namespace duckdb