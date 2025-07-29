# Fixing RPATH Issues for DuckDB Snowflake Extension

## Understanding RPATH

RPATH (Runtime Path) tells the dynamic linker where to find shared libraries at runtime. For DuckDB extensions that depend on external shared libraries (like Snowflake ADBC driver), proper RPATH configuration is crucial.

## Common RPATH Issues and Solutions

### 1. BUILD_RPATH vs INSTALL_RPATH

The current CMakeLists.txt sets both, which is correct:
```cmake
set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
```

However, you might need additional settings:

```cmake
# Enable RPATH for build tree
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# Don't skip RPATH for build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# When building, use the install RPATH
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Add the automatically determined parts to RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# RPATH to be used when installing
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    # For macOS
    INSTALL_RPATH "@loader_path"
)
```

### 2. Platform-Specific RPATH

Different platforms use different RPATH syntax:

```cmake
if(APPLE)
    set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
else()
    set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()
```

### 3. Verify RPATH After Building

Check if RPATH is properly set:

**Linux:**
```bash
readelf -d snowflake.duckdb_extension | grep RPATH
# or
chrpath -l snowflake.duckdb_extension
# or
objdump -x snowflake.duckdb_extension | grep RPATH
```

**macOS:**
```bash
otool -l snowflake.duckdb_extension | grep -A 2 LC_RPATH
```

### 4. Loading Shared Libraries at Runtime

If RPATH still doesn't work, you can:

#### Option 1: Set LD_LIBRARY_PATH (Linux) or DYLD_LIBRARY_PATH (macOS)
```bash
export LD_LIBRARY_PATH=/path/to/extension/directory:$LD_LIBRARY_PATH
```

#### Option 2: Use absolute paths in code
Instead of:
```cpp
#define SNOWFLAKE_ADBC_LIB_PATH "libadbc_driver_snowflake.so"
```

Use:
```cpp
// Get the directory of the current extension
std::string GetExtensionDirectory() {
    Dl_info info;
    if (dladdr((void*)SnowflakeExtension::Load, &info)) {
        std::string path = info.dli_fname;
        return path.substr(0, path.find_last_of("/"));
    }
    return ".";
}

// Load with full path
std::string lib_path = GetExtensionDirectory() + "/libadbc_driver_snowflake.so";
```

### 5. Complete CMakeLists.txt Fix

Here's a comprehensive RPATH configuration:

```cmake
# Set RPATH policies
cmake_policy(SET CMP0042 NEW) # MACOSX_RPATH is enabled by default
cmake_policy(SET CMP0068 NEW) # RPATH settings are used for build tree

# RPATH configuration
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Platform-specific RPATH
if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
    set(CMAKE_BUILD_RPATH "@loader_path")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_RPATH "$ORIGIN")
endif()

# Apply to target
set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
    SKIP_BUILD_RPATH FALSE
    BUILD_WITH_INSTALL_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Platform-specific target properties
if(APPLE)
    set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
        MACOSX_RPATH TRUE
    )
else()
    set_target_properties(${LOADABLE_EXTENSION_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()
```

### 6. Alternative: Static Linking

If RPATH issues persist, consider statically linking the ADBC driver:

```cmake
# Find static library instead
find_library(ADBC_SNOWFLAKE_STATIC 
    NAMES libadbc_driver_snowflake.a adbc_driver_snowflake_static
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/arrow-adbc/build
)

target_link_libraries(${LOADABLE_EXTENSION_NAME} 
    ${ADBC_SNOWFLAKE_STATIC}
    # ... other libs
)
```

### 7. Debug Loading Issues

Add debug output to see where libraries are being searched:

```bash
# Linux
LD_DEBUG=libs duckdb -c "LOAD 'snowflake.duckdb_extension';"

# macOS
DYLD_PRINT_LIBRARIES=1 duckdb -c "LOAD 'snowflake.duckdb_extension';"
```

## Testing RPATH

Create a test script:

```bash
#!/bin/bash
# test-rpath.sh

# Copy extension and dependencies to test directory
mkdir -p test_rpath
cp build/release/extension/snowflake/snowflake.duckdb_extension test_rpath/
cp build/release/libadbc_driver_snowflake.so test_rpath/

# Test loading from different directory
cd /tmp
duckdb -c "LOAD '/path/to/test_rpath/snowflake.duckdb_extension'; SELECT 1;"
```

If this works, RPATH is configured correctly.