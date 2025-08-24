# ADBC Arrow Integration for DuckDB Snowflake Extension

## Overview

This document describes the zero-copy Arrow integration between Snowflake (via ADBC) and DuckDB. The implementation provides efficient data transfer from Snowflake to DuckDB without intermediate memory allocations or data conversions.

## Architecture

### Key Components

1. **SnowflakeArrowStreamFactory**
   - Holds the ADBC connection and SQL query
   - Manages the ADBC statement lifecycle
   - Serves as the factory for producing Arrow streams

2. **SnowflakeProduceArrowScan**
   - Called by DuckDB's arrow_scan to create an ArrowArrayStreamWrapper
   - Executes the Snowflake query via ADBC
   - Returns a stream that provides Arrow data chunks

3. **SnowflakeGetArrowSchema**
   - Called during bind phase to get column metadata
   - Uses ADBC's ExecuteSchema for lightweight schema retrieval
   - Provides column types without executing the full query

### Data Flow

```
Snowflake → ADBC Driver → ArrowArrayStream → DuckDB Arrow Integration → DuckDB Vectors
```

## Implementation Details

### Zero-Copy Design

The implementation achieves zero-copy data transfer by:

1. **Direct Stream Transfer**: The ADBC ArrowArrayStream is directly passed to DuckDB without intermediate conversions
2. **Ownership Transfer**: The stream ownership is transferred from ADBC to DuckDB's ArrowArrayStreamWrapper
3. **Native Arrow Format**: Data remains in Arrow columnar format throughout the pipeline

### Factory Pattern

The factory pattern is used to integrate with DuckDB's arrow_scan table function:

```cpp
struct SnowflakeArrowStreamFactory {
    std::shared_ptr<snowflake::SnowflakeClient> connection;
    std::string query;
    AdbcStatement statement;
    bool statement_initialized = false;
};
```

The factory is kept alive throughout the scan operation, ensuring the ADBC resources remain valid.

### Lazy Statement Initialization

ADBC statements are initialized lazily to:
- Avoid unnecessary resource allocation during planning
- Defer query execution until data is actually needed
- Support query optimization and pushdown in future versions

## Integration with DuckDB

### Leveraging DuckDB's Arrow Infrastructure

Instead of reimplementing Arrow-to-DuckDB conversion, the implementation:

1. **Inherits from ArrowScanFunctionData**: Reuses DuckDB's Arrow scan state management
2. **Uses ArrowTableFunction methods**: Delegates to DuckDB's proven Arrow implementation
3. **Supports pushdown**: Enables projection and filter pushdown capabilities

### Table Function Registration

```cpp
TableFunction snowflake_scan("snowflake_scan", {LogicalType::VARCHAR, LogicalType::VARCHAR}, 
                            ArrowTableFunction::ArrowScanFunction,      // DuckDB's scan
                            snowflake::SnowflakeScanBind,              // Custom bind
                            ArrowTableFunction::ArrowScanInitGlobal,    // DuckDB's init
                            ArrowTableFunction::ArrowScanInitLocal);    // DuckDB's init
```

## ADBC Usage

### Statement Lifecycle

1. **Creation**: `AdbcStatementNew` creates a statement from the connection
2. **Configuration**: `AdbcStatementSetSqlQuery` sets the SQL query
3. **Schema Retrieval**: `AdbcStatementExecuteSchema` gets column metadata
4. **Execution**: `AdbcStatementExecuteQuery` returns an ArrowArrayStream
5. **Cleanup**: `AdbcStatementRelease` releases resources

### Error Handling

ADBC errors are properly handled with:
- Error message extraction from AdbcError structure
- Proper cleanup of error resources
- Conversion to DuckDB exceptions

## Benefits

1. **Performance**: Zero-copy data transfer eliminates memory overhead
2. **Simplicity**: Minimal code by reusing DuckDB's Arrow infrastructure
3. **Compatibility**: Works with any ADBC-compliant driver
4. **Type Safety**: Arrow schema ensures type compatibility
5. **Memory Efficiency**: Streaming interface supports large result sets

## Future Enhancements

1. **Projection Pushdown**: Pass column projections to Snowflake
2. **Filter Pushdown**: Convert DuckDB filters to Snowflake predicates
3. **Parallel Scanning**: Support multiple ADBC streams for parallelism
4. **Statistics**: Retrieve row counts and statistics from Snowflake

## Example Usage

```sql
-- Query Snowflake data in DuckDB
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypass;warehouse=WH;database=DB',
    'SELECT * FROM my_table WHERE condition = true'
);

-- Join with local DuckDB tables
SELECT s.*, d.local_col
FROM snowflake_scan(connection_string, query) s
JOIN duckdb_table d ON s.id = d.id;
```

## Technical Notes

### Memory Patterns

The error `0xbebebebebebebebe` indicates uninitialized memory. This was fixed by ensuring the Arrow schema is properly stored in the bind data.

### String Handling

String data in Arrow uses a separate buffer for the actual string content. The ADBC stream must remain alive throughout the scan to ensure string pointers remain valid.

### Type Conversion

DuckDB's `PopulateArrowTableType` handles all Arrow-to-DuckDB type conversions, including:
- Numeric types
- Strings
- Dates and timestamps
- Complex types (lists, structs)

## Debugging

To debug Arrow integration issues:

1. Check schema initialization in bind phase
2. Verify stream lifetime management
3. Use AddressSanitizer to catch memory errors
4. Enable DuckDB's Arrow verification with debug builds

## References

- [Arrow Columnar Format](https://arrow.apache.org/docs/format/Columnar.html)
- [ADBC Specification](https://arrow.apache.org/adbc/)
- [DuckDB Arrow Integration](https://duckdb.org/docs/api/arrow)