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

4. **vcpkg Dependency Resolution**: While vcpkg should automatically install transitive dependencies, explicitly listing them ensures proper dependency resolution across all platforms.

## The Complete Solution

Updated `vcpkg.json` to include Arrow with features AND all required dependencies:

```json
{
  "dependencies": [
    {
      "name": "arrow",
      "features": ["flight", "flightsql"]
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

## Why This Works

- **`flight` feature**: Provides the ADBC headers (`arrow-adbc/adbc.h`)
- **`flightsql` feature**: Provides FlightSQL ADBC functionality
- **Explicit dependencies**: Ensures all transitive dependencies are properly resolved:
  - `abseil`: Required by gRPC and Arrow Flight
  - `c-ares`: DNS resolution for network operations
  - `grpc`: Core gRPC framework for Arrow Flight
  - `protobuf`: Protocol serialization
  - `openssl`: SSL/TLS support

## Build Error Resolution

This fix resolves these specific errors:

- ✅ `fatal error: arrow-adbc/adbc.h: No such file or directory`
- ✅ `protobuf::libprotobuf target was not found`
- ✅ Missing gRPC targets
- ✅ Missing Abseil targets
- ✅ Cross-platform dependency resolution

## Alternative Solutions Considered

1. **Using nanoarrow**: The new Arrow community extension uses nanoarrow, but our extension specifically needs ADBC functionality which requires the full Arrow Flight implementation.

2. **Minimal features**: Could use only flight feature, but explicit dependencies ensure robust builds.

3. **System libraries**: Could work but would break vcpkg-based build system.

## References

- [Apache Arrow vcpkg package](https://vcpkg.io/en/package/arrow.html)
- [Arrow Flight feature dependencies](https://vcpkg.io/en/package/arrow.html#features)
- [DuckDB extension CI tools](https://github.com/duckdb/extension-ci-tools)
- [Apache Arrow Flight documentation](https://arrow.apache.org/docs/format/Flight.html)
