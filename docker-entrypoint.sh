#!/bin/bash
set -e

# Set GDAL environment variables
export GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers
export GDAL_DATA=/usr/share/gdal
export PROJ_LIB=/usr/share/proj

# Verify EOPF-Zarr driver is available
echo "🔍 Checking EOPF-Zarr driver installation..."
python3 -c "
from osgeo import gdal
gdal.AllRegister()
driver = gdal.GetDriverByName('EOPFZARR')
if driver:
    print('✅ EOPF-Zarr driver loaded successfully!')
    print(f'   Driver: {driver.GetDescription()}')
else:
    print('⚠️ EOPF-Zarr driver not found, using built-in Zarr driver')
    
print(f'📦 Total GDAL drivers: {gdal.GetDriverCount()}')
print(f'🐍 GDAL: {gdal.VersionInfo()}')
"

# Check if running in JupyterHub environment
if [ ! -z "$JUPYTERHUB_SERVICE_PREFIX" ]; then
    echo "🎯 JupyterHub environment detected"
    # JupyterHub will handle the startup
    exec "$@"
else
    # Start JupyterLab in standalone mode
    echo "🚀 Starting standalone JupyterLab..."
    exec jupyter lab \
        --ip=0.0.0.0 \
        --port=8888 \
        --no-browser \
        --notebook-dir=/home/jupyter/work \
        --ServerApp.token='' \
        --ServerApp.password='' \
        --ServerApp.allow_origin='*' \
        --ServerApp.allow_remote_access=True
fi
