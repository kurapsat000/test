# Disk Space Optimization for DuckDB Snowflake Extension

## The Problem

The GitHub Actions build was failing with:

```
fatal: write error: No space left on device
fatal: fetch-pack: invalid index-pack output
```

This error occurs during vcpkg installation in Docker containers when the available disk space is exhausted.

## Root Cause Analysis

1. **Limited Disk Space**: GitHub-hosted runners have ~25-29 GB total disk space, with only ~14-21 GB available for builds.

2. **Heavy Dependencies**: Our extension requires:

   - vcpkg with Arrow, gRPC, Protobuf dependencies
   - Docker images for multi-platform builds
   - Large C++ compilation artifacts
   - Debug symbols and intermediate files

3. **Resource-Intensive Builds**: ARM64 and WASM builds are particularly resource-heavy:
   - Cross-compilation requires additional toolchains
   - Multiple architecture targets multiply disk usage
   - vcpkg builds all dependencies for each target

## Solutions Implemented

### 1. Exclude Resource-Intensive Architectures

Temporarily excluded builds that consume the most disk space:

```yaml
exclude_archs: "wasm_mvp;wasm_eh;wasm_threads;linux_arm64;linux_arm64_gcc4"
```

**Why this helps**:

- ARM64 builds require cross-compilation toolchains (~3-5 GB)
- WASM builds need Emscripten SDK (~2-3 GB)
- Each architecture needs separate vcpkg dependencies (~1-2 GB each)

### 2. Minimize Build Artifacts

Added `minimal_build: true` to reduce intermediate files:

- Removes debug symbols from final artifacts
- Cleans up temporary build files
- Reduces Docker layer sizes

### 3. Optimized vcpkg Dependencies

Reduced Arrow features to essential only:

```json
{
  "dependencies": [
    {
      "name": "arrow",
      "features": ["flight"]
    },
    "grpc",
    "protobuf",
    "openssl"
  ]
}
```

**Benefits**:

- Removes optional Arrow features (dataset, json, parquet, csv, filesystem)
- Reduces transitive dependencies
- Faster build times and less disk usage

## Alternative Solutions (For Future)

### 1. GitHub Large Runners

- Available with up to 2040 GB storage
- Cost: $0.008-$0.256 per minute
- Requires GitHub Team/Enterprise plan

### 2. Custom Self-Hosted Runners

- AWS EC2 with custom disk sizes (up to 16 TB)
- More cost-effective for large builds
- Requires infrastructure management

### 3. Multi-Stage Builds

- Build each architecture separately
- Use build matrices to parallelize
- Cache vcpkg dependencies between builds

### 4. Disk Space Cleanup Actions

```yaml
- name: Free Disk Space
  uses: easimon/maximize-build-space@master
  with:
    remove-dotnet: "true"
    remove-android: "true"
    remove-haskell: "true"
    remove-codeql: "true"
    remove-docker-images: "true"
```

## Build Space Analysis

Typical GitHub runner disk usage for our extension:

| Component                | Size     | Required      |
| ------------------------ | -------- | ------------- |
| Base OS + tools          | ~45 GB   | ✅            |
| vcpkg + dependencies     | ~8-12 GB | ✅            |
| DuckDB source            | ~2 GB    | ✅            |
| Build artifacts (x86_64) | ~3-5 GB  | ✅            |
| ARM64 cross-compilation  | ~5-8 GB  | ❌ (excluded) |
| WASM toolchain           | ~3-5 GB  | ❌ (excluded) |
| Docker build cache       | ~2-4 GB  | Managed       |

## Current Status

✅ **Fixed**: vcpkg disk space issues
✅ **Optimized**: Dependency configuration  
✅ **Reduced**: Build target scope
⏳ **Monitoring**: Build success rates
🔄 **Future**: Re-enable ARM64/WASM when stable

## Re-enabling Full Builds

Once builds are stable, gradually re-enable excluded architectures:

1. **Phase 1**: Re-enable `linux_arm64`
2. **Phase 2**: Re-enable WASM targets one by one
3. **Phase 3**: Monitor disk usage and optimize further

Monitor build logs for disk space warnings:

```bash
df -h  # Check available space
du -sh /tmp/vcpkg  # Check vcpkg cache size
```

## References

- [GitHub Actions disk space optimization](https://github.com/easimon/maximize-build-space)
- [DuckDB extension CI tools](https://github.com/duckdb/extension-ci-tools)
- [vcpkg disk space management](https://vcpkg.io/en/docs/README.html)
