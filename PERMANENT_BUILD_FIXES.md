# Simplified Build Fixes for DuckDB Snowflake Extension

This document outlines the simplified approach to resolve build issues by using the standard DuckDB extension workflow instead of complex custom solutions.

## Issues Addressed

### 1. Complex Custom Build Solutions

**Problem**: Previous attempts used complex custom Docker containers and build scripts that were difficult to maintain and debug.

**Root Cause**: Over-engineering the build process instead of using the proven standard DuckDB extension workflow.

**Simplified Solution**: Use the standard DuckDB extension distribution workflow with minimal customizations.

### 2. Dependency Management Complexity

**Problem**: Manual dependency installation and complex package lists across different platforms.

**Root Cause**: Trying to manage all dependencies manually instead of leveraging vcpkg's automatic dependency resolution.

**Simplified Solution**: Let vcpkg handle dependency management automatically through the standard workflow.

## Simplified Approach

### 1. Standard Workflow Usage

The extension now uses the standard DuckDB extension distribution workflow:

```yaml
# .github/workflows/MainDistributionPipeline.yml
jobs:
  duckdb-next-build:
    name: Build extension binaries
    uses: duckdb/extension-ci-tools/.github/workflows/_extension_distribution.yml@main
    with:
      duckdb_version: main
      ci_tools_version: main
      extension_name: snowflake
      extra_toolchains: "go"
      exclude_archs: "wasm_mvp;wasm_eh;wasm_threads"
```

### 2. Minimal vcpkg Configuration

The `vcpkg.json` file contains only essential dependencies:

```json
{
  "dependencies": ["openssl", "arrow"],
  "vcpkg-configuration": {
    "overlay-ports": ["./extension-ci-tools/vcpkg_ports"]
  }
}
```

### 3. Simplified Docker Containers

Docker containers now only install essential build tools:

```dockerfile
# Install essential build dependencies
RUN yum install -y \
    flex \
    bison \
    gcc \
    gcc-c++ \
    make \
    cmake3 \
    pkgconfig
```

### 4. Simple Local Build Script

For local development, use the simple build script:

```bash
# scripts/simple_build.sh
#!/bin/bash
export GEN=ninja
export DUCKDB_EXTENSIONS=snowflake
make release
```

## Key Benefits of Simplified Approach

### 1. Reliability

- **Proven workflow**: Uses the same workflow as all other DuckDB extensions
- **Automatic dependency resolution**: vcpkg handles all dependencies automatically
- **Standard tooling**: Leverages the same tools and processes as the main DuckDB project

### 2. Maintainability

- **Minimal custom code**: Fewer custom scripts and configurations to maintain
- **Standard practices**: Follows DuckDB extension development best practices
- **Clear documentation**: Standard workflow is well-documented and understood

### 3. Compatibility

- **Cross-platform**: Works on all platforms supported by DuckDB
- **Version compatibility**: Automatically compatible with different DuckDB versions
- **Tool compatibility**: Works with standard DuckDB development tools

### 4. Debugging

- **Standard error messages**: Errors follow familiar patterns
- **Community support**: Issues can be resolved using standard DuckDB extension knowledge
- **Clear logs**: Standard workflow provides clear, actionable error messages

## Usage

### CI/CD Builds

The extension automatically builds using the standard workflow on:

- Push to main branch
- Pull requests
- Manual workflow dispatch

### Local Development

```bash
# Simple build
./scripts/simple_build.sh

# Or use standard DuckDB extension commands
export GEN=ninja
export DUCKDB_EXTENSIONS=snowflake
make release
```

### Testing

```bash
# Run tests
make test_release
```

## Dependencies

The extension requires minimal dependencies that are automatically managed:

### Runtime Dependencies

- **OpenSSL**: For secure connections to Snowflake
- **Arrow**: For efficient data transfer

### Build Dependencies

- **flex/bison**: For parser generation (handled by vcpkg)
- **CMake**: Build system (provided by Docker containers)
- **Ninja**: Build tool (provided by Docker containers)

## Troubleshooting

### Common Issues

1. **Build fails with missing dependencies**:

   - The standard workflow automatically installs all required dependencies
   - Check that `vcpkg.json` contains the correct dependencies

2. **Local build issues**:

   - Use the simple build script: `./scripts/simple_build.sh`
   - Ensure you have the standard DuckDB development environment

3. **CI/CD failures**:
   - Check the standard DuckDB extension workflow logs
   - Issues are typically related to DuckDB version compatibility

### Debug Commands

```bash
# Check environment
echo "GEN: $GEN"
echo "DUCKDB_EXTENSIONS: $DUCKDB_EXTENSIONS"

# Check build tools
which cmake ninja make

# Check vcpkg
ls -la vcpkg.json
```

## Migration from Complex Approach

If you were using the previous complex build approach:

1. **Remove custom build scripts**: Delete `scripts/robust_build.sh`
2. **Use standard workflow**: The simplified workflow is already in place
3. **Update local development**: Use `scripts/simple_build.sh` for local builds
4. **Remove custom dependencies**: Let vcpkg handle dependency management

## Future Improvements

1. **Standard extension practices**: Follow all DuckDB extension development guidelines
2. **Community contributions**: Use standard processes for accepting contributions
3. **Documentation**: Maintain standard DuckDB extension documentation practices
4. **Testing**: Use standard DuckDB extension testing practices

## References

- [DuckDB Extension Development Guide](https://duckdb.org/docs/extensions/overview)
- [DuckDB Extension Template](https://github.com/duckdb/extension-template)
- [DuckDB Extension CI Tools](https://github.com/duckdb/extension-ci-tools)
- [vcpkg Documentation](https://github.com/microsoft/vcpkg)
