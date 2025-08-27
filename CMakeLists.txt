cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME snowflake)
project(${TARGET_NAME})
include_directories(src/include)

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be added in ./vcpkg.json and then
# used in cmake with find_package. Feel free to remove or replace with other dependencies.
# Note that it should also be removed from vcpkg.json to prevent needlessly installing it..

# Find Arrow dependencies first to ensure proper linking
find_package(Protobuf CONFIG REQUIRED)
if(NOT WIN32)
    find_package(gRPC CONFIG REQUIRED)
    find_package(absl CONFIG REQUIRED) 
    find_package(c-ares CONFIG REQUIRED)
endif()
find_package(OpenSSL REQUIRED)

# Find Arrow after its dependencies are resolved
find_package(Arrow CONFIG REQUIRED)

# ADBC will be built manually via Makefile and linked dynamically
# This ensures full functionality works on all platforms
set(ADBC_AVAILABLE TRUE)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

set(EXTENSION_SOURCES
    src/snowflake_extension.cpp
    src/snowflake_arrow_utils.cpp
    src/snowflake_secrets.cpp
    src/snowflake_secret_provider.cpp
    src/snowflake_scan.cpp
    src/snowflake_client.cpp
    src/snowflake_client_manager.cpp
    src/snowflake_config.cpp
    src/snowflake_functions.cpp
    src/snowflake_types.cpp
    src/snowflake_transaction.cpp
    src/storage/snowflake_storage.cpp
    src/storage/snowflake_catalog.cpp
    src/storage/snowflake_catalog_set.cpp
    src/storage/snowflake_schema_entry.cpp
    src/storage/snowflake_schema_set.cpp
    src/storage/snowflake_table_entry.cpp
    src/storage/snowflake_table_set.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_definitions(-DDEBUG_SNOWFLAKE)
endif()

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})
set_property(TARGET ${LOADABLE_EXTENSION_NAME} PROPERTY "EXTENSION_VERSION_SCRIPT" "TRUE")

# Define the Snowflake ADBC driver library path
if(WIN32)
    set(SNOWFLAKE_ADBC_LIB "adbc_driver_snowflake.dll")
elseif(APPLE)
    set(SNOWFLAKE_ADBC_LIB "libadbc_driver_snowflake.dylib")
else()
    set(SNOWFLAKE_ADBC_LIB "libadbc_driver_snowflake.so")
endif()

# Link dependencies in both the static library and the loadable extension
# Note: Snowflake ADBC driver will be loaded dynamically to avoid symbol conflicts
target_link_libraries(${EXTENSION_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto
    Arrow::arrow_static
    protobuf::libprotobuf
    ${CMAKE_DL_LIBS}
)

# Add complex dependencies only on Unix systems
if(NOT WIN32)
    target_link_libraries(${EXTENSION_NAME}
        gRPC::grpc++
    )
endif()

target_link_libraries(${LOADABLE_EXTENSION_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto
    Arrow::arrow_static
    protobuf::libprotobuf
    ${CMAKE_DL_LIBS}
)

# Add complex dependencies only on Unix systems  
if(NOT WIN32)
    target_link_libraries(${LOADABLE_EXTENSION_NAME}
        gRPC::grpc++
    )
endif()

# Link filesystem library if needed
if(NOT APPLE)
    target_link_libraries(${EXTENSION_NAME} stdc++fs)
    target_link_libraries(${LOADABLE_EXTENSION_NAME} stdc++fs)
endif()

# No RPATH configuration needed - using dynamic path resolution in code

# Define path to Snowflake ADBC shared library for dynamic loading
target_compile_definitions(${EXTENSION_NAME} PRIVATE
    -DSNOWFLAKE_ADBC_LIB="${SNOWFLAKE_ADBC_LIB}"
)
target_compile_definitions(${LOADABLE_EXTENSION_NAME} PRIVATE
    -DSNOWFLAKE_ADBC_LIB="${SNOWFLAKE_ADBC_LIB}"
)

# Add ADBC availability flag
target_compile_definitions(${EXTENSION_NAME} PRIVATE -DADBC_AVAILABLE=1)
target_compile_definitions(${LOADABLE_EXTENSION_NAME} PRIVATE -DADBC_AVAILABLE=1)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

