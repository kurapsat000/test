# Pushdown Implementation Plan for DuckDB-Snowflake Extension

## Overview

This document outlines the plan for implementing query pushdown optimization in the DuckDB-Snowflake extension. Pushdown optimization can significantly improve query performance by executing filters and projections directly in Snowflake rather than transferring all data to DuckDB for processing.

## Key Insight: Direct SQL Pass-through vs. Pushdown

Before diving into complex pushdown implementation, it's important to understand that there are two fundamental approaches to querying Snowflake data from DuckDB:

### 1. Direct SQL Pass-through (Simpler, Often Better)
```sql
-- User writes complete SQL that executes entirely in Snowflake
SELECT C_NAME FROM CUSTOMER WHERE C_CUSTKEY > 100

-- Pass directly to Snowflake via:
snowflake_query('connection', 'SELECT C_NAME FROM CUSTOMER WHERE C_CUSTKEY > 100')
```

**Advantages:**
- Snowflake handles ALL optimization (indexes, partitions, clustering)
- Snowflake's world-class query optimizer does the work
- No complex pushdown logic needed
- Supports all Snowflake SQL features
- Maximum performance for Snowflake-only queries

### 2. Table Scan with Pushdown (Complex, but Flexible)
```sql
-- User writes query that DuckDB processes
SELECT s.C_NAME, d.local_col
FROM snowflake_scan('connection', 'SELECT * FROM CUSTOMER') s
JOIN duckdb_local_table d ON s.id = d.id
WHERE s.C_CUSTKEY > 100  -- Can be pushed down
```

**When Needed:**
- Mixed queries joining Snowflake and DuckDB tables
- Applying DuckDB-specific functions
- Complex query plans with multiple data sources

## Recommendation: Hybrid Approach

Implement both modes:

1. **`snowflake_query()`** - Direct SQL pass-through for Snowflake-only queries
2. **`snowflake_scan()`** - Table function with pushdown for mixed queries

The rest of this document focuses on implementing pushdown for the `snowflake_scan()` case, but remember: **for pure Snowflake queries, direct pass-through is simpler and often faster!**

## Background: What is Pushdown?

**Query pushdown** is an optimization technique where query operations are "pushed down" to the data source rather than being executed in the query engine. This reduces:
- Network traffic (less data transferred)
- Memory usage (fewer rows/columns loaded)
- CPU usage (filtering happens at source)
- Overall query latency

### Types of Pushdown

1. **Projection Pushdown**: Only retrieve needed columns
   ```sql
   -- User query
   SELECT name, age FROM users
   
   -- Without pushdown: SELECT * FROM users (then project locally)
   -- With pushdown: SELECT name, age FROM users
   ```

2. **Filter/Predicate Pushdown**: Apply WHERE clauses at source
   ```sql
   -- User query
   SELECT * FROM orders WHERE amount > 1000
   
   -- Without pushdown: SELECT * FROM orders (then filter locally)
   -- With pushdown: SELECT * FROM orders WHERE amount > 1000
   ```

3. **Limit Pushdown**: Limit rows at source
   ```sql
   -- User query
   SELECT * FROM logs LIMIT 100
   
   -- Without pushdown: SELECT * FROM logs (then limit locally)
   -- With pushdown: SELECT * FROM logs LIMIT 100
   ```

4. **Aggregate Pushdown**: Compute aggregates at source (advanced)
   ```sql
   -- User query
   SELECT COUNT(*), SUM(amount) FROM sales
   
   -- Without pushdown: SELECT * FROM sales (then aggregate locally)
   -- With pushdown: SELECT COUNT(*), SUM(amount) FROM sales
   ```

## Current Architecture

### Snowflake's Native Arrow Support

**Important: Snowflake has native server-side Arrow format support!** This is a key performance advantage:

