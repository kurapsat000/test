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

echo "✅ All workflow validations passed!"
echo "🚀 Workflows are ready for GitHub Actions" 