#!/bin/bash
# Test script to verify RPATH setup for the Snowflake extension

echo "=== Testing RPATH Configuration for DuckDB Snowflake Extension ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if extension exists
EXTENSION_PATH="build/release/extension/snowflake/snowflake.duckdb_extension"
ADBC_LIB_PATH="build/release/extension/snowflake/libadbc_driver_snowflake.so"

if [ ! -f "$EXTENSION_PATH" ]; then
    echo -e "${RED}Error: Extension not found at $EXTENSION_PATH${NC}"
    echo "Please run 'make release' first"
    exit 1
fi

if [ ! -f "$ADBC_LIB_PATH" ]; then
    echo -e "${RED}Error: ADBC library not found at $ADBC_LIB_PATH${NC}"
    echo "The Makefile should have copied it there during build"
    exit 1
fi

echo "1. Checking RPATH in the extension:"
echo "   Command: readelf -d $EXTENSION_PATH | grep -E '(RPATH|RUNPATH)'"
readelf -d "$EXTENSION_PATH" | grep -E '(RPATH|RUNPATH)'
echo

echo "2. Checking shared library dependencies:"
echo "   Command: ldd $EXTENSION_PATH | grep 'not found'"
NOT_FOUND=$(ldd "$EXTENSION_PATH" | grep 'not found')
if [ -z "$NOT_FOUND" ]; then
    echo -e "${GREEN}   All dependencies resolved successfully${NC}"
else
    echo -e "${RED}   Missing dependencies:${NC}"
    echo "$NOT_FOUND"
fi
echo

echo "3. Testing extension loading from different directory:"
# Create a temporary directory
TEMP_DIR=$(mktemp -d)
cp "$EXTENSION_PATH" "$TEMP_DIR/"
cp "$ADBC_LIB_PATH" "$TEMP_DIR/"

echo "   Copied files to: $TEMP_DIR"
ls -la "$TEMP_DIR"
echo

# Test loading with DuckDB
echo "4. Testing with DuckDB:"
echo "   Running: cd /tmp && duckdb -c \"LOAD '$TEMP_DIR/snowflake.duckdb_extension';\""

cd /tmp
RESULT=$(./build/release/duckdb -c "LOAD '$TEMP_DIR/snowflake.duckdb_extension'; SELECT 'Extension loaded successfully' as result;" 2>&1)

if echo "$RESULT" | grep -q "Extension loaded successfully"; then
    echo -e "${GREEN}   SUCCESS: Extension loaded correctly!${NC}"
else
    echo -e "${RED}   FAILED: Extension could not be loaded${NC}"
    echo "   Error output:"
    echo "$RESULT"
fi

# Cleanup
rm -rf "$TEMP_DIR"

echo
echo "=== Debugging Tips ==="
echo "If the extension fails to load:"
echo "1. Run with LD_DEBUG=libs to see library search paths:"
echo "   LD_DEBUG=libs duckdb -c \"LOAD 'path/to/snowflake.duckdb_extension';\""
echo
echo "2. Check that both files are in the same directory:"
echo "   - snowflake.duckdb_extension"
echo "   - libadbc_driver_snowflake.so"
echo
echo "3. Verify RPATH is set to \$ORIGIN:"
echo "   readelf -d snowflake.duckdb_extension | grep RUNPATH"
echo
echo "4. As a workaround, set LD_LIBRARY_PATH:"
echo "   export LD_LIBRARY_PATH=/path/to/extension/directory:\$LD_LIBRARY_PATH"