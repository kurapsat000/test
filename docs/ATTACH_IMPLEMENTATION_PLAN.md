# DuckDB Snowflake Extension - ATTACH Implementation Plan

## Overview

This document outlines the current state and implementation plan for the ATTACH command in the DuckDB Snowflake extension. The goal is to enable PostgreSQL-style database attachment, allowing users to query Snowflake tables using schema-qualified names (e.g., `sf_db.schema.table`).

**Target Syntax:**
```sql
ATTACH DATABASE 'snowflake://account=X;user=Y;password=Z;warehouse=W;database=D' AS sf_db;
SELECT * FROM sf_db.schema.table_name;
DETACH DATABASE sf_db;
```

## Current State

### ✅ Completed Components

1. **Client Infrastructure**
   - `SnowflakeClient`: ADBC-based connection wrapper with methods for:
     - `ListSchemas()` - Retrieves available schemas
     - `ListTables()` - Lists tables in a schema
     - `GetTableInfo()` - Gets table metadata
   - `SnowflakeClientManager`: Singleton for connection pooling
   - `SnowflakeConfig`: Configuration parsing and validation

2. **Basic Table Function**
   - `snowflake_attach()` table function works but creates views in main namespace
   - `snowflake_scan()` successfully queries Snowflake data
   - Arrow-to-DuckDB data conversion pipeline

3. **Storage Extension Registration**
   - `SnowflakeStorageExtension` registered with DuckDB
   - Basic structure in place but implementation incomplete

### 🚧 In Progress Components

1. **Catalog Framework**
   - Header files created but no implementation:
     - `snowflake_catalog.hpp`
     - `snowflake_catalog_set.hpp` 
     - `snowflake_schema_entry.hpp`
   - `SnowflakeAttach` function returns `nullptr` instead of catalog

### ❌ Missing Components

1. **Catalog Implementation**
   - No virtual method implementations for `SnowflakeCatalog`
   - Missing schema and table entry management
   - No integration with DuckDB's catalog system

2. **Transaction Manager**
   - Required for catalog operation
   - Can be minimal for read-only access

3. **Connection String Parsing**
   - No support for `snowflake://` URI format
   - Need to parse ATTACH parameters

## Phase 1: Understand DuckDB Storage Extension Architecture

### 1.1 Research Tasks
- **Study existing storage extensions** in DuckDB core (SQLite, Postgres if available)
- **Analyze DuckDB's catalog system** - how schemas and tables are organized
- **Understand AttachedDatabase lifecycle** - creation, usage, cleanup
- **Study connection string parsing** - how DuckDB handles different URI schemes

### 1.2 Key Components to Research
```cpp
// Core interfaces to understand
class Catalog;                    // Schema and table management
class AttachedDatabase;           // Database instance management  
class TransactionManager;        // Transaction handling
struct AttachInfo;               // Attach parameters
class DatabaseManager;           // Multi-database coordination
```

## Phase 2: Design Snowflake Catalog Implementation

### 2.1 Snowflake Catalog Class
**File:** `src/include/snowflake_catalog.hpp`

```cpp
class SnowflakeCatalog : public Catalog {
private:
    string connection_string;
    shared_ptr<SnowflakeConnection> sf_connection;
    SnowflakeConfig config;
    
public:
    // Schema and table discovery
    vector<string> GetSchemaNames();
    vector<string> GetTableNames(const string &schema);
    
    // Catalog entry creation (lazy loading)
    unique_ptr<CatalogEntry> CreateTable(const string &schema, const string &table);
    unique_ptr<CatalogEntry> CreateSchema(const string &schema);
    
    // DuckDB Catalog interface implementation
    optional_ptr<CatalogEntry> GetEntry(ClientContext &context, 
                                       CatalogType type,
                                       const string &schema,
                                       const string &name,
                                       bool if_not_found = false,
                                       QueryErrorContext error_context = QueryErrorContext()) override;
                                       
    void ScanEntries(ClientContext &context,
                     CatalogType type,
                     const std::function<void(CatalogEntry &)> &callback) override;
};
```

