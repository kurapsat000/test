# DuckDB Snowflake Extension Development Plan

## Overview

This document outlines the development plan for creating a DuckDB extension that integrates with Snowflake using ADBC (Arrow Database Connectivity). The extension will enable DuckDB to query Snowflake tables directly, leveraging Arrow for efficient data transfer.

## Architecture

The extension follows DuckDB's table function pattern, similar to the BigQuery extension, but uses ADBC as the connectivity layer to Snowflake.

```
┌─────────────┐     ┌──────────────┐     ┌────────────┐
│   DuckDB    │────▶│  Extension   │────▶│ Snowflake  │
│   Query     │     │              │     │   ADBC     │
│   Engine    │◀────│ Arrow→DuckDB │◀────│   Driver   │
└─────────────┘     └──────────────┘     └────────────┘
```

## Development Phases

### Phase 1: Foundation Setup

#### 1.1 Rename Extension (Priority: High)
- **Files to modify:**
  - `CMakeLists.txt`: Change extension name from "quack" to "snowflake"
  - `extension_config.cmake`: Update extension loading configuration
  - `src/quack_extension.cpp` → `src/snowflake_extension.cpp`
  - `src/include/quack_extension.hpp` → `src/include/snowflake_extension.hpp`
  - Update all references in test files

#### 1.2 Add Dependencies (Priority: High)
- **Update `vcpkg.json`:**
  ```json
  {
    "dependencies": [
      "openssl",
      "arrow",
      "adbc-driver-snowflake"
    ]
  }
  ```

#### 1.3 Update Build System (Priority: High)
- **Modify `CMakeLists.txt`:**
  - Link Arrow libraries
  - Link ADBC Snowflake driver
  - Add include directories for Arrow headers

### Phase 2: Core Components

#### 2.1 Connection Management
**Files:** `src/snowflake_connection.cpp`, `src/include/snowflake_connection.hpp`

**Key Classes:**
```cpp
class SnowflakeConnection {
    AdbcDatabase database;
    AdbcConnection connection;
    
    // Methods
    void Connect(const SnowflakeConfig& config);
    void Disconnect();
    bool IsConnected();
};

struct SnowflakeConfig {
    string account;
    string username;
    string password;
    string warehouse;
    string database;
    string schema;
    string role;
    AuthType auth_type;
};
```

**Features:**
- Connection string parsing
- Multiple authentication methods
- Connection pooling
- Error handling and retry logic

#### 2.2 Table Function Implementation
**File:** `src/snowflake_scan.cpp`

**Function Signature:**
```sql
snowflake_scan(connection_string VARCHAR, query VARCHAR, [options])
```

**Implementation Structure:**
```cpp
class SnowflakeScanFunction : public TableFunction {
    // Bind: Parse parameters, establish connection, fetch schema
    static unique_ptr<FunctionData> Bind(ClientContext &context, 
                                       TableFunctionBindInput &input);
    
    // InitGlobal: Prepare ADBC statement, configure streams
    static unique_ptr<GlobalTableFunctionState> InitGlobal(
        ClientContext &context, TableFunctionInitInput &input);
    
    // InitLocal: Per-thread state initialization
    static unique_ptr<LocalTableFunctionState> InitLocal(
        ExecutionContext &context, TableFunctionInitInput &input);
    
    // Execute: Convert Arrow data to DuckDB vectors
    static void Execute(ClientContext &context, 
                       TableFunctionInput &data, DataChunk &output);
};
```

#### 2.3 Arrow to DuckDB Type Conversion
**File:** `src/snowflake_arrow_utils.cpp`

**Type Mapping Table:**
| Snowflake Type | Arrow Type | DuckDB Type |
|----------------|------------|-------------|
| NUMBER | Int64/Double | BIGINT/DOUBLE |
| VARCHAR | Utf8 | VARCHAR |
| DATE | Date32 | DATE |
| TIMESTAMP | Timestamp | TIMESTAMP |
| BOOLEAN | Bool | BOOLEAN |
| VARIANT | Utf8 | JSON |
| ARRAY | List | LIST |
| OBJECT | Struct | STRUCT |

**Conversion Functions:**
```cpp
LogicalType ArrowTypeToDuckDB(const arrow::DataType& arrow_type);
void ConvertArrowArrayToDuckDB(const arrow::Array& array, 
                               Vector& vector, idx_t size);
```

### Phase 3: Advanced Features

#### 3.1 Query Pushdown (Priority: Medium)
- Filter pushdown for WHERE clauses
- Projection pushdown for column selection
- Aggregate pushdown for COUNT, MIN, MAX
- Limit pushdown for result size optimization

#### 3.2 Parallel Scanning (Priority: Low)
- Multiple ADBC result streams
- Thread-safe data reading
- Load balancing across streams
- Progress tracking

#### 3.3 Configuration Management (Priority: Medium)
**Supported Parameters:**
- `sf_account`: Snowflake account identifier
- `sf_warehouse`: Compute warehouse
- `sf_auth_type`: Authentication method
- `sf_client_session_keep_alive`: Session timeout
- `sf_query_timeout`: Query execution timeout

