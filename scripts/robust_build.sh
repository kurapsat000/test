#!/bin/bash

# Robust Build Script for DuckDB Snowflake Extension
# This script handles dependency installation, build configuration, and fallback options

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS and package manager
detect_system() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if command -v apt-get &> /dev/null; then
            PACKAGE_MANAGER="apt"
            INSTALL_CMD="sudo apt-get update && sudo apt-get install -y"
        elif command -v yum &> /dev/null; then
            PACKAGE_MANAGER="yum"
            INSTALL_CMD="sudo yum install -y"
        elif command -v dnf &> /dev/null; then
            PACKAGE_MANAGER="dnf"
            INSTALL_CMD="sudo dnf install -y"
        elif command -v apk &> /dev/null; then
            PACKAGE_MANAGER="apk"
            INSTALL_CMD="sudo apk add"
        else
            log_error "Unsupported Linux distribution"
            exit 1
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PACKAGE_MANAGER="brew"
        INSTALL_CMD="brew install"
    else
        log_error "Unsupported operating system: $OSTYPE"
        exit 1
    fi
    
    log_info "Detected package manager: $PACKAGE_MANAGER"
}

# Install essential dependencies
install_dependencies() {
    log_info "Installing essential build dependencies..."
    
    case $PACKAGE_MANAGER in
        "apt")
            eval $INSTALL_CMD \
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
                pkg-config \
                python3 \
                python3-pip
            ;;
        "yum"|"dnf")
            eval $INSTALL_CMD \
                flex \
                bison \
                libssl-devel \
                zlib-devel \
                bzip2-devel \
                readline-devel \
                sqlite-devel \
                libffi-devel \
                xz-devel \
                gdbm-devel \
                nss-devel \
                ncurses-devel \
                pkgconfig \
                gcc \
                gcc-c++ \
                make \
                cmake3 \
                openssl-devel \
                libcurl-devel \
                expat-devel \
                libxml2-devel \
                libuuid-devel \
                libtool \
                automake \
                gettext-devel \
                python3-devel \
                python3-pip \
                boost-devel \
                gmp-devel \
                mpfr-devel \
                mpc-devel \
                isl-devel \
                cloog-devel \
                glibc-devel \
                glibc-headers \
                glibc-static \
                libstdc++-devel \
                libstdc++-static
            ;;
        "apk")
            eval $INSTALL_CMD \
                flex \
                bison \
                libssl-dev \
                zlib-dev \
                bzip2-dev \
                readline-dev \
                sqlite-dev \
                libffi-dev \
                xz-dev \
                gdbm-dev \
                nss-dev \
                ncurses-dev \
                pkgconfig \
                gcc \
                g++ \
                make \
                cmake \
                openssl-dev \
                curl-dev \
                expat-dev \
                libxml2-dev \
                util-linux-dev \
                libtool \
                automake \
                gettext-dev \
                python3-dev \
                py3-pip \
                boost-dev \
                gmp-dev \
                mpfr-dev \
                mpc-dev \
                isl-dev \
                cloog-dev \
                musl-dev \
                libc-dev \
                libstdc++-dev \
                linux-headers \
                pcre-dev \
                libedit-dev \
                libbsd-dev \
                libexecinfo-dev \
                libatomic \
                libgomp \
                libgcc \
                libstdc++
            ;;
        "brew")
            eval $INSTALL_CMD \
                cmake \
                ninja \
                flex \
                bison \
                openssl \
                zlib \
                bzip2 \
                readline \
                sqlite3 \
                wget \
                curl \
                libffi \
                xz \
                gdbm \
                nss \
                ncurses \
                pkg-config \
                python3
            ;;
    esac
    
    log_success "Dependencies installed successfully"
}

