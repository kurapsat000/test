#!/bin/bash

# Validate GitHub Actions workflows
echo "🔍 Validating GitHub Actions workflows..."

# Check if yamllint is available
if ! command -v yamllint &> /dev/null; then
    echo "⚠️  yamllint not found. Installing..."
    pip install yamllint
fi

# Validate YAML syntax
echo "📋 Checking YAML syntax..."
yamllint .github/workflows/ || {
    echo "❌ YAML syntax errors found"
    exit 1
}

# Check for common workflow issues
echo "🔧 Checking for common workflow issues..."

# Check for reusable workflow calls in wrong places
if grep -r "uses:.*\.github/workflows/" .github/workflows/; then
    echo "❌ Found reusable workflow calls in wrong places"
    exit 1
fi

# Check for invalid runner.os usage
if grep -r "runner\.os" .github/workflows/; then
    echo "❌ Found invalid runner.os usage"
    exit 1
fi

# Check for hashFiles function usage
if grep -r "hashFiles" .github/workflows/; then
    echo "❌ Found hashFiles function usage"
    exit 1
fi

# Check for vcpkg tag usage instead of SHA1
if grep -r "vcpkgGitCommitId: '[0-9]\{4\}\.[0-9]\{2\}\.[0-9]\{2\}'" .github/workflows/; then
    echo "❌ Found vcpkg tag usage instead of SHA1 hash"
    exit 1
fi

# Check for proper vcpkg SHA1 format (40 hex digits)
if ! grep -r "vcpkgGitCommitId: '[a-f0-9]\{40\}'" .github/workflows/ > /dev/null; then
    echo "❌ vcpkgGitCommitId must be a 40-character SHA1 hash"
    exit 1
fi

# Check for missing required fields
echo "📝 Checking for required workflow fields..."

# Check for proper job structure
if ! grep -r "runs-on:" .github/workflows/ > /dev/null; then
    echo "❌ Missing 'runs-on' field in jobs"
    exit 1
fi

# Check for proper checkout actions
if ! grep -r "actions/checkout" .github/workflows/ > /dev/null; then
    echo "❌ Missing checkout action"
    exit 1
fi

# Check for timeout configurations
echo "⏱️  Checking timeout configurations..."
if ! grep -r "timeout-minutes:" .github/workflows/ > /dev/null; then
    echo "⚠️  Warning: No timeout configurations found"
else
    echo "✅ Timeout configurations found"
fi

# Check for parallel build optimizations
echo "⚡ Checking build optimizations..."
if ! grep -r "parallel [0-9]" .github/workflows/ > /dev/null; then
    echo "⚠️  Warning: No parallel build limits found"
else
    echo "✅ Parallel build limits configured"
fi

echo "✅ All workflow validations passed!"
echo "🚀 Workflows are ready for GitHub Actions"

# Additional checks for common issues
echo "🔍 Additional checks..."

# Check for proper CMake setup
if ! grep -r "jwlawson/actions-setup-cmake" .github/workflows/ > /dev/null; then
    echo "⚠️  Warning: CMake setup action not found"
fi

# Check for proper vcpkg setup
if ! grep -r "lukka/run-vcpkg" .github/workflows/ > /dev/null; then
    echo "⚠️  Warning: vcpkg setup action not found"
fi

# Check for caching
if ! grep -r "actions/cache" .github/workflows/ > /dev/null; then
    echo "⚠️  Warning: No caching configured"
fi

# Check for resource management
echo "💾 Checking resource management..."
if grep -r "parallel [0-9]" .github/workflows/ | grep -v "parallel 2\|parallel 4" > /dev/null; then
    echo "⚠️  Warning: Some builds may use too many parallel jobs"
else
    echo "✅ Parallel job limits are reasonable"
fi

echo "🎉 Validation complete!" 