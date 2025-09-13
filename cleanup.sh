#!/bin/bash

# Cleanup script for removing build artifacts, temporary files, and other unnecessary files

echo "Starting cleanup..."

# Remove build artifacts
echo "Removing build directories..."
rm -rf build/
rm -rf dist/

# Remove temporary files
echo "Removing temporary files..."
find . -type f \( -name "*.tmp" -o -name "*.log" \) -exec rm -f {} +

# Optionally, remove cache directories
echo "Removing cache directories..."
rm -rf .cache/
rm -rf __pycache__/

echo "Cleanup completed."