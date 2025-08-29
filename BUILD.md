# Building the DuckDB Snowflake Extension

## Prerequisites

- CMake >= 3.18
- C++17 compatible compiler
- vcpkg (for dependency management)
- Git (with submodules)

## Build Instructions

### Quick Build

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/your-repo/duckdb-snowflake.git
cd duckdb-snowflake

# Build release version (default)
make

# Or explicitly:
make release-snowflake
```

### Build Targets

The Makefile provides several targets:

```bash
# Build release version with ADBC driver
make release-snowflake

# Build debug version with ADBC driver
make debug-snowflake

# Build release without ADBC (uses pre-built driver)
make release

# Build debug without ADBC (uses pre-built driver)  
make debug

# Run tests
make test

# Clean build artifacts
make clean

# Clean everything including ADBC artifacts
make clean-all
```

### ADBC Driver

The extension requires the Snowflake ADBC driver. The build process:

1. Automatically builds the ADBC driver from the `arrow-adbc` submodule
2. Copies the driver to the extension directory
3. Uses dynamic path resolution to load the driver at runtime

### Manual ADBC Build

If you need to build ADBC separately:

```bash
# Build ADBC release
make build-adbc-release

# Build ADBC debug
make build-adbc-debug

# Copy to extension directory
make copy-adbc-to-extension BUILD_TYPE=release
```

### Troubleshooting

If you get warnings about overriding Makefile targets, use the `-snowflake` suffixed targets:
- `make release-snowflake` instead of `make release`
- `make debug-snowflake` instead of `make debug`

These targets properly chain the ADBC build before the extension build.

### Debug Builds

Debug builds include additional logging for troubleshooting:

```bash
make debug-snowflake

# Run with debug output
./build/debug/duckdb
```

Debug builds will print detailed information about:
- ADBC driver loading paths
- Connection establishment
- Type conversions