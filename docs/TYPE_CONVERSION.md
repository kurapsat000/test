# Type Conversion in DuckDB-Snowflake Extension

## Overview

The DuckDB-Snowflake extension now supports configurable type conversion for numeric types through the `use_high_precision` parameter. This feature allows you to control how Snowflake's `NUMBER`/`DECIMAL` types are mapped to DuckDB types.

## The `use_high_precision` Parameter

### Default Behavior (use_high_precision=false)

By default, `use_high_precision` is set to `false`, which enables automatic type conversion:

- **`DECIMAL(p,0)` → `INT64`**: Numbers with scale=0 are converted to INT64 for better performance
- **`DECIMAL(p,s)` → `FLOAT64`**: Numbers with scale>0 are converted to FLOAT64
- **`NUMBER(p,0)` → Integer types**: Based on precision:
  - `NUMBER(1-2,0)` → `TINYINT`
  - `NUMBER(3-4,0)` → `SMALLINT`
  - `NUMBER(5-9,0)` → `INTEGER`
  - `NUMBER(10-18,0)` → `BIGINT`
  - `NUMBER(19-38,0)` → `DECIMAL`

### High Precision Mode (use_high_precision=true)

When high precision mode is enabled:

- **All numeric types preserve exact precision**: `DECIMAL(p,s)` → `DECIMAL128(p,s)`
- **No automatic conversion to INT or FLOAT**
- **Better for financial calculations** requiring exact decimal arithmetic

## Configuration

### Connection String

Add the `use_high_precision` parameter to your connection string:

```sql
-- Default mode (converts DECIMAL to INT/FLOAT)
CREATE SECRET snowflake_secret (
    TYPE SNOWFLAKE,
    account = 'myaccount',
    user = 'myuser',
    password = 'mypass',
    database = 'mydb',
    warehouse = 'mywh',
    use_high_precision = 'false'
);

-- High precision mode (preserves DECIMAL types)
CREATE SECRET snowflake_secret_hp (
    TYPE SNOWFLAKE,
    account = 'myaccount',
    user = 'myuser',
    password = 'mypass',
    database = 'mydb',
    warehouse = 'mywh',
    use_high_precision = 'true'
);
```

### Using with snowflake_scan

```sql
-- With use_high_precision=false (default)
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypass;database=mydb;use_high_precision=false',
    'SELECT CAST(order_id AS NUMBER(38,0)) AS id FROM orders'
);
-- Result: id column is BIGINT

-- With use_high_precision=true
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypass;database=mydb;use_high_precision=true',
    'SELECT CAST(order_id AS NUMBER(38,0)) AS id FROM orders'
);
-- Result: id column is DECIMAL(38,0)
```

## Performance Implications

### Benefits of use_high_precision=false

1. **Faster arithmetic operations**: Native INT64 operations are significantly faster than DECIMAL128
2. **Reduced memory usage**: INT64 uses 8 bytes vs 16 bytes for DECIMAL128
3. **Better compatibility**: Many analytical functions work more efficiently with native integer types
4. **Improved join performance**: Integer joins are faster than decimal joins

### When to use use_high_precision=true

1. **Financial calculations**: When exact decimal arithmetic is required
2. **Compliance requirements**: When precision loss is not acceptable
3. **Large numeric values**: When dealing with numbers larger than INT64 range

## Examples

### Example 1: Performance Comparison

```sql
-- Fast aggregation with INT64 (use_high_precision=false)
SELECT COUNT(*), SUM(amount), AVG(amount)
FROM snowflake_scan(
    'account=myaccount;...;use_high_precision=false',
    'SELECT CAST(transaction_amount AS NUMBER(18,0)) AS amount FROM transactions'
);

-- Slower but precise with DECIMAL128 (use_high_precision=true)
SELECT COUNT(*), SUM(amount), AVG(amount)
FROM snowflake_scan(
    'account=myaccount;...;use_high_precision=true',
    'SELECT CAST(transaction_amount AS NUMBER(18,0)) AS amount FROM transactions'
);
```

### Example 2: Type Inspection

```sql
-- Check the actual types returned
SELECT 
    typeof(int_col) AS int_type,
    typeof(decimal_col) AS decimal_type
FROM snowflake_scan(
    'account=myaccount;...;use_high_precision=false',
    'SELECT 
        CAST(123 AS NUMBER(10,0)) AS int_col,
        CAST(123.45 AS NUMBER(10,2)) AS decimal_col'
);
-- Result: int_type = BIGINT, decimal_type = DOUBLE

-- With high precision
SELECT 
    typeof(int_col) AS int_type,
    typeof(decimal_col) AS decimal_type
FROM snowflake_scan(
    'account=myaccount;...;use_high_precision=true',
    'SELECT 
        CAST(123 AS NUMBER(10,0)) AS int_col,
        CAST(123.45 AS NUMBER(10,2)) AS decimal_col'
);
-- Result: int_type = DECIMAL(10,0), decimal_type = DECIMAL(10,2)
```

## Technical Details

This feature leverages the ADBC Snowflake driver's built-in type conversion capabilities. When `use_high_precision=false`, the driver automatically:

1. Detects FIXED-point numbers with scale=0
2. Converts them to INT64 at the Arrow layer
3. Passes the converted data to DuckDB

This conversion happens during data transfer, ensuring optimal performance without requiring any post-processing in DuckDB.

## Limitations

1. **Platform defaults**: The ADBC driver has platform-specific defaults (Windows/macOS default to true, Linux defaults to false)
2. **Precision loss**: Converting DECIMAL to FLOAT64 may result in precision loss for very large numbers
3. **Range limitations**: NUMBER(38,0) values larger than INT64 max will cause overflow errors when use_high_precision=false

## Best Practices

1. **Default to use_high_precision=false** for analytical workloads where performance is critical
2. **Use use_high_precision=true** for financial or accounting data where precision is paramount
3. **Test your queries** with both settings to understand the performance and precision trade-offs
4. **Consider your data types**: If your Snowflake tables use INTEGER types instead of NUMBER, this setting has no effect