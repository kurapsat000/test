# Build Fixes for DuckDB Snowflake Extension

This document outlines the fixes implemented for WASM build issues, Ninja build tool configuration, missing dependencies, Python formatting tools, and Windows package compatibility across different operating systems.

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
- **Windows**: Simplified to only essential packages available in Chocolatey

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

### 5. Missing Python Dependencies

**Problem**: Code formatting was failing due to missing Python packages:

```
you need to run `pip install "black>=24"` [Errno 2] No such file or directory: 'black'
```

**Solution**:

- Added Python setup step with `actions/setup-python@v4`
- Installed required Python packages: `pip install "black>=24"`
- Added dedicated format check job with proper Python environment

### 6. Ubuntu 24.04 Package Compatibility

**Problem**: Some packages don't exist in Ubuntu 24.04 (Noble):

```
E: Unable to locate package libtinfo5
```

**Solution**:

- Removed `libtinfo5` from package lists (not available in Ubuntu 24.04)
- Updated package lists to be compatible with Ubuntu 24.04
- Used `libncursesw5-dev` instead for terminal handling

### 7. Windows Chocolatey Package Issues

**Problem**: Many packages don't exist in the Chocolatey repository:

```
Failures:
 - gdbm - gdbm not installed. The package was not found with the source(s) listed.
 - libffi - libffi not installed. The package was not found with the source(s) listed.
 - ncurses - ncurses not installed. The package was not found with the source(s) listed.
```

**Solution**:

- Simplified Windows package installation to only essential packages available in Chocolatey
- Removed packages that don't exist in Chocolatey repository
- Used only: `cmake`, `ninja`, `winflexbison3`

### 8. CMake Toolchain Path Issues

**Problem**: CMake couldn't find the toolchain file due to malformed path:

```
Could not find toolchain file: WD/vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Solution**:

- Added path verification steps to debug toolchain path issues
- Ensured proper environment variable construction
- Added debugging output to verify toolchain file existence

## Workflow Changes

### MainDistributionPipeline.yml

1. **Excluded problematic WASM builds** from the main distribution pipeline temporarily
2. **Added custom comprehensive build job** with all required dependencies
3. **Added custom WASM build job** with proper toolchain setup
4. **Added custom format check job** with proper Python setup
5. **Added toolchain path verification** for debugging
6. **Implemented proper environment variable configuration** for all builds

### custom-build.yml

1. **Added comprehensive dependency installation** for all platforms
2. **Fixed WASM build process** with explicit CMake configuration
3. **Improved environment variable handling** for build tools
4. **Added compiler configuration** for all platforms
5. **Added Python dependencies** for code formatting
6. **Simplified Windows package installation** to avoid Chocolatey issues

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

### Python Format Check Configuration

```yaml
steps:
  - name: Setup Python
    uses: actions/setup-python@v4
    with:
      python-version: "3.9"

  - name: Install Python dependencies
    run: |
      pip install "black>=24"
```

## Platform-Specific Setup

### Linux (Ubuntu 24.04)

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
# Simplified to only essential packages available in Chocolatey
choco install cmake ninja winflexbison3
```

## WASM Build Process

The WASM build process now follows these steps:

1. **Setup Emscripten** using `mymindstorm/setup-emsdk@v13`
2. **Install build tools and dependencies** including Ninja, Make, flex, bison, and all required libraries
3. **Setup vcpkg** with proper toolchain configuration
4. **Verify toolchain path** to ensure proper file location
5. **Configure CMake** with explicit generator and make program
6. **Build using emcmake and emmake** wrappers

## Testing

To test the fixes:

1. **Standard builds**: Run the main distribution pipeline
2. **Comprehensive builds**: Run the custom comprehensive build job
3. **WASM builds**: Run the custom WASM build job
4. **Format checks**: Run the custom format check job
5. **Cross-platform**: Verify builds work on Linux, macOS, and Windows

## Future Improvements

1. **Re-enable WASM builds** in main distribution pipeline once upstream fixes are available
2. **Add more comprehensive testing** for WASM builds
3. **Optimize build times** with better caching strategies
4. **Automate dependency detection** to prevent missing package issues
5. **Add more Python tools** for comprehensive code quality checks
6. **Improve Windows package management** with alternative package managers

## Troubleshooting

### Common Issues

1. **Ninja not found**: Ensure Ninja is installed for the target platform
2. **WASM build fails**: Check that Emscripten is properly configured
3. **CMake generator issues**: Verify the generator is explicitly set for WASM builds
4. **Missing flex/bison**: Ensure flex and bison are installed
5. **Compiler not found**: Set CC and CXX environment variables explicitly
6. **Python black not found**: Install black with `pip install "black>=24"`
7. **Package not found**: Check Ubuntu version compatibility (e.g., `libtinfo5` not in Ubuntu 24.04)
8. **Windows package not found**: Use only packages available in Chocolatey repository
9. **Toolchain path issues**: Verify VCPKG_TOOLCHAIN_PATH environment variable

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

# Check Python and black installation
python3 --version
black --version

# Check Ubuntu version
lsb_release -a

# Check toolchain path
echo $VCPKG_TOOLCHAIN_PATH
ls -la "$VCPKG_TOOLCHAIN_PATH"

# Check Windows Chocolatey packages
choco list --local-only
```

## References

- [DuckDB Extension CI Tools](https://github.com/duckdb/extension-ci-tools)
- [Emscripten Documentation](https://emscripten.org/docs/getting_started/index.html)
- [CMake Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)
- [Ninja Build System](https://ninja-build.org/)
- [vcpkg Troubleshooting](https://learn.microsoft.com/vcpkg/troubleshoot/build-failures)
- [Black Code Formatter](https://black.readthedocs.io/)
- [Ubuntu 24.04 Package Changes](https://discourse.ubuntu.com/t/noble-numbat-24-04-lts-release-notes/38247)
- [Chocolatey Package Repository](https://community.chocolatey.org/packages)
