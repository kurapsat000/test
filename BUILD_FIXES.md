# Build Fixes for DuckDB Snowflake Extension

This document outlines the fixes implemented for WASM build issues and Ninja build tool configuration across different operating systems.

## Issues Fixed

### 1. WASM Build Issues

**Problem**: The WASM build was failing with the following error:

```
CMake Error: CMake was unable to find a build program corresponding to "Unix Makefiles".
CMAKE_MAKE_PROGRAM is not set. You probably need to select a different build tool.
```

**Root Cause**: The WASM build process was not properly configuring the CMake generator and make program for Emscripten builds.

**Solution**:

- Added explicit CMake generator configuration (`-G "Unix Makefiles"`)
- Set the make program explicitly (`-DCMAKE_MAKE_PROGRAM=make`)
- Ensured proper Emscripten environment setup with `source $EMSDK/emsdk_env.sh`
- Used `emcmake` and `emmake` wrappers for proper toolchain integration

### 2. Ninja Build Tool Issues

**Problem**: Ninja build tool was not consistently available across all platforms, causing build failures.

**Solution**:

- **Linux**: Added `ninja-build` package installation via `apt-get`
- **macOS**: Added `ninja` package installation via `brew`
- **Windows**: Added `ninja` package installation via `choco`
- Set `GEN: "ninja"` environment variable for all non-WASM builds

## Workflow Changes

### MainDistributionPipeline.yml

1. **Excluded problematic WASM builds** from the main distribution pipeline temporarily
2. **Added custom WASM build job** with proper toolchain setup
3. **Implemented proper environment variable configuration** for WASM builds

### custom-build.yml

1. **Added Ninja installation** for all platforms
2. **Fixed WASM build process** with explicit CMake configuration
3. **Improved environment variable handling** for build tools

## Build Configuration

### WASM Build Configuration

```yaml
env:
  VCPKG_TARGET_TRIPLET: wasm32-emscripten
  VCPKG_HOST_TRIPLET: x64-linux
  DUCKDB_PLATFORM: ${{ matrix.wasm_platform }}
  GEN: "Unix Makefiles"
  CMAKE_MAKE_PROGRAM: "make"
```

### Standard Build Configuration

```yaml
env:
  VCPKG_TOOLCHAIN_PATH: "$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
  GEN: "ninja"
```

## Platform-Specific Setup

### Linux

```bash
sudo apt-get install -y build-essential cmake git ninja-build
```

### macOS

```bash
brew install cmake ninja
```

### Windows

```bash
choco install cmake ninja
```

## WASM Build Process

The WASM build process now follows these steps:

1. **Setup Emscripten** using `mymindstorm/setup-emsdk@v13`
2. **Install build tools** including Ninja and Make
3. **Setup vcpkg** with proper toolchain configuration
4. **Configure CMake** with explicit generator and make program
5. **Build using emcmake and emmake** wrappers

## Testing

To test the fixes:

1. **Standard builds**: Run the main distribution pipeline
2. **WASM builds**: Run the custom WASM build job
3. **Cross-platform**: Verify builds work on Linux, macOS, and Windows

## Future Improvements

1. **Re-enable WASM builds** in main distribution pipeline once upstream fixes are available
2. **Add more comprehensive testing** for WASM builds
3. **Optimize build times** with better caching strategies

## Troubleshooting

### Common Issues

1. **Ninja not found**: Ensure Ninja is installed for the target platform
2. **WASM build fails**: Check that Emscripten is properly configured
3. **CMake generator issues**: Verify the generator is explicitly set for WASM builds

### Debug Commands

```bash
# Check Ninja installation
ninja --version

# Check Emscripten installation
emcc --version

# Check CMake generator
cmake --help
```

## References

- [DuckDB Extension CI Tools](https://github.com/duckdb/extension-ci-tools)
- [Emscripten Documentation](https://emscripten.org/docs/getting_started/index.html)
- [CMake Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)
- [Ninja Build System](https://ninja-build.org/)
