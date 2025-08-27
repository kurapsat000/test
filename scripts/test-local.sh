#!/bin/bash

# Local testing script for DuckDB Snowflake Extension
# Uses 'act' to run GitHub Actions workflows locally for faster testing

set -e

echo "🚀 Starting local GitHub Actions testing with act..."

# Check if act is installed
if ! command -v act &> /dev/null; then
    echo "❌ act is not installed. Please install it first:"
    echo "   macOS: brew install act"
    echo "   Linux: curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash"
    exit 1
fi

# Check if Docker is running
if ! docker info &> /dev/null; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

echo "✅ act and Docker are available"

# Create act configuration for faster testing
cat > .actrc << 'EOF'
# Use smaller Docker images for faster startup
-P ubuntu-latest=catthehacker/ubuntu:act-latest
-P macos-latest=catthehacker/ubuntu:act-latest
-P windows-latest=catthehacker/ubuntu:act-latest

# Mount local directory for faster iteration
--bind

# Use host network for faster downloads
--use-gitignore=false
EOF

echo "📝 Created .actrc configuration for faster testing"

# Function to test specific job
test_job() {
    local job_name="$1"
    echo "🧪 Testing job: $job_name"
    
    act workflow_dispatch \
        --job "$job_name" \
        --workflows .github/workflows/test-build.yml \
        --verbose \
        --env-file <(echo "GITHUB_TOKEN=dummy") \
        2>&1 | tee "test-${job_name}.log"
    
    if [ $? -eq 0 ]; then
        echo "✅ Job $job_name completed successfully"
    else
        echo "❌ Job $job_name failed - check test-${job_name}.log"
    fi
}

# Menu for testing options
echo ""
echo "🎯 Local Testing Options:"
echo "1. Test basic dependencies only (fastest)"
echo "2. Test Arrow dependencies (slower but comprehensive)"
echo "3. Test both (full test)"
echo "4. List available workflows"
echo ""
read -p "Choose option (1-4): " choice

case $choice in
    1)
        echo "🔧 Testing basic dependencies..."
        test_job "test-dependencies"
        ;;
    2)
        echo "🏹 Testing Arrow dependencies..."
        test_job "test-arrow-deps"
        ;;
    3)
        echo "🎯 Running full test suite..."
        test_job "test-dependencies"
        if [ $? -eq 0 ]; then
            test_job "test-arrow-deps"
        fi
        ;;
    4)
        echo "📋 Available workflows:"
        act --list
        ;;
    *)
        echo "❌ Invalid option"
        exit 1
        ;;
esac

echo ""
echo "🎉 Local testing completed!"
echo "📊 Check the generated .log files for detailed output"
echo ""
echo "💡 Tips for faster iteration:"
echo "   - Use option 1 for quick dependency checks"
echo "   - Use option 2 only when testing Arrow-specific changes"
echo "   - Edit .github/workflows/test-build.yml and re-run for quick tests" 