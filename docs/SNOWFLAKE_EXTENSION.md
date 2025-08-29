# DuckDB Snowflake Extension

A DuckDB extension that enables seamless querying of Snowflake databases using Arrow ADBC drivers with runtime loading capabilities.

## Overview

This extension provides the `snowflake_scan` table function that allows you to execute SQL queries against Snowflake databases directly from DuckDB, leveraging Arrow's columnar data format for efficient data transfer.

## Features

- **Runtime Driver Loading**: Dynamically loads Snowflake ADBC driver at runtime
- **Arrow-Native Pipeline**: Efficient data transfer using Arrow columnar format
- **Connection Pooling**: Automatic connection management and reuse
- **Multiple Authentication**: Supports password, OAuth, and key-pair authentication
- **Schema Detection**: Automatic column type inference and mapping
- **Streaming Results**: Memory-efficient processing of large result sets
- **Database Attachment**: Attach Snowflake databases for schema-qualified access
- **Type Precision Control**: Configurable decimal-to-integer conversion for performance

## Installation

### Prerequisites

- DuckDB with extension support
- Arrow ADBC Snowflake driver (shared library)
- vcpkg for dependency management

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd duckdb-snowflake

# Build the extension
make release

# The extension will be available at:
# build/release/extension/snowflake/snowflake.duckdb_extension
```

## Configuration

### Driver Setup

The extension automatically loads the Snowflake ADBC driver from the same directory as the extension. During build, the ADBC driver library should be copied to:
```
build/release/extension/snowflake/libadbc_driver_snowflake.so
```

The extension uses dynamic path resolution to find the driver library at runtime, ensuring portability across different installations.

## Usage

### Loading the Extension

```sql
LOAD snowflake;
```

### Basic Query Syntax

```sql
SELECT * FROM snowflake_scan(
    'connection_string',
    'sql_query'
);
```

### Connection String Format

The connection string uses semicolon-separated key=value pairs:

```
account=<account>;user=<username>;password=<password>;warehouse=<warehouse>;database=<database>
```

#### Required Parameters
- `account`: Snowflake account identifier
- `user` or `username`: Snowflake username

#### Optional Parameters
- `password`: User password (for password authentication)
- `warehouse`: Snowflake warehouse name
- `database`: Default database name
- `schema`: Default schema name
- `role`: Snowflake role to assume
- `auth_type`: Authentication type (`password`, `oauth`, `key_pair`)
- `token`: OAuth token (for OAuth authentication)
- `private_key`: Private key (for key-pair authentication)
- `query_timeout`: Query timeout in seconds (default: 300)
- `keep_alive`: Keep connection alive (default: true)
- `use_high_precision`: Use high precision for decimals (default: false)

## Database Attachment

### Using snowflake_attach Table Function

The `snowflake_attach` function creates views in your DuckDB session for all tables in the specified Snowflake database:

```sql
-- Attach all tables from a Snowflake database as views
SELECT * FROM snowflake_attach(
    'account=myaccount;user=myuser;password=mypassword;warehouse=COMPUTE_WH;database=MYDB'
);

-- Now you can query Snowflake tables directly
SELECT * FROM CUSTOMER_TABLE LIMIT 10;
SELECT * FROM ORDERS_TABLE WHERE order_date > '2024-01-01';
```

### Using ATTACH Command (PostgreSQL-style)

For schema-qualified access to Snowflake tables:

```sql
-- Attach a Snowflake database
ATTACH 'account=myaccount;user=myuser;password=mypassword;warehouse=COMPUTE_WH;database=SALES_DB' 
AS salesdb (TYPE snowflake);

-- Query using schema-qualified names
SELECT * FROM salesdb.public.customers;
SELECT * FROM salesdb.analytics.monthly_revenue;

-- Join across databases
SELECT c.name, o.total
FROM salesdb.public.customers c
JOIN salesdb.public.orders o ON c.id = o.customer_id;

-- Detach when done
DETACH salesdb;
```

## Examples

### Simple Query
```sql
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypassword;warehouse=COMPUTE_WH',
    'SELECT 1 as test_column'
);
```

### Querying Sample Data
```sql
SELECT COUNT(*) FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypassword;warehouse=COMPUTE_WH;database=SNOWFLAKE_SAMPLE_DATA',
    'SELECT * FROM TPCH_SF1.CUSTOMER LIMIT 100'
);
```

### Complex Query with Joins
```sql
SELECT 
    customer_name,
    total_orders,
    avg_order_value
FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypassword;warehouse=COMPUTE_WH;database=MYDB',
    'SELECT 
        c.c_name as customer_name,
        COUNT(o.o_orderkey) as total_orders,
        AVG(o.o_totalprice) as avg_order_value
     FROM customer c
     JOIN orders o ON c.c_custkey = o.o_custkey
     GROUP BY c.c_name
     ORDER BY total_orders DESC
     LIMIT 10'
);
```

### Using with DuckDB Operations
```sql
-- Combine Snowflake data with local DuckDB tables
SELECT 
    sf.product_id,
    sf.sales_amount,
    local.discount_rate,
    sf.sales_amount * (1 - local.discount_rate) as discounted_amount
FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypassword;warehouse=COMPUTE_WH;database=SALES_DB',
    'SELECT product_id, SUM(amount) as sales_amount FROM sales GROUP BY product_id'
) sf
JOIN local_discounts local ON sf.product_id = local.product_id;
```

## Authentication Methods

### Password Authentication (Default)
```sql
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;password=mypassword',
    'SELECT CURRENT_USER()'
);
```

### OAuth Authentication
```sql
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;auth_type=oauth;token=your_oauth_token',
    'SELECT CURRENT_USER()'
);
```

### Key-Pair Authentication
```sql
SELECT * FROM snowflake_scan(
    'account=myaccount;user=myuser;auth_type=key_pair;private_key=your_private_key',
    'SELECT CURRENT_USER()'
);
```

## Data Type Mapping

| Snowflake Type | DuckDB Type (use_high_precision=false) | DuckDB Type (use_high_precision=true) | Notes |
|----------------|---------------------------------------|--------------------------------------|-------|
| NUMBER(p,0) | BIGINT/INTEGER | DECIMAL(p,0) | INT types for better performance when precision=false |
| NUMBER(p,s) | DOUBLE | DECIMAL(p,s) | DOUBLE for performance, DECIMAL for accuracy |
| DECIMAL | DOUBLE/BIGINT | DECIMAL | Depends on scale |
| INTEGER | BIGINT | BIGINT | |
| FLOAT | DOUBLE | DOUBLE | |
| VARCHAR | VARCHAR | VARCHAR | |
| CHAR | VARCHAR | VARCHAR | |
| BOOLEAN | BOOLEAN | BOOLEAN | |
| DATE | DATE | DATE | |
| TIMESTAMP | TIMESTAMP | TIMESTAMP | |
| VARIANT | VARCHAR | VARCHAR | JSON serialized |
| ARRAY | LIST | LIST | Converted to DuckDB lists |
| OBJECT | STRUCT | STRUCT | Converted to DuckDB structs |

## Performance Considerations

### Connection Pooling
The extension automatically pools connections based on connection strings. Identical connection strings will reuse the same connection.

### Batch Processing
Data is processed in batches using Arrow's columnar format for optimal memory usage and performance.

### Type Precision Optimization
- Set `use_high_precision=false` (default) for better performance with numeric types
- DECIMAL(p,0) columns are converted to INT64 for faster operations
- DECIMAL(p,s) columns are converted to DOUBLE for efficient processing
- Use `use_high_precision=true` only when exact decimal arithmetic is required

### Query Optimization
- Use `LIMIT` clauses in Snowflake queries to reduce data transfer
- Apply filters at the Snowflake level when possible
- Consider using Snowflake's query optimization features

## Error Handling

### Common Error Messages

#### Connection Errors
```
Failed to initialize connection: [Snowflake] 390100 (08004): Incorrect username or password was specified.
```
**Solution**: Verify your credentials and account information.

#### Driver Loading Errors
```
Failed to set Snowflake driver path
```
**Solution**: Ensure the ADBC driver is available at the configured path.

#### Query Errors
```
Failed to get schema: [Snowflake] 090105 (22000): Cannot perform SELECT. This session does not have a current database.
```
**Solution**: Specify a database in the connection string or use fully qualified table names.

## Testing

### Running Tests
```bash
# Run all tests
make test_release

# Run specific integration tests
./build/release/test/unittest "test/sql/snowflake_integration.test"
```

### Test Configuration
Update test credentials in `test/sql/snowflake_integration.test`:
```sql
SELECT * FROM snowflake_scan(
    'account=your_account;user=your_user;password=your_password;warehouse=your_warehouse;database=your_database',
    'SELECT 1 as test_column'
);
```

## Troubleshooting

### Driver Path Issues
1. Verify the driver exists: `ls -la /path/to/libadbc_driver_snowflake.so`
2. Check library dependencies: `ldd /path/to/libadbc_driver_snowflake.so`
3. Update `CMakeLists.txt` with correct path
4. Rebuild the extension: `make release`

### Connection Issues
1. Test credentials with Snowflake's web interface
2. Verify network connectivity to Snowflake
3. Check firewall and proxy settings
4. Ensure warehouse is active and accessible

### Performance Issues
1. Monitor query execution in Snowflake's query history
2. Use appropriate warehouse sizes
3. Optimize SQL queries for Snowflake's architecture
4. Consider data locality and clustering

## Architecture

### Components
- **SnowflakeExtension**: Main extension registration
- **SnowflakeScanFunction**: Table function implementation
- **SnowflakeConnection**: Connection management with ADBC
- **ArrowStreamWrapper**: Arrow data stream processing
- **SnowflakeConnectionManager**: Connection pooling

### Data Flow
1. **Query Planning**: DuckDB parses and plans the query
2. **Connection**: Extension connects to Snowflake via ADBC
3. **Query Execution**: SQL is executed on Snowflake
4. **Data Streaming**: Results stream back via Arrow format
5. **Type Conversion**: Arrow data converts to DuckDB types
6. **Result Integration**: Data integrates with DuckDB query result

## Dependencies

- **DuckDB**: Core database engine
- **Arrow**: Columnar data format and processing
- **Arrow ADBC**: Database connectivity standard
- **OpenSSL**: Secure connections
- **vcpkg**: Package management

## License

[Include your license information here]

## Contributing

[Include contribution guidelines here]

## Support

[Include support information here]