### 2.2 Snowflake Table Entry Class
**File:** `src/include/snowflake_table_entry.hpp`

```cpp
class SnowflakeTableEntry : public TableCatalogEntry {
private:
    string connection_string;
    string qualified_table_name;  // schema.table
    
public:
    // Table function binding - returns snowflake_scan equivalent
    unique_ptr<BaseTableRef> GetTableRef(ClientContext &context) override;
    
    // Column information (cached from Snowflake DESCRIBE)
    vector<ColumnDefinition> GetColumns();
};
```

## Phase 3: Implement Core Components

### 3.1 Connection String Handling
**File:** `src/snowflake_uri.cpp`

```cpp
struct SnowflakeURI {
    static bool IsSnowflakeURI(const string &uri);
    static SnowflakeConfig ParseURI(const string &uri);
    
    // Support formats:
    // snowflake://account=X;user=Y;password=Z;warehouse=W;database=D
    // snowflake://user:password@account/database?warehouse=W&schema=S
};
```

### 3.2 Storage Extension Implementation
**File:** `src/snowflake_storage.cpp`

```cpp
unique_ptr<Catalog> SnowflakeStorageExtension::Attach(
    StorageExtensionInfo *storage_info,
    ClientContext &context,
    AttachedDatabase &db,
    const string &name,
    AttachInfo &info,
    AccessMode access_mode) {
    
    // 1. Parse Snowflake URI from info.path
    if (!SnowflakeURI::IsSnowflakeURI(info.path)) {
        throw BinderException("Invalid Snowflake URI: %s", info.path);
    }
    
    // 2. Create and test connection
    auto config = SnowflakeURI::ParseURI(info.path);
    auto connection = SnowflakeConnectionManager::GetConnection(info.path, config);
    
    // 3. Create and return Snowflake catalog
    return make_uniq<SnowflakeCatalog>(info.path, config, connection);
}
```

### 3.3 Transaction Manager (Minimal Implementation)
**File:** `src/snowflake_transaction.cpp`

```cpp
class SnowflakeTransactionManager : public TransactionManager {
public:
    // Snowflake doesn't need local transactions
    // All operations are pass-through to Snowflake
    Transaction &StartTransaction(ClientContext &context) override;
    void CommitTransaction(ClientContext &context, Transaction &transaction) override;
    void RollbackTransaction(Transaction &transaction) override;
};
```

## Phase 4: Schema and Table Discovery

### 4.1 Schema Discovery
**Method:** `SnowflakeCatalog::GetSchemaNames()`

```sql
-- Query to run on Snowflake
SELECT schema_name 
FROM information_schema.schemata 
WHERE catalog_name = CURRENT_DATABASE()
ORDER BY schema_name;
```

### 4.2 Table Discovery
**Method:** `SnowflakeCatalog::GetTableNames(schema)`

```sql
-- Query to run on Snowflake  
SELECT table_name
FROM information_schema.tables
WHERE table_schema = ? AND table_catalog = CURRENT_DATABASE()
ORDER BY table_name;
```

### 4.3 Column Discovery
**Method:** `SnowflakeTableEntry::GetColumns()`

```sql
-- Query to run on Snowflake
SELECT column_name, data_type, is_nullable
FROM information_schema.columns  
WHERE table_schema = ? AND table_name = ? AND table_catalog = CURRENT_DATABASE()
ORDER BY ordinal_position;
```

## Phase 5: Table Function Integration

### 5.1 Dynamic Table Function Creation
**File:** `src/snowflake_table_function.cpp`

```cpp
class SnowflakeTableFunction : public TableFunction {
public:
    SnowflakeTableFunction(const string &connection_string,
                          const string &qualified_table_name);
    
    // Bind to snowflake_scan internally
    static unique_ptr<FunctionData> Bind(ClientContext &context,
                                        TableFunctionBindInput &input,
                                        vector<LogicalType> &return_types,
                                        vector<string> &names);
                                        
    // Execute using existing snowflake_scan logic                                  
    static void Execute(ClientContext &context,
                       TableFunctionInput &data,
                       DataChunk &output);
};
```