1. **Server-Side Arrow Generation**: When using ADBC, Snowflake produces Arrow IPC format directly in the warehouse
2. **Efficient Data Transfer**: Data is streamed as compressed Arrow batches over the network
3. **Zero-Copy Integration**: The Arrow format flows from Snowflake → ADBC → DuckDB without format conversion

### Data Flow Without Pushdown
```
┌─────────────┐     ┌──────────┐     ┌──────────┐     ┌─────────────────┐
│   DuckDB    │────▶│  Arrow   │────▶│   ADBC   │────▶│   Snowflake     │
│             │     │  Scan    │     │  Driver  │     │   Warehouse     │
│ SELECT name │     │          │     │          │     │ ┌─────────────┐ │
│ FROM users  │     │ Get all  │     │ Execute  │     │ │ SELECT *    │ │
│ WHERE id>10 │◀────│ columns  │◀────│   SQL    │◀────│ │ FROM users  │ │
│             │     │          │     │          │     │ └─────────────┘ │
│ Filter &    │     │          │     │ Receives │     │ Native Arrow    │
│ Project     │     │          │     │  Arrow   │◀────│ IPC Format      │
│ locally     │     │          │     │  Stream  │     │ Generation      │
└─────────────┘     └──────────┘     └──────────┘     └─────────────────┘
                                           ↑
                                    Zero-copy Arrow
                                    all the way!
```

### How Arrow Conversion Works

The Arrow format conversion happens as follows:

1. **Server-Side (Snowflake Warehouse)**:
   - Snowflake natively produces Arrow IPC format when `QueryArrowStream()` is called
   - The warehouse converts query results directly to columnar Arrow format
   - Results are compressed and streamed as Arrow record batches

2. **Client-Side (ADBC Driver)**:
   - Receives compressed Arrow IPC streams
   - Performs minimal processing:
     - Decompression of Arrow batches
     - Type adjustments based on settings (e.g., `use_high_precision`)
     - Timezone conversions for timestamps
   - Passes Arrow streams directly to consumers

3. **DuckDB Integration**:
   - Receives Arrow streams without any format conversion
   - Zero-copy data access through Arrow arrays
   - Native columnar processing

### ADBC Driver Characteristics

The Apache Arrow ADBC Snowflake driver:
- **Pass-through SQL execution**: Takes SQL queries as strings and passes them to Snowflake
- **Arrow-native data retrieval**: Uses `QueryArrowStream()` to get Arrow-formatted results
- **Configurable type mapping**: Supports options like `use_high_precision` for type conversion
- **Streaming support**: Handles large result sets efficiently through Arrow streaming

