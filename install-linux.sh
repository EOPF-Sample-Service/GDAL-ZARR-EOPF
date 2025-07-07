#!/bin/bash
set -e

PLUGIN_DIR=${GDAL_DRIVER_PATH:-/usr/lib/gdal/plugins}
PLUGIN_FILE="gdal_EOPFZarr.so"

echo "Installing GDAL EOPF Plugin for Linux..."

if [ "$1" = "debug" ]; then
  SOURCE_DIR="linux/debug"
  echo "Installing DEBUG version"
else
  SOURCE_DIR="linux/release"  
  echo "Installing RELEASE version"
fi

sudo mkdir -p "$PLUGIN_DIR"
sudo cp "$SOURCE_DIR/$PLUGIN_FILE" "$PLUGIN_DIR/"
sudo chmod 755 "$PLUGIN_DIR/$PLUGIN_FILE"

echo "Plugin installed to: $PLUGIN_DIR/$PLUGIN_FILE"
echo "Add this to your environment:"
echo "export GDAL_DRIVER_PATH=$PLUGIN_DIR:\$GDAL_DRIVER_PATH"
