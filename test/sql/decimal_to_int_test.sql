-- Test DECIMAL to INT conversion with use_high_precision=false

-- Create test connection with use_high_precision=false (default)
-- This should convert DECIMAL(38,0) to INT64
CREATE OR REPLACE VARIABLE sf_connection VARCHAR = 'account=myaccount;user=myuser;password=mypass;warehouse=mywh;database=mydb;use_high_precision=false';

-- Test 1: Query a table with DECIMAL(38,0) columns
-- The ADBC driver should return these as INT64 instead of DECIMAL128
SELECT 
    column_name,
    data_type,
    numeric_precision,
    numeric_scale
FROM snowflake_scan(
    getvariable(sf_connection),
    'SELECT CAST(123 AS NUMBER(38,0)) AS big_int, CAST(456.78 AS NUMBER(10,2)) AS decimal_val'
);

-- Test 2: Verify type mapping for various NUMBER types
SELECT 
    typeof(big_int) AS big_int_type,      -- Should be BIGINT
    typeof(decimal_val) AS decimal_type   -- Should be DOUBLE (scale > 0)
FROM snowflake_scan(
    getvariable(sf_connection),
    'SELECT CAST(123 AS NUMBER(38,0)) AS big_int, CAST(456.78 AS NUMBER(10,2)) AS decimal_val'
);

-- Test 3: Test with high_precision=true
CREATE OR REPLACE VARIABLE sf_connection_hp VARCHAR = 'account=myaccount;user=myuser;password=mypass;warehouse=mywh;database=mydb;use_high_precision=true';

SELECT 
    typeof(big_int) AS big_int_type,      -- Should be DECIMAL(38,0)
    typeof(decimal_val) AS decimal_type   -- Should be DECIMAL(10,2)
FROM snowflake_scan(
    getvariable(sf_connection_hp),
    'SELECT CAST(123 AS NUMBER(38,0)) AS big_int, CAST(456.78 AS NUMBER(10,2)) AS decimal_val'
);

-- Test 4: Performance comparison
-- With use_high_precision=false, operations on NUMBER(38,0) should be faster
EXPLAIN ANALYZE
SELECT COUNT(*), SUM(amount)
FROM snowflake_scan(
    getvariable(sf_connection),
    'SELECT CAST(order_amount AS NUMBER(38,0)) AS amount FROM large_orders_table LIMIT 1000000'
);