### 5.2 Table Reference Implementation
**Method:** `SnowflakeTableEntry::GetTableRef()`

```cpp
unique_ptr<BaseTableRef> SnowflakeTableEntry::GetTableRef(ClientContext &context) {
    // Create table function reference that uses snowflake_scan
    auto table_function = make_uniq<SnowflakeTableFunction>(connection_string, qualified_table_name);
    
    vector<Value> parameters;
    parameters.emplace_back(connection_string);
    parameters.emplace_back(StringUtil::Format("SELECT * FROM %s", qualified_table_name));
    
    auto function_ref = make_uniq<TableFunctionRef>();
    function_ref->function = make_uniq<FunctionExpression>("snowflake_scan", std::move(parameters));
    
    return std::move(function_ref);
}
```

## Phase 6: Catalog Integration

### 6.1 Lazy Loading Strategy
- **Schemas:** Load all schema names on catalog creation
- **Tables:** Load table names when schema is first accessed  
- **Columns:** Load column info when table is first queried

### 6.2 Caching Strategy
```cpp
class SnowflakeCatalog : public Catalog {
private:
    // Cache structures
    unordered_map<string, vector<string>> schema_tables_cache;
    unordered_map<string, vector<ColumnDefinition>> table_columns_cache;
    time_t last_cache_refresh;
    
    // Cache management
    void RefreshCacheIfNeeded();
    bool IsCacheValid();
};
```

## Phase 7: Error Handling and Edge Cases

### 7.1 Connection Failures
- **Graceful degradation** when Snowflake is unavailable
- **Retry logic** for transient connection issues
- **Clear error messages** for authentication failures

### 7.2 Schema/Table Not Found
- **Proper CatalogException** when schema/table doesn't exist
- **Case sensitivity handling** (Snowflake is case-sensitive for quoted identifiers)

### 7.3 Connection String Validation
- **Validate required parameters** (account, user)
- **Support multiple authentication methods** (password, OAuth, key-pair)
- **Clear error messages** for missing/invalid parameters

## Phase 8: Testing Strategy

### 8.1 Unit Tests
**File:** `test/unittest/test_snowflake_attach.cpp`

```cpp
// Test cases:
- URI parsing (valid/invalid formats)
- Connection string validation
- Catalog creation and schema discovery
- Table entry creation
- Error handling scenarios
```

### 8.2 Integration Tests  
**File:** `test/sql/attach_integration.test`

```sql
# Test PostgreSQL-style attach
ATTACH DATABASE 'snowflake://account=test;user=test;password=test;warehouse=WH;database=DB' AS sf;

# Test schema access
SELECT * FROM sf.information_schema.tables LIMIT 1;

# Test table access  
SELECT * FROM sf.public.my_table LIMIT 5;

# Test detach
DETACH DATABASE sf;
```

### 8.3 Mock Testing Infrastructure
**File:** `test/mock/mock_snowflake_connection.cpp`

```cpp
// Mock Snowflake connection for testing without real credentials
class MockSnowflakeConnection : public SnowflakeConnection {
    // Return predefined schema/table/column information
    // Simulate connection failures
    // Test various authentication scenarios
};
```

## Phase 9: Documentation and Examples

### 9.1 Usage Documentation
```sql
-- Basic attach
ATTACH DATABASE 'snowflake://account=myaccount;user=myuser;password=mypass;warehouse=COMPUTE_WH;database=SALES_DB' AS sales;

-- Query with schema qualification  
SELECT COUNT(*) FROM sales.public.customers;
SELECT * FROM sales.analytics.daily_sales WHERE date >= '2024-01-01';

-- Join across databases
SELECT c.name, s.total_amount
FROM sales.public.customers c
JOIN sales.public.orders s ON c.id = s.customer_id;

-- Detach when done
DETACH DATABASE sales;
```

