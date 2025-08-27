#!/bin/bash

# Simple local CMake test without Docker
# Tests actual dependency resolution with system packages

set -e

echo "🔧 Testing CMake configuration locally without Docker..."

# Check if we have basic build tools
check_build_tools() {
    local missing=()
    
    if ! command -v cmake &> /dev/null; then
        missing+=("cmake")
    fi
    
    if ! command -v pkg-config &> /dev/null; then
        missing+=("pkg-config")
    fi
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo "❌ Missing build tools: ${missing[*]}"
        echo "📦 Install with: brew install cmake pkg-config"
        return 1
    fi
    
    echo "✅ Build tools available"
}

# Test CMake configuration with system OpenSSL
test_minimal_cmake() {
    echo "🔍 Testing minimal CMake configuration..."
    
    local test_dir="build/local-test"
    mkdir -p "$test_dir"
    
    # Create a minimal test that uses only system packages
    cat > "$test_dir/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.5)
project(LocalTest)

# Test finding OpenSSL (available on macOS via system/Homebrew)
find_package(OpenSSL REQUIRED)

if(OpenSSL_FOUND)
    message(STATUS "✅ OpenSSL found: ${OPENSSL_VERSION}")
    message(STATUS "✅ OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "✅ OpenSSL libraries: ${OPENSSL_LIBRARIES}")
else()
    message(FATAL_ERROR "❌ OpenSSL not found")
endif()

# Test creating a simple executable
add_executable(local_test test.cpp)
target_link_libraries(local_test OpenSSL::SSL OpenSSL::Crypto)

message(STATUS "🎉 Local CMake configuration successful!")
EOF

    echo 'int main(){return 0;}' > "$test_dir/test.cpp"
    
    cd "$test_dir"
    
    if cmake . -DCMAKE_BUILD_TYPE=Release; then
        echo "✅ CMake configuration successful"
        
        # Try to build
        if cmake --build .; then
            echo "✅ Local build successful"
            
            # Test the executable
            if ./local_test; then
                echo "✅ Executable runs successfully"
            else
                echo "⚠️  Executable failed to run"
            fi
        else
            echo "❌ Build failed"
            return 1
        fi
    else
        echo "❌ CMake configuration failed"
        return 1
    fi
    
    cd ../..
    rm -rf "$test_dir"
}

# Check if Homebrew packages are available
check_homebrew_packages() {
    echo "🍺 Checking Homebrew package availability..."
    
    if command -v brew &> /dev/null; then
        echo "✅ Homebrew available"
        
        # Check if common packages are installed
        local packages=("openssl" "cmake" "pkg-config")
        for pkg in "${packages[@]}"; do
            if brew list "$pkg" &> /dev/null; then
                echo "✅ $pkg installed via Homebrew"
            else
                echo "⚠️  $pkg not installed via Homebrew"
            fi
        done
        
        # Check if vcpkg is available
        if brew list vcpkg &> /dev/null; then
            echo "✅ vcpkg available via Homebrew"
        else
            echo "💡 vcpkg not installed. Install with: brew install vcpkg"
        fi
    else
        echo "⚠️  Homebrew not available"
    fi
}

# Test our actual CMakeLists.txt structure
test_project_structure() {
    echo "📁 Testing project structure..."
    
    # Check that key files exist
    local required_files=("CMakeLists.txt" "vcpkg.json" "src/snowflake_extension.cpp")
    for file in "${required_files[@]}"; do
        if [ -f "$file" ]; then
            echo "✅ $file exists"
        else
            echo "❌ Missing required file: $file"
            return 1
        fi
    done
    
    # Check src directory structure
    if [ -d "src" ]; then
        local src_files=$(find src -name "*.cpp" | wc -l)
        echo "✅ Found $src_files source files in src/"
        
        if [ -d "src/include" ]; then
            local header_files=$(find src/include -name "*.hpp" | wc -l)
            echo "✅ Found $header_files header files in src/include/"
        else
            echo "❌ Missing src/include directory"
            return 1
        fi
    else
        echo "❌ Missing src directory"
        return 1
    fi
}

# Main execution
main() {
    echo "🎯 Running local CMake testing..."
    echo ""
    
    if ! check_build_tools; then
        echo "❌ Build tools check failed"
        exit 1
    fi
    
    check_homebrew_packages
    
    if ! test_project_structure; then
        echo "❌ Project structure check failed"
        exit 1
    fi
    
    if ! test_minimal_cmake; then
        echo "❌ CMake test failed"
        exit 1
    fi
    
    echo ""
    echo "🎉 All local tests passed!"
    echo "💡 This validates basic CMake functionality with system packages"
    echo "🐳 For full dependency testing, ensure Docker is running and use: ./scripts/test-local.sh"
    echo "☁️  For multi-OS testing, check GitHub Actions in your repository"
}

main "$@" 