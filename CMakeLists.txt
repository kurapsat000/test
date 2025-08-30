cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME snowflake)

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be added in ./vcpkg.json and then
# used in cmake with find_package. Feel free to remove or replace with other dependencies.
# Note that it should also be removed from vcpkg.json to prevent needlessly installing it..
find_package(OpenSSL REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/include)

# Include all source files for the extension
set(EXTENSION_SOURCES 
    src/snowflake_extension.cpp
    src/snowflake_arrow_utils.cpp
    src/snowflake_attach.cpp
    src/snowflake_client.cpp
    src/snowflake_client_manager.cpp
    src/snowflake_config.cpp
    src/snowflake_functions.cpp
    src/snowflake_scan.cpp
    src/snowflake_secret_provider.cpp
    src/snowflake_secrets.cpp
    src/snowflake_transaction.cpp
    src/snowflake_types.cpp
    src/storage/snowflake_catalog.cpp
    src/storage/snowflake_catalog_set.cpp
    src/storage/snowflake_schema_entry.cpp
    src/storage/snowflake_schema_set.cpp
    src/storage/snowflake_storage.cpp
    src/storage/snowflake_table_entry.cpp
    src/storage/snowflake_table_set.cpp
)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

# Link OpenSSL in both the static library as the loadable extension
target_link_libraries(${EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(${LOADABLE_EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
