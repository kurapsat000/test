#!/bin/bash

# DuckDB Snowflake Extension Development Setup Script
# This script sets up the development environment for the DuckDB Snowflake extension

set -e

echo "🚀 Setting up DuckDB Snowflake Extension development environment..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] || [ ! -f "Makefile" ]; then
    echo "❌ Error: Please run this script from the project root directory"
    exit 1
fi

# Initialize and update submodules
echo "📦 Initializing submodules..."
git submodule update --init --recursive

# Check if Go is installed
if ! command -v go &> /dev/null; then
    echo "⚠️  Warning: Go is not installed. ADBC driver building will not work."
    echo "   Please install Go from https://golang.org/dl/"
else
    echo "✅ Go is installed: $(go version)"
fi

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "❌ Error: CMake is required but not installed"
    echo "   Please install CMake from https://cmake.org/download/"
    exit 1
else
    echo "✅ CMake is installed: $(cmake --version | head -n1)"
fi

# Check if Ninja is installed
if ! command -v ninja &> /dev/null; then
    echo "⚠️  Warning: Ninja is not installed. Builds will be slower."
    echo "   Please install Ninja for faster builds"
else
    echo "✅ Ninja is installed: $(ninja --version)"
fi

# Setup vcpkg if not already done
if [ ! -d "vcpkg" ]; then
    echo "📦 Setting up vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh
    export VCPKG_TOOLCHAIN_PATH=$(pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake
    echo "✅ vcpkg setup complete"
else
    echo "✅ vcpkg already exists"
fi

# Test basic build
echo "🔨 Testing basic build..."
make clean || true
make

echo "✅ Development environment setup complete!"
echo ""
echo "📋 Next steps:"
echo "   1. Run 'make test' to run tests"
echo "   2. Run 'make release-snowflake' to build with ADBC support"
echo "   3. Start DuckDB with: ./build/release/duckdb"
echo ""
echo "🔧 Available make targets:"
echo "   - make              : Build basic extension"
echo "   - make release      : Build release version"
echo "   - make debug        : Build debug version"
echo "   - make test         : Run tests"
echo "   - make release-snowflake : Build with ADBC driver"
echo "   - make clean        : Clean build artifacts" 