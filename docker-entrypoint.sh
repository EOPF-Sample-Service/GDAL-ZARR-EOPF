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
import sys

gdal.AllRegister()
print(f'📦 Total GDAL drivers: {gdal.GetDriverCount()}')
print(f'🐍 GDAL: {gdal.VersionInfo()}')

# Check EOPFZARR driver
eopf_driver = gdal.GetDriverByName('EOPFZARR')
if eopf_driver:
    print('✅ EOPF-Zarr driver loaded successfully!')
    print(f'   Driver: {eopf_driver.GetDescription()}')
    ext = eopf_driver.GetMetadataItem(gdal.DMD_EXTENSION) or 'None'
    prefix = eopf_driver.GetMetadataItem(gdal.DMD_CONNECTION_PREFIX) or 'None'
    print(f'   Extension: {ext}')
    print(f'   Prefix: {prefix}')
else:
    print('⚠️ EOPF-Zarr driver not found')

# Check standard Zarr driver
zarr_driver = gdal.GetDriverByName('Zarr')
if zarr_driver:
    print('✅ Standard Zarr driver available')
else:
    print('❌ Standard Zarr driver not found')

# Test rasterio compatibility
try:
    import rasterio
    print(f'✅ Rasterio {rasterio.__version__} available')
    
    # Check rasterio driver extensions
    with rasterio.env.Env():
        extensions = rasterio.drivers.raster_driver_extensions()
        zarr_exts = {k:v for k,v in extensions.items() if 'zarr' in k.lower() or 'zarr' in v.lower()}
        if zarr_exts:
            print(f'   Rasterio Zarr mappings: {zarr_exts}')
        else:
            print('   Use explicit EOPFZARR: prefix with rasterio')
except ImportError as e:
    print(f'❌ Rasterio not available: {e}')

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
