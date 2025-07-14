#!/bin/bash
set -e

# Configuration
PROJECT_DIR="/Users/yuvrajadagale/gdal/development/GDAL-ZARR-EOPF"
BUILD_DIR="$PROJECT_DIR/build"

# Dynamically detect GDAL installation
GDAL_PREFIX=$(gdal-config --prefix 2>/dev/null || echo "/opt/homebrew")
PLUGIN_DIR="$GDAL_PREFIX/lib/gdalplugins"
PLUGIN_NAME="gdal_EOPFZarr.dylib"

echo "🔨 Building EOPF Zarr plugin..."
echo "GDAL prefix: $GDAL_PREFIX"
echo "Plugin directory: $PLUGIN_DIR"

# Navigate to build directory
cd "$BUILD_DIR"

# Build the plugin
echo "🚀 Building plugin..."
cmake --build . --config Debug --target all -j 8

# Install using CMake
echo "📦 Installing plugin using CMake..."
sudo cmake --install . --component Runtime

# Verify installation
if [ -f "$PLUGIN_DIR/$PLUGIN_NAME" ]; then
    echo "✅ Plugin installed successfully!"
    
    # Test plugin recognition
    echo "🔍 Testing plugin..."
    gdalinfo --formats | grep -i EOPFZARR || echo "⚠️  Plugin may need terminal restart to be recognized"
else
    echo "❌ Installation verification failed!"
    exit 1
fi

echo "🎉 Complete!"

