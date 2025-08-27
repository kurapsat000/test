# Build Fixes for DuckDB Snowflake Extension

This document outlines the fixes implemented for WASM build issues, Ninja build tool configuration, and missing dependencies across different operating systems.

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

### 3. Missing Dependencies (flex, bison, etc.)

**Problem**: The build was failing due to missing dependencies required by vcpkg packages:

```
Could not find flex. Please install it via your package manager:
    sudo apt-get install flex
```

**Root Cause**: The thrift package and other dependencies require additional system packages that weren't installed.

**Solution**:

- **Linux**: Added comprehensive package installation including `flex`, `bison`, `libssl-dev`, `zlib1g-dev`, etc.
- **macOS**: Added `flex`, `bison`, `openssl`, `zlib`, and other required packages via `brew`
- **Windows**: Added `winflexbison3` and other packages via `choco`

### 4. Compiler Configuration Issues

**Problem**: CMake couldn't find the C and C++ compilers:

```
CMake Error: CMAKE_C_COMPILER not set, after EnableLanguage
CMake Error: CMAKE_CXX_COMPILER not set, after EnableLanguage
```

**Solution**:

- Set explicit compiler environment variables:
  - **Linux**: `CC: "gcc"`, `CXX: "g++"`
  - **macOS**: `CC: "clang"`, `CXX: "clang++"`
  - **Windows**: `CC: "cl"`, `CXX: "cl"`
- Set `CMAKE_MAKE_PROGRAM: "ninja"` for proper build tool configuration

## Workflow Changes

### MainDistributionPipeline.yml

1. **Excluded problematic WASM builds** from the main distribution pipeline temporarily
2. **Added custom comprehensive build job** with all required dependencies
3. **Added custom WASM build job** with proper toolchain setup
4. **Implemented proper environment variable configuration** for all builds

### custom-build.yml

1. **Added comprehensive dependency installation** for all platforms
2. **Fixed WASM build process** with explicit CMake configuration
3. **Improved environment variable handling** for build tools
4. **Added compiler configuration** for all platforms

## Build Configuration

### Standard Build Configuration

```yaml
env:
  VCPKG_TOOLCHAIN_PATH: "$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
  GEN: "ninja"
  CC: "gcc" # or "clang" for macOS, "cl" for Windows
  CXX: "g++" # or "clang++" for macOS, "cl" for Windows
  CMAKE_MAKE_PROGRAM: "ninja"
```

### WASM Build Configuration

```yaml
env:
  VCPKG_TARGET_TRIPLET: wasm32-emscripten
  VCPKG_HOST_TRIPLET: x64-linux
  DUCKDB_PLATFORM: ${{ matrix.wasm_platform }}
  GEN: "Unix Makefiles"
  CMAKE_MAKE_PROGRAM: "make"
```

## Platform-Specific Setup

### Linux

```bash
sudo apt-get install -y \
  build-essential \
  cmake \
  git \
  ninja-build \
  flex \
  bison \
  libssl-dev \
  zlib1g-dev \
  libbz2-dev \
  libreadline-dev \
  libsqlite3-dev \
  wget \
  curl \
  libffi-dev \
  liblzma-dev \
  libgdbm-dev \
  libnss3-dev \
  libncursesw5-dev \
  libtinfo5 \
  libc6-dev \
  libgcc-s1 \
  gcc \
  g++ \
  make \
  pkg-config
```

### macOS

```bash
brew install cmake ninja flex bison openssl zlib bzip2 readline sqlite3 wget curl libffi xz gdbm nss ncurses pkg-config
```

### Windows

```bash
choco install cmake ninja winflexbison3 openssl zlib bzip2 readline sqlite3 wget curl libffi xz gdbm nss ncurses pkg-config
```

## WASM Build Process

The WASM build process now follows these steps:

1. **Setup Emscripten** using `mymindstorm/setup-emsdk@v13`
2. **Install build tools and dependencies** including Ninja, Make, flex, bison, and all required libraries
3. **Setup vcpkg** with proper toolchain configuration
4. **Configure CMake** with explicit generator and make program
5. **Build using emcmake and emmake** wrappers

## Testing

To test the fixes:

1. **Standard builds**: Run the main distribution pipeline
2. **Comprehensive builds**: Run the custom comprehensive build job
3. **WASM builds**: Run the custom WASM build job
4. **Cross-platform**: Verify builds work on Linux, macOS, and Windows

## Future Improvements

1. **Re-enable WASM builds** in main distribution pipeline once upstream fixes are available
2. **Add more comprehensive testing** for WASM builds
3. **Optimize build times** with better caching strategies
4. **Automate dependency detection** to prevent missing package issues

## Troubleshooting

### Common Issues

1. **Ninja not found**: Ensure Ninja is installed for the target platform
2. **WASM build fails**: Check that Emscripten is properly configured
3. **CMake generator issues**: Verify the generator is explicitly set for WASM builds
4. **Missing flex/bison**: Ensure flex and bison are installed
5. **Compiler not found**: Set CC and CXX environment variables explicitly

### Debug Commands

```bash
# Check Ninja installation
ninja --version

# Check Emscripten installation
emcc --version

# Check CMake generator
cmake --help

# Check flex/bison installation
flex --version
bison --version

# Check compiler availability
gcc --version
g++ --version
```

## References

- [DuckDB Extension CI Tools](https://github.com/duckdb/extension-ci-tools)
- [Emscripten Documentation](https://emscripten.org/docs/getting_started/index.html)
- [CMake Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)
- [Ninja Build System](https://ninja-build.org/)
- [vcpkg Troubleshooting](https://learn.microsoft.com/vcpkg/troubleshoot/build-failures)