# Install Python dependencies
install_python_deps() {
    log_info "Installing Python dependencies..."
    
    if command -v pip3 &> /dev/null; then
        pip3 install "black>=24"
        log_success "Python dependencies installed"
    elif command -v pip &> /dev/null; then
        pip install "black>=24"
        log_success "Python dependencies installed"
    else
        log_warning "pip not found, skipping Python dependencies"
    fi
}

# Setup vcpkg
setup_vcpkg() {
    log_info "Setting up vcpkg..."
    
    if [ ! -d "vcpkg" ]; then
        git clone https://github.com/Microsoft/vcpkg.git
    fi
    
    cd vcpkg
    ./bootstrap-vcpkg.sh
    cd ..
    
    export VCPKG_ROOT="$PWD/vcpkg"
    export VCPKG_TOOLCHAIN_PATH="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
    export PATH="$VCPKG_ROOT:$PATH"
    
    log_success "vcpkg setup completed"
}

# Configure build environment
configure_build_env() {
    log_info "Configuring build environment..."
    
    # Set build tool
    export GEN="ninja"
    
    # Set compilers
    if command -v gcc &> /dev/null; then
        export CC="gcc"
        export CXX="g++"
    elif command -v clang &> /dev/null; then
        export CC="clang"
        export CXX="clang++"
    else
        log_error "No C/C++ compiler found"
        exit 1
    fi
    
    # Set make program
    if command -v ninja &> /dev/null; then
        export CMAKE_MAKE_PROGRAM="ninja"
    elif command -v make &> /dev/null; then
        export CMAKE_MAKE_PROGRAM="make"
    else
        log_error "No build tool found (ninja or make)"
        exit 1
    fi
    
    log_success "Build environment configured"
    log_info "CC: $CC"
    log_info "CXX: $CXX"
    log_info "GEN: $GEN"
    log_info "CMAKE_MAKE_PROGRAM: $CMAKE_MAKE_PROGRAM"
}

# Verify build tools
verify_build_tools() {
    log_info "Verifying build tools..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found"
        exit 1
    fi
    
    # Check compiler
    if ! command -v $CC &> /dev/null; then
        log_error "C compiler ($CC) not found"
        exit 1
    fi
    
    if ! command -v $CXX &> /dev/null; then
        log_error "C++ compiler ($CXX) not found"
        exit 1
    fi
    
    # Check build tool
    if ! command -v $CMAKE_MAKE_PROGRAM &> /dev/null; then
        log_error "Build tool ($CMAKE_MAKE_PROGRAM) not found"
        exit 1
    fi
    
    # Check vcpkg
    if [ ! -f "$VCPKG_TOOLCHAIN_PATH" ]; then
        log_error "vcpkg toolchain file not found: $VCPKG_TOOLCHAIN_PATH"
        exit 1
    fi
    
    log_success "All build tools verified"
}

# Build the extension
build_extension() {
    log_info "Building extension..."
    
    # Create build directory
    mkdir -p build
    
    # Configure with CMake
    cmake -B build \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN_PATH" \
        -G "$GEN" \
        -DCMAKE_MAKE_PROGRAM="$CMAKE_MAKE_PROGRAM" \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX" \
        -DCMAKE_BUILD_TYPE=Release
    
    # Build
    cmake --build build --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    log_success "Extension built successfully"
}

# Test the extension
test_extension() {
    log_info "Testing extension..."
    
    if [ -f "build/test_extension" ]; then
        ./build/test_extension
        log_success "Extension tests passed"
    else
        log_warning "No test executable found, skipping tests"
    fi
}

# Main execution
main() {
    log_info "Starting robust build process..."
    
    # Detect system
    detect_system
    
    # Install dependencies
    install_dependencies
    
    # Install Python dependencies
    install_python_deps
    
    # Setup vcpkg
    setup_vcpkg
    
    # Configure build environment
    configure_build_env
    
    # Verify build tools
    verify_build_tools
    
    # Build extension
    build_extension
    
    # Test extension
    test_extension
    
    log_success "Build process completed successfully!"
}

# Run main function
main "$@" 