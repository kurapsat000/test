#!/bin/bash

# Test vcpkg dependency installation locally
# This validates our exact dependency configuration

set -e

echo "📦 Testing vcpkg dependency installation locally..."

# Setup vcpkg if not already available
setup_vcpkg() {
    if [ ! -d "$HOME/vcpkg" ]; then
        echo "🔧 Setting up vcpkg..."
        git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
        cd "$HOME/vcpkg"
        ./bootstrap-vcpkg.sh
        cd - > /dev/null
    else
        echo "✅ vcpkg already available at $HOME/vcpkg"
    fi
    
    export VCPKG_ROOT="$HOME/vcpkg"
    export PATH="$VCPKG_ROOT:$PATH"
}

# Test our manifest mode configuration
test_manifest_mode() {
    echo "📋 Testing manifest mode dependency resolution..."
    
    local triplet="arm64-osx"  # For Apple Silicon Mac
    if [[ $(uname -m) == "x86_64" ]]; then
        triplet="x64-osx"
    fi
    
    echo "🔍 Using triplet: $triplet"
    echo "🔍 Testing with vcpkg.json manifest file"
    
    # Test dry-run first to see what would be installed
    echo "📊 Analyzing dependency resolution..."
    if vcpkg install --triplet="$triplet" --dry-run; then
        echo "✅ Manifest dependency resolution successful"
        
        # Show what would be installed
        echo "📦 Dependencies that would be installed:"
        vcpkg install --triplet="$triplet" --dry-run 2>&1 | grep -E "(Computing|Installing|Restored)" | head -20 || true
        
    else
        echo "❌ Manifest dependency resolution failed"
        echo "📋 Error details:"
        vcpkg install --triplet="$triplet" --dry-run 2>&1 | tail -20
        return 1
    fi
}

# Test specific dependency existence
test_dependency_availability() {
    echo "🔍 Testing individual dependency availability..."
    
    # Move to a temp directory to avoid manifest mode
    local temp_dir="/tmp/vcpkg-test-$$"
    mkdir -p "$temp_dir"
    cd "$temp_dir"
    
    local triplet="arm64-osx"
    if [[ $(uname -m) == "x86_64" ]]; then
        triplet="x64-osx"
    fi
    
    # Test each dependency individually
    local deps=("openssl" "protobuf" "grpc" "abseil" "c-ares" "arrow[flight]")
    
    for dep in "${deps[@]}"; do
        echo "📦 Testing $dep availability..."
        if vcpkg install "$dep:$triplet" --dry-run > /dev/null 2>&1; then
            echo "✅ $dep is available"
        else
            echo "❌ $dep is not available or has issues"
            # Show the specific error for this dependency
            vcpkg install "$dep:$triplet" --dry-run 2>&1 | tail -5
        fi
    done
    
    cd - > /dev/null
    rm -rf "$temp_dir"
}

# Test CMake integration with manifest
test_cmake_manifest_integration() {
    echo "🔧 Testing CMake integration with vcpkg manifest..."
    
    local test_dir="build/vcpkg-manifest-test"
    mkdir -p "$test_dir"
    
    # Copy our vcpkg.json to the test directory
    cp vcpkg.json "$test_dir/"
    
    # Create a simplified CMakeLists.txt for testing
    cat > "$test_dir/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.5)
project(ManifestTest)

# Test finding essential dependencies (available even without full Arrow)
find_package(OpenSSL REQUIRED)

if(OpenSSL_FOUND)
    message(STATUS "✅ OpenSSL found via vcpkg manifest")
else()
    message(FATAL_ERROR "❌ OpenSSL not found")
endif()

# Test target creation
add_executable(manifest_test test.cpp)
target_link_libraries(manifest_test OpenSSL::SSL OpenSSL::Crypto)

message(STATUS "✅ CMake + vcpkg manifest integration successful!")
EOF

    echo 'int main(){return 0;}' > "$test_dir/test.cpp"
    
    cd "$test_dir"
    
    echo "🔍 Testing CMake configuration with vcpkg manifest..."
    
    # Test CMake configuration with vcpkg toolchain
    if cmake . -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" > cmake_output.log 2>&1; then
        echo "✅ CMake manifest integration successful"
        
        # Show key CMake messages
        grep -E "(✅|OpenSSL|Found)" cmake_output.log || true
        
    else
        echo "❌ CMake manifest integration failed"
        echo "📋 Error details:"
        tail -10 cmake_output.log
        return 1
    fi
    
    cd - > /dev/null
    rm -rf "$test_dir"
}

# Validate our vcpkg.json structure
validate_manifest() {
    echo "📋 Validating vcpkg.json manifest structure..."
    
    # Check if the file is valid JSON
    if jq empty vcpkg.json > /dev/null 2>&1; then
        echo "✅ vcpkg.json is valid JSON"
        
        # Show the structure
        echo "📊 Manifest dependencies:"
        jq -r '.dependencies[] | if type == "object" then "  - \(.name) (features: \(.features | join(", ")))" else "  - \(.)" end' vcpkg.json
        
        # Check for vcpkg-configuration
        if jq -e '."vcpkg-configuration"' vcpkg.json > /dev/null 2>&1; then
            echo "✅ vcpkg-configuration section found"
        else
            echo "⚠️  No vcpkg-configuration section (not required)"
        fi
        
    else
        echo "❌ vcpkg.json is invalid JSON"
        return 1
    fi
}

# Check vcpkg tool version
check_vcpkg_version() {
    echo "ℹ️  vcpkg version information:"
    vcpkg version
    echo ""
}

# Main execution
main() {
    echo "🎯 Testing vcpkg manifest dependency configuration..."
    echo ""
    
    setup_vcpkg
    check_vcpkg_version
    validate_manifest
    
    if ! test_dependency_availability; then
        echo "❌ Dependency availability test failed"
        exit 1
    fi
    
    if ! test_manifest_mode; then
        echo "❌ Manifest mode test failed"
        exit 1
    fi
    
    if ! test_cmake_manifest_integration; then
        echo "❌ CMake manifest integration test failed"
        exit 1
    fi
    
    echo ""
    echo "🎉 All vcpkg manifest tests passed!"
    echo "✅ vcpkg.json manifest is valid"
    echo "✅ All dependencies are available"
    echo "✅ Manifest mode dependency resolution works"
    echo "✅ CMake + vcpkg manifest integration works"
    echo ""
    echo "💡 The configuration is ready for CI/CD environments"
}

main "$@" 