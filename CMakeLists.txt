cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME snowflake)
project(${TARGET_NAME})

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be added in ./vcpkg.json and then
# used in cmake with find_package. Feel free to remove or replace with other dependencies.
# Note that it should also be removed from vcpkg.json to prevent needlessly installing it..
find_package(OpenSSL REQUIRED)
find_package(Arrow CONFIG REQUIRED)
find_package(AdbcDriverManager CONFIG REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

include_directories(src/include)

set(EXTENSION_SOURCES
    src/snowflake_extension.cpp
    src/snowflake_connection.cpp
    src/snowflake_arrow_utils.cpp
    src/snowflake_scan.cpp
    src/snowflake_attach.cpp
    src/snowflake_storage.cpp
)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})
set_property(TARGET ${LOADABLE_EXTENSION_NAME} PROPERTY "EXTENSION_VERSION_SCRIPT" "TRUE")

# Define the Snowflake ADBC driver library path
set(SNOWFLAKE_ADBC_LIB "arrow-adbc/c/driver/snowflake/libadbc_driver_snowflake.so")

# Link dependencies in both the static library and the loadable extension
# Note: Snowflake ADBC driver will be loaded dynamically to avoid symbol conflicts
target_link_libraries(${EXTENSION_NAME} 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Arrow::arrow_static
    AdbcDriverManager::adbc_driver_manager_static
)
target_link_libraries(${LOADABLE_EXTENSION_NAME} 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Arrow::arrow_static
    AdbcDriverManager::adbc_driver_manager_static
)

# Define path to Snowflake ADBC shared library for dynamic loading
target_compile_definitions(${EXTENSION_NAME} PRIVATE 
    -DSNOWFLAKE_ADBC_LIB_PATH="${SNOWFLAKE_ADBC_LIB}"
)
target_compile_definitions(${LOADABLE_EXTENSION_NAME} PRIVATE 
    -DSNOWFLAKE_ADBC_LIB_PATH="${SNOWFLAKE_ADBC_LIB}"
)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")