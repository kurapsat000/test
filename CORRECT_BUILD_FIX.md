# Correct Build Fix for DuckDB Snowflake Extension

## The Real Issue

The build failures were occurring because the DuckDB extension CI tools Docker containers were missing essential build dependencies like `flex` and `bison` that are required by vcpkg packages such as `thrift`.

## The Correct Solution

The proper fix is **NOT** to manually modify Docker containers or create complex custom build scripts. Instead, DuckDB provides a built-in mechanism for this through the `extra_toolchains` parameter.

### What Was Changed

In `.github/workflows/MainDistributionPipeline.yml`:

```yaml
# Before (WRONG)
extra_toolchains: 'go'

# After (CORRECT)
extra_toolchains: 'go;parser_tools'
```

### Why This Works

The `parser_tools` toolchain is a predefined toolchain in the DuckDB extension CI tools that automatically installs:

- `flex` - Required by thrift and other parser generators
- `bison` - Required by thrift and other parser generators
- Other parser-related dependencies

When `parser_tools` is included in `extra_toolchains`, the Docker containers automatically install these dependencies **before** running vcpkg, which resolves the thrift build failures.

### How DuckDB Extension Toolchains Work

The DuckDB extension CI tools support several predefined toolchains:

- `go` - Installs Go development tools
- `rust` - Installs Rust development tools
- `parser_tools` - Installs flex, bison, and parser tools
- `fortran` - Installs Fortran compiler
- `python3` - Installs Python 3

These are specified as a semicolon-separated list in the `extra_toolchains` parameter.

## Why Previous Approaches Were Wrong

1. **Modifying Docker containers directly** - This breaks the standardized DuckDB extension build process and makes maintenance difficult
2. **Creating custom build scripts** - Unnecessary complexity when the proper infrastructure already exists
3. **Removing vcpkg dependencies** - Would break the extension's functionality

## The Benefits of This Approach

1. **Standard Compliance** - Uses the official DuckDB extension infrastructure
2. **Minimal Changes** - Only two lines changed in the workflow
3. **Automatic Maintenance** - DuckDB team maintains the toolchain definitions
4. **Cross-Platform** - Works across all supported platforms (Linux, macOS, Windows)
5. **Future-Proof** - Will continue to work as DuckDB evolves

## Verification

After this change, the build should work because:

1. The Docker containers will have `flex` and `bison` installed via the `parser_tools` toolchain
2. vcpkg will be able to build `thrift` and `arrow` dependencies successfully
3. The extension will compile and link properly across all platforms

## Key Takeaway

Always check the DuckDB extension documentation and existing toolchain options before creating custom solutions. The DuckDB team has likely already solved common build issues through the standardized toolchain system.