### 9.2 Migration Guide
- **From table function to storage extension**
- **Breaking changes and compatibility**
- **Performance considerations**

## Phase 10: Implementation Order

### Priority 1 (Core Functionality)
1. **SnowflakeURI** class for connection string parsing
2. **SnowflakeCatalog** basic implementation
3. **Storage extension** attach function
4. **Basic schema discovery**

### Priority 2 (Table Access)
5. **SnowflakeTableEntry** implementation
6. **Table function integration**
7. **Column discovery and type mapping**
8. **Basic query functionality**

### Priority 3 (Robustness)
9. **Error handling and validation**
10. **Caching and performance optimization**
11. **Transaction manager implementation**
12. **Comprehensive testing**

### Priority 4 (Polish)
13. **Documentation and examples**
14. **Migration utilities**
15. **Performance benchmarking**

## Success Criteria

### Functional Requirements
- ✅ Standard `ATTACH DATABASE` syntax works
- ✅ Schema-qualified table access (`db.schema.table`)
- ✅ All existing `snowflake_scan` functionality preserved
- ✅ Proper `DETACH DATABASE` cleanup
- ✅ Multiple attached databases supported

### Performance Requirements
- ✅ Schema discovery completes within 5 seconds
- ✅ First table access within 2 seconds
- ✅ Subsequent queries match `snowflake_scan` performance
- ✅ Memory usage scales linearly with attached databases

### Compatibility Requirements
- ✅ Backward compatibility with existing `snowflake_scan`
- ✅ Standard DuckDB attached database semantics
- ✅ PostgreSQL-style syntax and behavior
- ✅ Works with all Snowflake authentication methods

## Current Implementation Analysis

### What Exists Today

#### Table Function Approach (`snowflake_attach`)
```sql
SELECT * FROM snowflake_attach('connection_string');
-- Creates views in main namespace: SELECT * FROM TABLE_NAME;
```

**Implementation:**
- `AttachBind()` - parses connection string
- `AttachFunction()` - lists tables and creates views using `snowflake_scan`
- Views created in main DuckDB namespace (no schema separation)

#### Storage Extension (Incomplete)
```sql
ATTACH DATABASE 'snowflake://connection_string' AS db_name;  -- Should work but doesn't
```

**Current Issues:**
- Storage extension just delegates to table function
- Returns `nullptr` for catalog (no proper database attachment)
- No schema-qualified access support
- Missing transaction manager

### Expected Architecture Flow
```
ATTACH Command
    ↓
SnowflakeStorageExtension::Attach
    ↓
Parse Connection String → Create SnowflakeClient
    ↓
Create SnowflakeCatalog → Initialize with Client
    ↓
Return Catalog to DuckDB
    ↓
DuckDB registers sf_db.* namespace
    ↓
User queries sf_db.schema.table
    ↓
SnowflakeCatalog::GetEntry → Create Table View
    ↓
Execute snowflake_scan for data access
```

### Migration Strategy

1. **Keep existing table function** for backward compatibility
2. **Implement proper storage extension** alongside existing functionality
3. **Deprecate table function** in documentation but keep functional
4. **Provide migration tools** and clear upgrade path

## Next Steps

### Immediate Actions
1. Implement `SnowflakeCatalog::Initialize()` method
2. Create basic `SnowflakeSchemaEntry` class
3. Update `SnowflakeAttach` to return catalog

### Follow-up Tasks
1. Add connection string parsing
2. Implement schema discovery
3. Create comprehensive tests

### Future Enhancements
1. Support for views and materialized views
2. Column statistics for query optimization
3. Pushdown predicates to Snowflake
4. Support for Snowflake-specific features (variants, arrays)

## References

- DuckDB Catalog API documentation
- BigQuery extension implementation
- PostgreSQL extension implementation
- Snowflake ADBC driver documentation

This plan transforms the Snowflake extension from a simple table function into a full-featured database attachment system that follows PostgreSQL conventions and integrates seamlessly with DuckDB's multi-database architecture.