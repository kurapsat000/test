#!/bin/bash

# Simple build script for DuckDB Snowflake Extension
# Uses the standard DuckDB extension build process

set -e

echo "Building DuckDB Snowflake Extension..."

# Check if we're in the right directory
if [ ! -f "extension_config.cmake" ]; then
    echo "Error: extension_config.cmake not found. Please run this script from the project root."
    exit 1
fi

# Set up environment variables
export GEN=ninja
export DUCKDB_EXTENSIONS=snowflake

# Build the extension using the standard DuckDB extension build process
echo "Running make release..."
make release

echo "Build completed successfully!"
echo "Extension location: build/release/extension/snowflake/snowflake.duckdb_extension" 