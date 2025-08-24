# Snowflake

This repository is based on https://github.com/duckdb/extension-template, check it out if you want to build and ship your own DuckDB extension.

---

This extension, Snowflake, enables DuckDB to connect to and query Snowflake data sources using the Arrow Database Connectivity (ADBC) interface. It provides seamless integration between DuckDB and Snowflake, allowing you to execute SQL queries against Snowflake databases directly from DuckDB.


## Building
### Managing dependencies
DuckDB extensions uses VCPKG for dependency management. Enabling VCPKG is very simple: follow the [installation instructions](https://vcpkg.io/en/getting-started) or just run the following:
```shell
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_TOOLCHAIN_PATH=`pwd`/vcpkg/scripts/buildsystems/vcpkg.cmake
```
Note: VCPKG is only required for extensions that want to rely on it for dependency management. If you want to develop an extension without dependencies, or want to do your own dependency management, just skip this step. Note that the example extension uses VCPKG to build with a dependency for instructive purposes, so when skipping this step the build may not work without removing the dependency.

### Build steps
Now to build the extension, run:
```sh
make
```

For building with ADBC driver support:
```sh
make release-snowflake
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/snowflake/snowflake.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically loaded.
- `unittest` is the test runner of duckdb. Again, the extension is already linked into the binary.
- `snowflake.duckdb_extension` is the loadable binary as it would be distributed.

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`.

Now we can use the features from the extension directly in DuckDB. The extension provides several functions:

### Basic Functions
- `snowflake(name)` - Returns a greeting with the provided name
- `snowflake_openssl_version(name)` - Returns a greeting with OpenSSL version information
- `snowflake_adbc_version(name)` - Returns a greeting with ADBC version and driver status

### Example Usage
```sql
-- Basic function
SELECT snowflake('Jane') as result;
┌───────────────┐
│    result     │
│    varchar    │
├───────────────┤
│ Snowflake Jane 🐥 │
└───────────────┘

-- Function with OpenSSL version
SELECT snowflake_openssl_version('Michael') as result;
┌─────────────────────────────────────────────────────────────┐
│                           result                            │
│                          varchar                            │
├─────────────────────────────────────────────────────────────┤
│ Snowflake Michael, my linked OpenSSL version is OpenSSL... │
└─────────────────────────────────────────────────────────────┘

-- Function with ADBC version
SELECT snowflake_adbc_version('Test') as result;
┌─────────────────────────────────────────────────────────────┐
│                           result                            │
│                          varchar                            │
├─────────────────────────────────────────────────────────────┤
│ Snowflake Test - ADBC Available - Driver Found             │
└─────────────────────────────────────────────────────────────┘
```

### ADBC Integration
The extension includes Arrow Database Connectivity (ADBC) integration for connecting to Snowflake. This functionality is currently in development and will provide:
- Direct connection to Snowflake databases
- SQL query execution against Snowflake
- Data transfer between DuckDB and Snowflake

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:
```sh
make test
```

### Installing the deployed binaries
To install your extension binaries from S3, you will need to do two things. Firstly, DuckDB should be launched with the
`allow_unsigned_extensions` option set to true. How to set this will depend on the client you're using. Some examples:

CLI:
```shell
duckdb -unsigned
```

Python:
```python
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions' : 'true'})
```

NodeJS:
```js
db = new duckdb.Database(':memory:', {"allow_unsigned_extensions": "true"});
```

Secondly, you will need to set the repository endpoint in DuckDB to the HTTP url of your bucket + version of the extension
you want to install. To do this run the following SQL query in DuckDB:
```sql
SET custom_extension_repository='bucket.s3.eu-west-1.amazonaws.com/<your_extension_name>/latest';
```
Note that the `/latest` path will allow you to install the latest extension version available for your current version of
DuckDB. To specify a specific version, you can pass the version instead.

After running these steps, you can install and load your extension using the regular INSTALL/LOAD commands in DuckDB:
```sql
INSTALL snowflake
LOAD snowflake
```
