# DuckDB Snowflake Extension Usage Guide

## Overview

The DuckDB Snowflake extension allows you to query Snowflake data directly from DuckDB using Arrow ADBC for efficient data transfer. This guide covers installation, configuration, and usage.

## Installation

```sql
-- Install the extension
INSTALL snowflake FROM community;
LOAD snowflake;
```

## Setting Up Snowflake Credentials

### Method 1: Using Profiles (Recommended)

Create a named profile to securely store your Snowflake credentials:

```sql
-- Create a Snowflake profile
CREATE SECRET snowflake_profile_main (
    TYPE snowflake,
    ACCOUNT 'your_account',
    USERNAME 'your_username', 
    PASSWORD 'your_password',
    DATABASE 'your_database',
    WAREHOUSE 'your_warehouse'
);
```

Optional parameters:
- `WAREHOUSE`: Snowflake warehouse to use (recommended for performance)
- `ROLE`: Snowflake role to assume

### Method 2: Attach Database

You can attach a Snowflake database using either a profile/secret or a connection string:

#### Using SECRET (Recommended)

```sql
-- First create a secret with your credentials (if not already created)
CREATE SECRET my_snowflake_secret (
    TYPE snowflake,
    ACCOUNT 'your_account',
    USERNAME 'your_username',
    PASSWORD 'your_password',
    DATABASE 'your_database',
    WAREHOUSE 'your_warehouse'
);

-- Attach using the secret (read-only mode is required)
ATTACH '' AS snow_db (TYPE snowflake, SECRET my_snowflake_secret, READ_ONLY);

-- Now you can query tables directly
SELECT * FROM snow_db.schema_name.table_name;
```

#### Using Connection String

```sql
-- Attach using connection string (credentials in plain text, read-only mode required)
ATTACH 'account=myaccount;user=myuser;password=mypass;database=mydb;warehouse=mywh' AS snow_db (TYPE snowflake, READ_ONLY);

-- Now you can query tables directly
SELECT * FROM snow_db.schema_name.table_name;
```

## Querying Snowflake Data

### Using snowflake_scan Function

The `snowflake_scan` function allows you to execute arbitrary SQL queries against Snowflake:

```sql
-- Execute any Snowflake SQL query
SELECT * FROM snowflake_scan(
    'SELECT * FROM customers WHERE state = ''CA''',
    'snowflake_profile_main'  -- profile/secret name
);

-- Join Snowflake data with local DuckDB tables
SELECT 
    s.customer_id,
    s.customer_name,
    l.total_orders
FROM snowflake_scan(
    'SELECT customer_id, customer_name FROM dim_customers',
    'snowflake_profile_main'
) s
JOIN local_orders l ON s.customer_id = l.customer_id;
```

### Using Attached Database

Once attached, you can query Snowflake tables like native DuckDB tables:

```sql
-- Attach the database using a secret (recommended, read-only mode required)
ATTACH '' AS snow (TYPE snowflake, SECRET my_snowflake_secret, READ_ONLY);

-- Query tables directly using attached_name.schema.table notation
-- Note: The attached database name from your secret's DATABASE field is the root
-- You cannot access other Snowflake databases through the attachment
SELECT * FROM snow.public.customers LIMIT 10;

-- For Snowflake sample data (if DATABASE='SNOWFLAKE_SAMPLE_DATA' in your secret):
-- This would access SNOWFLAKE_SAMPLE_DATA.TPCH_SF1.CUSTOMER in Snowflake
SELECT * FROM snow.tpch_sf1.customer LIMIT 5;

-- Create a local copy of Snowflake data
CREATE TABLE local_customers AS 
SELECT * FROM snow.public.customers;

-- To query across multiple Snowflake databases, use snowflake_scan:
-- This allows you to specify the full database.schema.table path
SELECT * FROM snowflake_scan(
    'SELECT * FROM OTHER_DATABASE.SCHEMA.TABLE',
    'my_snowflake_secret'
);
```

## Profile Management

### List Profiles

```sql
-- View all Snowflake profiles
SELECT * FROM duckdb_secrets() WHERE type = 'snowflake';
```

### Update Profile

```sql
-- Drop and recreate to update
DROP SECRET snowflake_profile_main;

CREATE SECRET snowflake_profile_main (
    TYPE snowflake,
    ACCOUNT 'new_account',
    USERNAME 'new_username',
    PASSWORD 'new_password',
    DATABASE 'new_database',
    WAREHOUSE 'new_warehouse'
);
```

### Delete Profile

```sql
DROP SECRET snowflake_profile_main;
```

## Advanced Usage

### Complex Queries with CTEs

```sql
-- Use Snowflake's computational power for complex analytics
SELECT * FROM snowflake_scan(
    '
    WITH monthly_sales AS (
        SELECT 
            DATE_TRUNC(''month'', order_date) as month,
            SUM(amount) as total_sales
        FROM orders
        GROUP BY 1
    )
    SELECT 
        month,
        total_sales,
        LAG(total_sales) OVER (ORDER BY month) as prev_month_sales
    FROM monthly_sales
    ',
    'snowflake_profile_main'
);
```