**Key Limitations**:
1. ADBC cannot modify queries for optimization (it's a pass-through driver)
2. All pushdown logic must be implemented in our extension
3. We must construct the optimized SQL before passing to ADBC

### Snowflake's Interface

**Critical Points**:

1. **Input**: Snowflake only accepts SQL queries as strings
   - No Arrow predicate pushdown API
   - No native filter expression support
   - All optimizations must be expressed as SQL

2. **Output**: Snowflake returns results in native Arrow format
   - Server-side Arrow generation for efficiency
   - Compressed Arrow IPC streams
   - Direct columnar format without row-based intermediate representation

3. **Performance Benefits**:
   - **10-20x faster** than traditional JDBC/ODBC for large result sets
   - No client-side row-to-columnar conversion
   - Efficient compression and streaming
   - Reduced CPU usage on both server and client

This architecture means pushdown to Snowflake requires:
1. Converting DuckDB's pushdown parameters → SQL query modifications
2. Sending the modified SQL string to Snowflake
3. Snowflake executing the SQL with its optimizer
4. Receiving Arrow-formatted results back (already in optimal format)

### Current Implementation Gaps

1. **ArrowStreamParameters Not Used**
   ```cpp
   // Current: Parameters are ignored
   unique_ptr<ArrowArrayStreamWrapper> SnowflakeProduceArrowScan(
       uintptr_t factory_ptr,
       ArrowStreamParameters &parameters) {  // <- Not used!
       // Just executes the original query
   }
   ```

2. **Fixed SQL Query**
   - Query is set during bind phase
   - No modification based on runtime parameters
   - Always retrieves all columns and rows

## Proposed Architecture

### Data Flow With Pushdown
```
┌─────────────┐     ┌──────────┐     ┌──────────┐     ┌─────────────────┐
│   DuckDB    │────▶│  Arrow   │────▶│   ADBC   │────▶│   Snowflake     │
│             │     │  Scan    │     │  Driver  │     │   Warehouse     │
│ SELECT name │     │ Pushdown │     │          │     │ ┌─────────────┐ │
│ FROM users  │     │ params:  │     │ Modified │     │ │ SELECT name │ │
│ WHERE id>10 │     │ -columns │────▶│   SQL    │────▶│ │ FROM users  │ │
│             │     │ -filters │     │          │     │ │ WHERE id>10 │ │
│             │◀────│          │◀────│ Receives │◀────│ └─────────────┘ │
│ (Receives   │     │          │     │  Arrow   │     │                 │
│ only needed │     │          │     │  Stream  │     │ Optimized Arrow │
│ data)       │     │          │     │          │     │ Generation:     │
└─────────────┘     └──────────┘     └──────────┘     │ - Only 1 column │
                                           ↑            │ - Filtered rows │
                                    Zero-copy Arrow     └─────────────────┘
                                    with minimal data!
```

**Key Improvements with Pushdown**:
1. **Reduced Data Generation**: Snowflake only generates Arrow data for needed columns/rows
2. **Smaller Network Transfer**: Less data sent over the network
3. **Optimized Execution**: Snowflake's optimizer uses indexes, clustering, and partitions
4. **Still Zero-Copy**: Arrow format maintained throughout the pipeline

## Implementation Steps

### Phase 1: SQL Query Builder

Create a SQL query modification system:

```cpp
class SnowflakeQueryBuilder {
    std::string base_query;
    
public:
    SnowflakeQueryBuilder(const std::string& query) : base_query(query) {}
    
    // Add projection (column selection)
    std::string ApplyProjection(const ArrowProjectedColumns& columns);
    
    // Add filters (WHERE clause)
    std::string ApplyFilters(const TableFilterSet* filters);
    
    // Add limit
    std::string ApplyLimit(idx_t limit);
};
```

### Phase 2: Filter Translation

Translate DuckDB filters to SQL predicates:

```cpp
std::string TranslateFilter(const TableFilter& filter, const std::string& column_name) {
    switch (filter.filter_type) {
        case TableFilterType::CONSTANT_COMPARISON:
            auto& comp = (ConstantFilter&)filter;
            switch (comp.comparison_type) {
                case ExpressionType::COMPARE_EQUAL:
                    return column_name + " = " + ValueToSQL(comp.constant);
                case ExpressionType::COMPARE_GREATERTHAN:
                    return column_name + " > " + ValueToSQL(comp.constant);
                // ... other comparisons
            }
        // ... other filter types
    }
}
```

### Phase 3: Modify SnowflakeProduceArrowScan

```cpp
unique_ptr<ArrowArrayStreamWrapper> SnowflakeProduceArrowScan(
    uintptr_t factory_ptr,
    ArrowStreamParameters &parameters) {
    
    auto factory = reinterpret_cast<SnowflakeArrowStreamFactory*>(factory_ptr);
    
    // Build modified query based on pushdown parameters
    SnowflakeQueryBuilder builder(factory->query);
    std::string modified_query = factory->query;
    
    // Apply projection pushdown
    if (parameters.projected_columns.columns.size() > 0) {
        modified_query = builder.ApplyProjection(parameters.projected_columns);
    }
    
    // Apply filter pushdown
    if (parameters.filters && !parameters.filters->filters.empty()) {
        modified_query = builder.ApplyFilters(parameters.filters);
    }
    
    // Execute modified query
    AdbcStatementSetSqlQuery(&factory->statement, modified_query.c_str(), &error);
    // ... rest of execution
}
```

### Phase 4: Type Compatibility

Ensure pushed-down operations work with Snowflake types:

```cpp
bool CanPushdownFilter(const TableFilter& filter, const LogicalType& type) {
    // Check if this filter type can be pushed for this column type
    if (type.id() == LogicalTypeId::DECIMAL) {
        // Special handling for DECIMAL(38,0)
        return true; // Snowflake can handle decimal comparisons
    }
    return ArrowTableFunction::ArrowPushdownType(type);
}
```

## Benefits

### Arrow Format Performance Advantages

Before discussing pushdown benefits, it's important to understand the baseline performance advantages we already get from Snowflake's native Arrow support:

1. **Server-Side Columnar Generation**
   - Snowflake generates Arrow format directly, no row-to-columnar conversion
   - 10-20x faster than traditional JDBC/ODBC for analytical queries
   - Efficient compression of columnar data

2. **Zero-Copy Data Pipeline**
   - Arrow format from Snowflake → ADBC → DuckDB without conversion
   - No serialization/deserialization overhead
   - Direct memory mapping of Arrow buffers

3. **Type-Aware Optimizations**
   - Configurable type mapping (e.g., `use_high_precision=false` for INT64)
   - Native support for complex types (arrays, structs)
   - Efficient null handling with Arrow's validity bitmaps

### Performance Improvements with Pushdown

1. **Reduced Network Traffic**
   - Only requested columns transferred
   - Only matching rows transferred
   - Example: `SELECT name FROM users WHERE active = true`
     - Without pushdown: Transfer all columns, all rows
     - With pushdown: Transfer only 'name' column for active users

2. **Reduced Memory Usage**
   - DuckDB doesn't need to buffer unwanted data
   - Arrow buffers are smaller
   - Less memory pressure on client

3. **Improved Query Speed**
   - Snowflake's distributed processing for filtering
   - Indexes and statistics used at source
   - Parallel filtering in Snowflake cluster

### Example Performance Impact

```sql
-- Query: Get names of California customers
SELECT C_NAME 
FROM CUSTOMER 
WHERE C_STATE = 'CA' 
LIMIT 1000

-- Without pushdown:
-- Transfer: 1,500,000 rows × 8 columns = ~1GB data
-- Filter locally: 1,500,000 → 50,000 rows
-- Project locally: 8 → 1 column
-- Time: ~10 seconds

-- With pushdown:
-- Transfer: 1,000 rows × 1 column = ~10KB data  
-- Snowflake does all filtering/projection
-- Time: ~0.5 seconds (20x faster!)
```

## Challenges and Considerations

### 1. SQL Query Parsing
- Need to parse user's original query
- Identify table references and aliases
- Handle complex queries (subqueries, joins)

### 2. Filter Complexity
- Simple comparisons are easy
- Complex expressions need translation
- Some filters may not be pushable

### 3. Type Mismatches
- DECIMAL(38,0) handling
- NULL semantics differences
- String collation differences

### 4. Security Considerations
- SQL injection prevention
- Proper escaping of values
- Query validation

## Testing Strategy

### Unit Tests
```cpp
TEST_CASE("Projection pushdown", "[snowflake]") {
    // Test column selection is pushed down
    auto query = "SELECT * FROM customer";
    ArrowProjectedColumns proj = {{"C_NAME", "C_CITY"}};
    auto result = BuildQueryWithProjection(query, proj);
    REQUIRE(result == "SELECT C_NAME, C_CITY FROM customer");
}
```

### Integration Tests
- Test with real Snowflake connection
- Verify result correctness
- Measure performance improvements

### Edge Cases
- Empty projections
- Invalid column names
- Complex filter expressions
- Mixed pushable/non-pushable filters

## Future Enhancements

### 1. Aggregate Pushdown
Push aggregations to Snowflake:
```sql
SELECT COUNT(*), SUM(amount) FROM orders
```

### 2. Join Pushdown
Execute joins in Snowflake when multiple Snowflake tables are involved

### 3. Function Pushdown
Push compatible functions to Snowflake:
```sql
SELECT UPPER(name), YEAR(date) FROM users
```

### 4. Partition Pruning
Use Snowflake's partition metadata to skip irrelevant partitions

## Choosing the Right Approach

### Use Direct SQL Pass-through When:
1. **Entire query executes in Snowflake**
   ```sql
   -- Simple aggregation
   SELECT COUNT(*), SUM(amount) FROM snowflake_sales WHERE year = 2023
   
   -- Complex Snowflake-specific features
   SELECT * FROM TABLE(FLATTEN(input => parse_json(json_column)))
   ```

2. **Using Snowflake-specific SQL features**
   - FLATTEN for JSON
   - MATCH_RECOGNIZE for pattern matching
   - Snowflake-specific functions

3. **Maximum performance is critical**
   - Let Snowflake's optimizer handle everything
   - Leverage Snowflake's clustering, micro-partitions

### Use Table Scan with Pushdown When:
1. **Joining with DuckDB tables**
   ```sql
   SELECT s.*, d.enrichment_data
   FROM snowflake_scan(...) s
   JOIN duckdb_enrichment d ON s.id = d.id
   ```

2. **Applying DuckDB-specific processing**
   ```sql
   SELECT snowflake_col, duckdb_special_function(snowflake_col)
   FROM snowflake_scan(...)
   ```

3. **Part of larger DuckDB pipeline**
   - Multiple data sources
   - Complex transformations
   - DuckDB-specific optimizations

## Implementation Priority

1. **Phase 1**: Implement `snowflake_query()` for direct pass-through (simpler, immediate value)
2. **Phase 2**: Enhance `snowflake_scan()` with basic pushdown (projection, simple filters)
3. **Phase 3**: Advanced pushdown features (complex predicates, aggregates)

## Conclusion

### Key Architectural Insights

1. **Snowflake's Native Arrow Support** provides massive performance benefits out-of-the-box:
   - Server-side Arrow generation eliminates format conversion overhead
   - Zero-copy pipeline from Snowflake to DuckDB
   - 10-20x performance improvement over traditional drivers

2. **Two Complementary Approaches** for different use cases:
   - **Direct SQL pass-through** (`snowflake_query`) for pure Snowflake queries
   - **Pushdown optimization** (`snowflake_scan`) for mixed DuckDB/Snowflake queries

3. **Implementation Priorities**:
   - Phase 1: Leverage existing Arrow benefits with direct pass-through
   - Phase 2: Add pushdown for mixed workloads
   - Phase 3: Advanced optimizations (aggregates, joins)

### The Power of Arrow

The combination of Snowflake's native Arrow support and DuckDB's Arrow integration creates an exceptionally efficient data pipeline:

```
Traditional Pipeline:
Snowflake (rows) → JDBC/ODBC → Client (convert to columnar) → DuckDB
  └─ Slow: format conversion at every step

Arrow Pipeline:
Snowflake (Arrow) → ADBC → DuckDB (Arrow)
  └─ Fast: zero-copy columnar data throughout
```

### Final Recommendations

1. **Start Simple**: Use direct SQL pass-through for immediate benefits
2. **Leverage Arrow**: The native Arrow support already provides huge performance gains
3. **Add Pushdown Strategically**: Focus on mixed workloads where it adds real value
4. **Measure Performance**: The Arrow pipeline may already meet your performance needs

Remember: **for pure Snowflake queries, direct SQL pass-through with Arrow is often the optimal solution**. The pushdown implementation adds value primarily for scenarios involving mixed data sources or DuckDB-specific processing.