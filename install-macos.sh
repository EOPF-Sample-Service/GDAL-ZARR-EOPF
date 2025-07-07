#!/bin/bash
set -e

PLUGIN_DIR=${GDAL_DRIVER_PATH:-$(brew --prefix gdal)/lib/gdal/plugins}
PLUGIN_FILE="gdal_EOPFZarr.dylib"

echo "Installing GDAL EOPF Plugin for macOS..."

# Check for command line argument for universal binary
if [ "$1" = "universal" ]; then
    SOURCE_DIR="macos/universal"
    echo "Using universal binary (works on both Intel and Apple Silicon)"
else
    # Detect architecture and use specific binary
    ARCH=$(uname -m)
    echo "Detected architecture: $ARCH"
    
    if [ "$ARCH" = "arm64" ]; then
        SOURCE_DIR="macos/arm64"
        echo "Using Apple Silicon (arm64) specific binary"
    elif [ "$ARCH" = "x86_64" ]; then
        SOURCE_DIR="macos/x86_64"
        echo "Using Intel (x86_64) specific binary"
    else
        echo "Error: Unsupported architecture: $ARCH"
        echo "Supported architectures: arm64 (Apple Silicon), x86_64 (Intel)"
        echo "Or use: ./install-macos.sh universal"
        exit 1
    fi
fi

if [ ! -f "$SOURCE_DIR/$PLUGIN_FILE" ]; then
    echo "Error: Plugin file not found: $SOURCE_DIR/$PLUGIN_FILE"
    echo "Available files:"
    find macos -name "*.dylib" 2>/dev/null || echo "No .dylib files found"
    exit 1
fi

mkdir -p "$PLUGIN_DIR"
cp "$SOURCE_DIR/$PLUGIN_FILE" "$PLUGIN_DIR/"
chmod 755 "$PLUGIN_DIR/$PLUGIN_FILE"

echo "Plugin installed to: $PLUGIN_DIR/$PLUGIN_FILE"
if [ "$1" = "universal" ]; then
    echo "Binary type: Universal (Intel + Apple Silicon)"
else
    echo "Architecture: $ARCH"
    echo "Note: Universal binary also available with: ./install-macos.sh universal"
fi
echo "Add this to your environment:"
echo "export GDAL_DRIVER_PATH=$PLUGIN_DIR:\$GDAL_DRIVER_PATH"