### Phase 4: Testing and Documentation

#### 4.1 SQL Tests
**File:** `test/sql/snowflake.test`

**Test Categories:**
- Basic connectivity tests
- Data type conversion tests
- Query pushdown verification
- Error handling scenarios
- Performance benchmarks

#### 4.2 Documentation
- README.md with usage examples
- Authentication setup guide
- Performance tuning recommendations
- Troubleshooting guide

## Implementation Timeline

### Week 1-2: Foundation
- [ ] Extension rename and setup
- [ ] Dependency configuration
- [ ] Basic project structure

### Week 3-4: Core Connection
- [ ] ADBC connection management
- [ ] Authentication implementation
- [ ] Basic error handling

### Week 5-6: Table Function
- [ ] Bind/Init/Execute structure
- [ ] Arrow data conversion
- [ ] Basic query execution

### Week 7-8: Advanced Features
- [ ] Query pushdown
- [ ] Parallel scanning
- [ ] Performance optimization

### Week 9-10: Testing & Polish
- [ ] Comprehensive test suite
- [ ] Documentation
- [ ] Performance benchmarking

## Usage Examples

### Basic Query
```sql
-- Direct table scan
SELECT * FROM snowflake_scan(
    'account=myaccount;username=user;password=pass;warehouse=compute_wh',
    'SELECT * FROM my_schema.my_table'
);

-- With authentication token
SELECT * FROM snowflake_scan(
    'account=myaccount;auth_type=oauth;token=xxx',
    'my_database.my_schema.my_table'
);
```

### Advanced Usage
```sql
-- Create a view for convenience
CREATE VIEW snowflake_tables AS
SELECT * FROM snowflake_scan(
    'account=myaccount;warehouse=compute_wh',
    'SELECT * FROM information_schema.tables'
);

-- Query with pushdown
SELECT COUNT(*) 
FROM snowflake_scan(
    'account=myaccount;warehouse=compute_wh',
    'sales.transactions'
) 
WHERE amount > 1000;
```

## Dependencies

### Required Libraries
- **Apache Arrow** (>= 10.0.0): Data format and conversion
- **ADBC Snowflake Driver** (>= 0.7.0): Snowflake connectivity
- **DuckDB** (>= 0.9.0): Extension API
- **OpenSSL**: Secure connections

### Build Requirements
- CMake >= 3.18
- C++17 compatible compiler
- vcpkg for dependency management

## Error Handling

### Connection Errors
- Invalid credentials → Clear error message with authentication type
- Network timeout → Retry with exponential backoff
- Invalid account → Validate account format

### Query Errors
- SQL syntax errors → Pass through Snowflake error message
- Permission denied → Include required privileges in error
- Resource limits → Suggest warehouse sizing

## Performance Considerations

### Optimization Strategies
1. **Connection Pooling**: Reuse connections across queries
2. **Metadata Caching**: Cache table schemas for repeated queries
3. **Batch Size Tuning**: Optimize Arrow RecordBatch sizes
4. **Parallel Reads**: Use multiple ADBC streams for large results
5. **Query Pushdown**: Minimize data transfer by pushing filters

### Benchmarking Targets
- Connection overhead: < 100ms
- Data transfer rate: > 100MB/s
- Type conversion overhead: < 5% of transfer time
- Memory usage: < 2x data size

## Security Considerations

### Authentication Security
- Never log passwords or tokens
- Support secure token storage
- Implement token refresh for OAuth
- Clear credentials from memory after use

### Data Security
- Support encrypted connections
- Validate SSL certificates
- Implement row-level security passthrough
- Audit log integration

## Future Enhancements

### Planned Features
1. **Write Support**: INSERT/UPDATE/DELETE operations
2. **Streaming Results**: Process large results without full materialization
3. **Schema Evolution**: Handle schema changes gracefully
4. **Query Caching**: Cache frequently accessed data
5. **Materialized Views**: Local caching of Snowflake views

### Integration Opportunities
- dbt integration for transformations
- Apache Iceberg table support
- Delta Lake compatibility
- Real-time CDC (Change Data Capture)

## Contributing

### Development Setup
```bash
# Clone repository
git clone https://github.com/yourusername/duckdb-snowflake
cd duckdb-snowflake

# Initialize submodules
git submodule update --init --recursive

# Build extension
make

# Run tests
make test
```

### Code Style
- Follow DuckDB coding standards
- Use clang-format for C++ code
- Include unit tests for new features
- Document public APIs

## References

- [DuckDB Extension Template](https://github.com/duckdb/extension-template)
- [Arrow ADBC Specification](https://arrow.apache.org/adbc/)
- [Snowflake ADBC Driver](https://arrow.apache.org/adbc/current/driver/snowflake.html)
- [BigQuery Extension](https://github.com/hafenkran/duckdb-bigquery) (reference implementation)