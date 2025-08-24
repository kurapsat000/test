#!/bin/bash
# Script to run Snowflake integration tests with credentials from .env

# Load environment variables
if [ -f .env ]; then
    echo "Loading credentials from .env file..."
    export $(cat .env | grep -v '^#' | xargs)
else
    echo "Error: .env file not found!"
    echo "Please copy .env.example to .env and fill in your credentials"
    exit 1
fi

# Check if required variables are set
if [ -z "$SNOWFLAKE_TEST_ACCOUNT" ] || [ -z "$SNOWFLAKE_TEST_USER" ] || [ -z "$SNOWFLAKE_TEST_PASSWORD" ]; then
    echo "Error: Required environment variables not set!"
    echo "Please ensure SNOWFLAKE_TEST_ACCOUNT, SNOWFLAKE_TEST_USER, and SNOWFLAKE_TEST_PASSWORD are set in .env"
    exit 1
fi

echo "Running Snowflake integration tests..."
echo "Account: $SNOWFLAKE_TEST_ACCOUNT"
echo "User: $SNOWFLAKE_TEST_USER"
echo "Database: $SNOWFLAKE_TEST_DATABASE"
echo "Warehouse: $SNOWFLAKE_TEST_WAREHOUSE"
echo

# Run the tests
if [ "$1" == "debug" ]; then
    echo "Running in debug mode..."
    ./build/debug/test/unittest "test/sql/test_list_schemas.test"
else
    ./build/release/test/unittest "test/sql/test_list_schemas.test"
fi