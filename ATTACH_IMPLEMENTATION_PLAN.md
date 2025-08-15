# DuckDB Snowflake Extension - ATTACH Implementation Plan

## Overview
This document outlines the current state and implementation plan for the ATTACH command in the DuckDB Snowflake extension. The goal is to enable PostgreSQL-style database attachment, allowing users to query Snowflake tables using schema-qualified names (e.g., `sf_db.schema.table`).

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

## Implementation Plan

### Phase 1: Core Catalog Implementation (Week 1)

#### 1.1 Implement SnowflakeCatalog Class
```cpp
// Required methods to implement:
- void Initialize(bool load_builtin)
- optional_ptr<CatalogEntry> CreateEntry(...)
- optional_ptr<SchemaCatalogEntry> LookupSchema(...)
- void ScanSchemas(ClientContext &context, callback)
- DatabaseSize GetDatabaseSize(ClientContext &context)
```

#### 1.2 Implement SnowflakeSchemaEntry
```cpp
// Manages tables within a schema:
- optional_ptr<CatalogEntry> GetTable(...)
- void ScanTables(ClientContext &context, callback)
- Lazy loading of table metadata
```

#### 1.3 Create SnowflakeTransaction
```cpp
// Minimal transaction manager:
- Store reference to SnowflakeClient
- Provide client access to catalog operations
```

### Phase 2: Storage Extension Integration (Week 2)

#### 2.1 Update SnowflakeAttach Function
- Parse connection string/options
- Create and configure SnowflakeCatalog
- Return catalog instead of nullptr

#### 2.2 Implement Connection String Parsing
- Support semicolon-separated format: `account=xyz;user=abc;password=123`
- Support URI format: `snowflake://user:password@account/database/schema`
- Extract all required parameters

#### 2.3 Create Transaction Manager
- Implement `BigqueryCreateTransactionManager` equivalent
- Handle connection lifecycle

### Phase 3: Schema Discovery (Week 2-3)

#### 3.1 Implement Schema Scanning
- Use `SnowflakeClient::ListSchemas()` 
- Cache schema list for performance
- Handle authentication/permission errors

#### 3.2 Implement Table Discovery
- Use `SnowflakeClient::ListTables(schema)`
- Create table entries on demand
- Support information schema queries

#### 3.3 Create Table Views
- Generate `snowflake_scan` views for each table
- Handle table name escaping
- Support column metadata

### Phase 4: Testing & Polish (Week 3-4)

#### 4.1 Unit Tests
- Test catalog operations
- Test schema/table discovery
- Test error handling

#### 4.2 Integration Tests
- End-to-end ATTACH tests
- Cross-schema queries
- Performance testing

#### 4.3 Documentation
- Update README with ATTACH examples
- Document connection string format
- Add troubleshooting guide

## Technical Details

### Connection String Formats
```sql
-- Semicolon-separated (current)
ATTACH 'account=myaccount;user=myuser;password=mypass;warehouse=mywh;database=mydb' AS sf (TYPE snowflake);

-- URI format (to implement)
ATTACH 'snowflake://myuser:mypass@myaccount/mydb?warehouse=mywh' AS sf (TYPE snowflake);
```

### Expected Usage
```sql
-- Attach Snowflake database
ATTACH 'connection_string' AS sf_db (TYPE snowflake);

-- Query using schema-qualified names
SELECT * FROM sf_db.public.customers;
SELECT * FROM sf_db.sales.orders;

-- Join with local tables
SELECT * FROM local_table JOIN sf_db.public.products ON ...;

-- Detach when done
DETACH sf_db;
```

### Architecture Flow
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

## Success Criteria

1. **Functional Requirements**
   - ✅ ATTACH creates accessible database namespace
   - ✅ Schema-qualified queries work (sf_db.schema.table)
   - ✅ Table discovery works on demand
   - ✅ Multiple databases can be attached simultaneously
   - ✅ DETACH removes the database

2. **Performance Requirements**
   - Lazy loading of schemas/tables
   - Connection pooling works correctly
   - Metadata caching reduces API calls

3. **Compatibility**
   - Follows DuckDB extension patterns
   - Similar UX to PostgreSQL/BigQuery extensions
   - Standard SQL syntax support

## Next Steps

1. **Immediate Actions**
   - Implement `SnowflakeCatalog::Initialize()` method
   - Create basic `SnowflakeSchemaEntry` class
   - Update `SnowflakeAttach` to return catalog

2. **Follow-up Tasks**
   - Add connection string parsing
   - Implement schema discovery
   - Create comprehensive tests

3. **Future Enhancements**
   - Support for views and materialized views
   - Column statistics for query optimization
   - Pushdown predicates to Snowflake
   - Support for Snowflake-specific features (variants, arrays)

## References

- DuckDB Catalog API documentation
- BigQuery extension implementation
- PostgreSQL extension implementation
- Snowflake ADBC driver documentation