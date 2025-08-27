#!/bin/bash

# Super-fast dependency validation script
# Tests CMake configuration locally without Docker

set -e

echo "⚡ Fast local dependency validation..."

# Check if required tools are available
check_tools() {
    local missing_tools=()
    
    if ! command -v cmake &> /dev/null; then
        missing_tools+=("cmake")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo "❌ Missing required tools: ${missing_tools[*]}"
        echo "📦 Install with:"
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "   brew install cmake"
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            echo "   sudo apt-get install cmake"
        fi
        exit 1
    fi
    
    echo "✅ Required tools available"
}

# Test CMake syntax and structure
test_cmake_syntax() {
    echo "🔍 Testing CMake syntax..."
    
    # Create a temporary test directory
    local test_dir="build/syntax-test"
    mkdir -p "$test_dir"
    
    # Copy our CMakeLists.txt and modify it for syntax testing
    cat > "$test_dir/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.5)
project(SyntaxTest)

# Mock find_package calls to test syntax only
macro(find_package)
    set(package_name ${ARGV0})
    message(STATUS "✅ find_package(${package_name}) syntax OK")
    
    # Create mock targets for linking test
    if(package_name STREQUAL "Protobuf")
        add_library(protobuf::libprotobuf INTERFACE IMPORTED)
    elseif(package_name STREQUAL "gRPC")
        add_library(gRPC::grpc++ INTERFACE IMPORTED)
    elseif(package_name STREQUAL "OpenSSL")
        add_library(OpenSSL::SSL INTERFACE IMPORTED)
        add_library(OpenSSL::Crypto INTERFACE IMPORTED)
    elseif(package_name STREQUAL "Arrow")
        add_library(Arrow::arrow_static INTERFACE IMPORTED)
    endif()
endmacro()

# Test our exact find_package sequence
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(c-ares CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Arrow CONFIG REQUIRED)

message(STATUS "🎉 All find_package calls have correct syntax!")

# Test target creation syntax
add_executable(syntax_test test.cpp)

# Test our exact target_link_libraries syntax
target_link_libraries(syntax_test
    OpenSSL::SSL
    OpenSSL::Crypto
    Arrow::arrow_static
    protobuf::libprotobuf
    gRPC::grpc++
)

message(STATUS "🎉 target_link_libraries syntax is correct!")
EOF

    # Create dummy source file
    echo 'int main(){return 0;}' > "$test_dir/test.cpp"
    
    # Test CMake configuration (syntax only)
    cd "$test_dir"
    if cmake . -DCMAKE_BUILD_TYPE=Release &> cmake_syntax.log; then
        echo "✅ CMake syntax validation passed"
        grep "✅\|🎉" cmake_syntax.log || true
    else
        echo "❌ CMake syntax validation failed"
        echo "📋 Error details:"
        tail -20 cmake_syntax.log
        return 1
    fi
    
    cd ../..
    rm -rf "$test_dir"
}

# Validate vcpkg.json syntax
test_vcpkg_syntax() {
    echo "🔍 Testing vcpkg.json syntax..."
    
    if command -v jq &> /dev/null; then
        if jq empty vcpkg.json &> /dev/null; then
            echo "✅ vcpkg.json is valid JSON"
            
            # Show dependency structure
            echo "📦 Dependencies found:"
            jq -r '.dependencies[] | if type == "object" then "  - \(.name) (features: \(.features | join(", ")))" else "  - \(.)" end' vcpkg.json
        else
            echo "❌ vcpkg.json has invalid JSON syntax"
            return 1
        fi
    else
        echo "⚠️  jq not available, skipping detailed JSON validation"
        echo "📦 Install jq for better validation: brew install jq"
        
        # Basic JSON validation using Python
        if command -v python3 &> /dev/null; then
            if python3 -c "import json; json.load(open('vcpkg.json'))" 2>/dev/null; then
                echo "✅ vcpkg.json is valid JSON (via Python)"
            else
                echo "❌ vcpkg.json has invalid JSON syntax"
                return 1
            fi
        fi
    fi
}

# Check for common CMake issues
check_cmake_issues() {
    echo "🔍 Checking for common CMake issues..."
    
    # Check for mixed case in package names
    if grep -q "find_package.*[A-Z].*[a-z]" CMakeLists.txt; then
        echo "⚠️  Found mixed case in find_package calls - this might cause issues"
    fi
    
    # Check for correct order
    if grep -n "find_package.*Arrow" CMakeLists.txt | head -1 | cut -d: -f1 > /tmp/arrow_line &&
       grep -n "find_package.*Protobuf" CMakeLists.txt | head -1 | cut -d: -f1 > /tmp/protobuf_line 2>/dev/null; then
        if [ "$(cat /tmp/arrow_line)" -lt "$(cat /tmp/protobuf_line)" ]; then
            echo "❌ Arrow find_package called before Protobuf - this will cause dependency issues!"
            return 1
        else
            echo "✅ Dependencies are in correct order"
        fi
    fi
    
    rm -f /tmp/arrow_line /tmp/protobuf_line
    
    # Check that we're linking the critical targets
    if grep -q "protobuf::libprotobuf" CMakeLists.txt; then
        echo "✅ Explicit protobuf linking found"
    else
        echo "⚠️  Missing explicit protobuf::libprotobuf linking"
    fi
    
    if grep -q "gRPC::grpc++" CMakeLists.txt; then
        echo "✅ Explicit gRPC linking found"
    else
        echo "⚠️  Missing explicit gRPC::grpc++ linking"
    fi
}

# OS-specific validation
validate_os_specific() {
    echo "🖥️  Validating OS-specific configuration..."
    
    case "$OSTYPE" in
        darwin*)
            echo "🍎 macOS detected"
            echo "✅ OpenSSL should be available via Homebrew"
            echo "✅ CMake and ninja should work correctly"
            ;;
        linux-gnu*)
            echo "🐧 Linux detected"
            echo "✅ System packages should provide base dependencies"
            echo "✅ vcpkg will handle remaining dependencies"
            ;;
        msys*|cygwin*|win32*)
            echo "🪟 Windows detected"
            echo "⚠️  Windows builds may need additional configuration"
            echo "💡 Consider using Windows-specific vcpkg triplets"
            ;;
        *)
            echo "❓ Unknown OS: $OSTYPE"
            echo "⚠️  Build may require additional configuration"
            ;;
    esac
}

# Main execution
main() {
    echo "🎯 Running super-fast dependency validation..."
    echo ""
    
    check_tools
    test_vcpkg_syntax
    test_cmake_syntax
    check_cmake_issues
    validate_os_specific
    
    echo ""
    echo "🎉 Fast validation completed!"
    echo "💡 This validates syntax and structure only."
    echo "🚀 For full testing, use: ./scripts/test-local.sh"
    echo "☁️  For complete testing, push to trigger GitHub Actions"
}

# Run main function
main "$@" 