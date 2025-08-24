cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME snowflake)
project(${TARGET_NAME})
include_directories(src/include)

# Set C++17 standard for std::filesystem support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use system packages instead of vcpkg for better CI compatibility
find_package(OpenSSL REQUIRED)

# Try to find Arrow and ADBC from system packages first
find_package(PkgConfig REQUIRED)

# Find Arrow
pkg_check_modules(ARROW arrow)
if(ARROW_FOUND)
    message(STATUS "Found Arrow via pkg-config: ${ARROW_LIBRARIES}")
else()
    # Fallback to find_package if pkg-config doesn't work
    find_package(Arrow CONFIG REQUIRED)
    if(Arrow_FOUND)
        set(ARROW_LIBRARIES Arrow::arrow_static)
        message(STATUS "Found Arrow via find_package")
    endif()
endif()

# Find ADBC Driver Manager
pkg_check_modules(ADBC_DRIVER_MANAGER adbc-driver-manager)
if(ADBC_DRIVER_MANAGER_FOUND)
    message(STATUS "Found ADBC Driver Manager via pkg-config: ${ADBC_DRIVER_MANAGER_LIBRARIES}")
else()
    # Fallback to find_package if pkg-config doesn't work
    find_package(AdbcDriverManager CONFIG REQUIRED)
    if(AdbcDriverManager_FOUND)
        set(ADBC_DRIVER_MANAGER_LIBRARIES AdbcDriverManager::adbc_driver_manager_static)
        message(STATUS "Found ADBC Driver Manager via find_package")
    endif()
endif()

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
    ${ARROW_LIBRARIES}
    ${ADBC_DRIVER_MANAGER_LIBRARIES}
    ${CMAKE_DL_LIBS}
)
target_link_libraries(${LOADABLE_EXTENSION_NAME} 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${ARROW_LIBRARIES}
    ${ADBC_DRIVER_MANAGER_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Add include directories
if(ARROW_FOUND)
    target_include_directories(${EXTENSION_NAME} PRIVATE ${ARROW_INCLUDE_DIRS})
    target_include_directories(${LOADABLE_EXTENSION_NAME} PRIVATE ${ARROW_INCLUDE_DIRS})
    # Add link directories for pkg-config libraries
    if(ARROW_LIBRARY_DIRS)
        target_link_directories(${EXTENSION_NAME} PRIVATE ${ARROW_LIBRARY_DIRS})
        target_link_directories(${LOADABLE_EXTENSION_NAME} PRIVATE ${ARROW_LIBRARY_DIRS})
    endif()
endif()
if(ADBC_DRIVER_MANAGER_FOUND)
    target_include_directories(${EXTENSION_NAME} PRIVATE ${ADBC_DRIVER_MANAGER_INCLUDE_DIRS})
    target_include_directories(${LOADABLE_EXTENSION_NAME} PRIVATE ${ADBC_DRIVER_MANAGER_INCLUDE_DIRS})
    # Add link directories for pkg-config libraries
    if(ADBC_DRIVER_MANAGER_LIBRARY_DIRS)
        target_link_directories(${EXTENSION_NAME} PRIVATE ${ADBC_DRIVER_MANAGER_LIBRARY_DIRS})
        target_link_directories(${LOADABLE_EXTENSION_NAME} PRIVATE ${ADBC_DRIVER_MANAGER_LIBRARY_DIRS})
    endif()
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

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
