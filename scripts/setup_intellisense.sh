#!/bin/bash

# DuckDB IntelliSense Setup Script
# This script sets up IntelliSense for the DuckDB project using clangd

set -e

echo "Setting up IntelliSense for DuckDB..."

# Check if clangd is installed
if ! command -v clangd &> /dev/null; then
    echo "Error: clangd is not installed. Please install it first:"
    echo "  sudo apt update && sudo apt install clangd"
    exit 1
fi

# Create necessary directories
mkdir -p build/clangd

# Run the make target to set up IntelliSense
echo "Running make intellisense..."
make intellisense

# Verify the setup
if [ -f "build/clangd/compile_commands.json" ]; then
    echo "✅ IntelliSense setup complete!"
    echo "📁 compile_commands.json is available at: build/clangd/compile_commands.json"
    echo ""
    echo "To update IntelliSense after making changes, run:"
    echo "  make update-intellisense"
    echo ""
    echo "VS Code should now provide full IntelliSense support including:"
    echo "  - Code completion"
    echo "  - Go to definition"
    echo "  - Find all references"
    echo "  - Error detection"
    echo "  - Code formatting"
else
    echo "❌ IntelliSense setup failed!"
    exit 1
fi 