### Data Export from Snowflake to DuckDB

```sql
-- Export large dataset efficiently
CREATE TABLE local_fact_sales AS
SELECT * FROM snowflake_scan(
    'SELECT * FROM fact_sales WHERE year >= 2020',
    'snowflake_profile_main'
);

-- Create Parquet files from Snowflake data
COPY (
    SELECT * FROM snowflake_scan(
        'SELECT * FROM large_table',
        'snowflake_profile_main'
    )
) TO 'output.parquet' (FORMAT PARQUET);
```

### Working with Multiple Profiles

```sql
-- Create multiple profiles for different environments
CREATE SECRET snowflake_dev (
    TYPE snowflake,
    ACCOUNT 'dev_account',
    USERNAME 'dev_user',
    PASSWORD 'dev_password',
    DATABASE 'dev_db',
    WAREHOUSE 'dev_warehouse'
);

CREATE SECRET snowflake_prod (
    TYPE snowflake,
    ACCOUNT 'prod_account',
    USERNAME 'prod_user',
    PASSWORD 'prod_password',
    DATABASE 'prod_db',
    WAREHOUSE 'prod_warehouse'
);

-- Option 1: Query using snowflake_scan with different profiles
SELECT * FROM snowflake_scan('SELECT COUNT(*) FROM users', 'snowflake_dev');
SELECT * FROM snowflake_scan('SELECT COUNT(*) FROM users', 'snowflake_prod');

-- Option 2: Attach multiple databases simultaneously (read-only mode required)
ATTACH '' AS dev_snow (TYPE snowflake, SECRET snowflake_dev, READ_ONLY);
ATTACH '' AS prod_snow (TYPE snowflake, SECRET snowflake_prod, READ_ONLY);

-- Compare data across environments
SELECT 
    'dev' as environment, COUNT(*) as user_count 
FROM dev_snow.public.users
UNION ALL
SELECT 
    'prod' as environment, COUNT(*) as user_count 
FROM prod_snow.public.users;
```

## Performance Tips

1. **Use a Warehouse**: Always specify a `WAREHOUSE` in your profile for better performance
2. **Filter in Snowflake**: Push filters to Snowflake queries to reduce data transfer
3. **Use Column Selection**: Only select columns you need to minimize data transfer
4. **Batch Operations**: For multiple queries, consider using an attached database

## Troubleshooting

### Connection Issues

```sql
-- Test connection with a simple query
SELECT * FROM snowflake_scan('SELECT 1', 'snowflake_profile_main');
```

### Permission Errors

Ensure your Snowflake user has appropriate permissions:
- `USAGE` on warehouse
- `USAGE` on database
- `SELECT` on tables/views

### Type Compatibility

The extension automatically maps Snowflake types to DuckDB types:
- `VARCHAR` → `VARCHAR`
- `NUMBER(p,0)` → Appropriate integer type based on precision
- `NUMBER(p,s)` → `DECIMAL(p,s)`
- `FLOAT` → `FLOAT`
- `BOOLEAN` → `BOOLEAN`

## Security Best Practices

1. **Use Profiles**: Store credentials in profiles rather than connection strings
2. **Principle of Least Privilege**: Use Snowflake roles with minimal required permissions
3. **Separate Environments**: Use different profiles for dev/test/prod
4. **Regular Rotation**: Regularly update passwords and credentials

## Examples

### ETL Pipeline Example

```sql
-- Extract data from Snowflake, transform in DuckDB, load to local storage
CREATE TEMPORARY TABLE staging_data AS
SELECT 
    customer_id,
    order_date,
    amount * 1.1 as adjusted_amount  -- Apply business logic
FROM snowflake_scan(
    'SELECT * FROM orders WHERE order_date >= CURRENT_DATE - 30',
    'snowflake_profile_main'
);

-- Further transformations
CREATE TABLE final_report AS
SELECT 
    DATE_TRUNC('week', order_date) as week,
    COUNT(DISTINCT customer_id) as unique_customers,
    SUM(adjusted_amount) as total_revenue
FROM staging_data
GROUP BY 1;

-- Export results
COPY final_report TO 'weekly_report.csv' (HEADER, DELIMITER ',');
```

### Cross-Database Analytics

```sql
-- Combine Snowflake data with local data (read-only mode required for Snowflake)
ATTACH '' AS snow (TYPE snowflake, SECRET snowflake_prod, READ_ONLY);
ATTACH 'local.db' AS local_db;

-- Analyze across databases
SELECT 
    s.product_category,
    COUNT(DISTINCT l.customer_id) as local_customers,
    SUM(s.revenue) as snowflake_revenue
FROM snow.public.sales s
JOIN local_db.customers l ON s.product_id = l.last_purchased_product_id
GROUP BY 1;
```

## Limitations

- Read-only access (no INSERT/UPDATE/DELETE operations on Snowflake)
- Some Snowflake-specific functions may not be available
- Large result sets should be filtered at source for optimal performance

## Support

For issues or questions:
- Check the [GitHub repository](https://github.com/your-repo/duckdb-snowflake)
- Review Snowflake ADBC driver documentation
- Ensure you have the latest version of the extension