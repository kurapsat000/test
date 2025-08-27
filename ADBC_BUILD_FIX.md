# ADBC Build Fix for DuckDB Snowflake Extension

## The Problem

The build was failing with:

```
fatal error: arrow-adbc/adbc.h: No such file or directory
    6 | #include <arrow-adbc/adbc.h>
```

Then after adding Arrow features, it failed with:

```
CMake Error: The link interface of target "Arrow::arrow_static" contains:
    protobuf::libprotobuf
but the target was not found.
```

## Root Cause Analysis

1. **ADBC is part of Arrow Flight**: Arrow Database Connectivity (ADBC) headers are included in the Arrow C++ library, specifically in the Arrow package with Flight features.

2. **Missing Arrow Features**: Our original `vcpkg.json` only included a basic arrow dependency without the required features.

3. **Missing Transitive Dependencies**: Arrow's flight feature requires additional dependencies that weren't explicitly listed:

   - `abseil`: Abseil C++ library
   - `c-ares`: DNS resolution library
   - `grpc`: gRPC framework
   - `protobuf`: Protocol Buffers

4. **CMake Dependency Resolution Order**: Arrow's CMake config expects its dependencies to be found before Arrow itself is found.

## The Complete Solution

### 1. Updated vcpkg.json Dependencies

```json
{
  "dependencies": [
    {
      "name": "arrow",
      "features": ["flight"]
    },
    "abseil",
    "c-ares",
    "grpc",
    "protobuf",
    "openssl"
  ],
  "vcpkg-configuration": {
    "overlay-ports": ["./extension-ci-tools/vcpkg_ports"]
  }
}
```

### 2. Fixed CMake Dependency Order

Updated CMakeLists.txt to find Arrow dependencies before Arrow:

```cmake
# Find Arrow dependencies first to ensure proper linking
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(c-ares CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Find Arrow after its dependencies are resolved
find_package(Arrow CONFIG REQUIRED)
```

### 3. Explicit Dependency Linking

Added explicit linking to ensure all dependencies are available:

```cmake
target_link_libraries(${EXTENSION_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto
    Arrow::arrow_static
    protobuf::libprotobuf
    gRPC::grpc++
    ${CMAKE_DL_LIBS}
)
```

## Why This Works

- **`flight` feature**: Provides the ADBC headers (`arrow-adbc/adbc.h`)
- **Explicit dependencies**: Ensures all transitive dependencies are properly resolved:
  - `protobuf`: Protocol Buffers for Arrow Flight serialization
  - `grpc`: Core gRPC framework for Arrow Flight RPC
  - `abseil`: Required by gRPC and Arrow Flight
  - `c-ares`: DNS resolution for network operations
  - `openssl`: SSL/TLS support
- **Correct find_package order**: Ensures CMake can resolve all Arrow dependencies before configuring Arrow
- **Explicit linking**: Prevents missing symbol errors by explicitly linking required libraries

## Build Error Resolution

This fix resolves these specific errors:

- ✅ `fatal error: arrow-adbc/adbc.h: No such file or directory`
- ✅ `protobuf::libprotobuf target was not found`
- ✅ Missing gRPC targets
- ✅ Missing Abseil targets
- ✅ CMake dependency resolution failures
- ✅ Cross-platform dependency resolution issues

## Alternative Solutions Considered

1. **Using nanoarrow**: The new Arrow community extension uses nanoarrow, but our extension specifically needs ADBC functionality which requires the full Arrow Flight implementation.

2. **Minimal features**: Could use only flight feature, but explicit dependencies ensure robust builds.

3. **System libraries**: Could work but would break vcpkg-based build system.

## References

- [Apache Arrow vcpkg package](https://vcpkg.io/en/package/arrow.html)
- [Arrow Flight feature dependencies](https://vcpkg.io/en/package/arrow.html#features)
- [DuckDB extension CI tools](https://github.com/duckdb/extension-ci-tools)
- [Apache Arrow Flight documentation](https://arrow.apache.org/docs/format/Flight.html)
- [CMake find_package documentation](https://cmake.org/cmake/help/latest/command/find_package.html)
