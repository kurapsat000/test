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

### Data Flow Without Pushdown
```
┌─────────────┐     ┌──────────┐     ┌──────────┐     ┌───────────┐
│   DuckDB    │────▶│  Arrow   │────▶│   ADBC   │────▶│ Snowflake │
│             │     │  Scan    │     │  Driver  │     │           │
│ SELECT name │     │          │     │          │     │ SELECT *  │
│ FROM users  │     │ Get all  │     │ Execute  │     │ FROM users│
│ WHERE id>10 │◀────│ columns  │◀────│   SQL    │◀────│           │
│             │     │          │     │          │     │ (ALL DATA)│
│ Filter &    │     │          │     │          │     │           │
│ Project     │     │          │     │          │     │           │
│ locally     │     │          │     │          │     │           │
└─────────────┘     └──────────┘     └──────────┘     └───────────┘
```

### ADBC Driver Limitations

The Apache Arrow ADBC Snowflake driver is a **pass-through driver** that:
- Takes SQL queries as strings
- Passes them directly to Snowflake without modification
- Returns Arrow data streams
- **Does NOT support query modification or pushdown**

This means:
1. ADBC cannot modify queries for optimization
2. All pushdown logic must be implemented in our extension
3. We must construct the optimized SQL before passing to ADBC

### Snowflake's Interface Limitations

**Critical Point: Snowflake only accepts SQL queries, not Arrow predicates or pushdown expressions.**

Unlike some modern data systems that have Arrow-native APIs, Snowflake:
- **Only accepts SQL strings** as input
- **Cannot process Arrow filter expressions** directly
- **Has no Arrow-native query interface**
- Returns results in Arrow format (via ADBC) but doesn't accept Arrow query specifications

This is why pushdown to Snowflake always requires:
1. Converting Arrow pushdown parameters → SQL query modifications
2. Sending the modified SQL string to Snowflake
3. Snowflake executing the SQL (with its own optimizations)
4. Receiving Arrow-formatted results back

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
┌─────────────┐     ┌──────────┐     ┌──────────┐     ┌───────────┐
│   DuckDB    │────▶│  Arrow   │────▶│   ADBC   │────▶│ Snowflake │
│             │     │  Scan    │     │  Driver  │     │           │
│ SELECT name │     │ Pushdown │     │          │     │ SELECT    │
│ FROM users  │     │ params:  │     │ Modified │     │   name    │
│ WHERE id>10 │     │ -columns │────▶│   SQL    │────▶│ FROM users│
│             │     │ -filters │     │          │     │ WHERE     │
│             │◀────│          │◀────│          │◀────│   id > 10 │
│ (Receives   │     │          │     │          │     │           │
│ only needed │     │          │     │          │     │ (FILTERED │
│ data)       │     │          │     │          │     │   DATA)   │
└─────────────┘     └──────────┘     └──────────┘     └───────────┘
```

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

### Performance Improvements

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

While implementing pushdown optimization can improve performance for mixed queries, **don't overlook the simpler solution**: for Snowflake-only queries, direct SQL pass-through is often the best approach. It's simpler to implement, maintains full Snowflake SQL compatibility, and leverages Snowflake's sophisticated query optimizer.

The pushdown implementation described in this document is most valuable for scenarios where DuckDB needs to coordinate between multiple data sources or apply DuckDB-specific processing to Snowflake data.