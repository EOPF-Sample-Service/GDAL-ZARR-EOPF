#!/bin/bash
set -e

# Activate the eopf-zarr conda environment
source /opt/conda/etc/profile.d/conda.sh
conda activate eopf-zarr

# Set GDAL environment variables
export GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers
export GDAL_DATA=/usr/share/gdal
export PROJ_LIB=/usr/share/proj

# Verify EOPF-Zarr driver is available
echo "🔍 Checking EOPF-Zarr driver installation..."
python -c "
from osgeo import gdal
gdal.AllRegister()
driver = gdal.GetDriverByName('EOPFZARR')
if driver:
    print('✅ EOPF-Zarr driver loaded successfully!')
    print(f'   Driver: {driver.GetDescription()}')
else:
    print('⚠️ EOPF-Zarr driver not found, using built-in Zarr driver')
    
print(f'📦 Total GDAL drivers: {gdal.GetDriverCount()}')
print(f'🐍 Python: {gdal.VersionInfo()}')
"

# Check if this is for JupyterHub or standalone
if [ "$JUPYTERHUB_SERVICE_PREFIX" ]; then
    echo "🚀 Starting JupyterLab for JupyterHub..."
    exec jupyter-labhub \
        --ip=0.0.0.0 \
        --port=8888 \
        --no-browser \
        --notebook-dir=/home/eopfuser/notebooks \
        --NotebookApp.token='' \
        --NotebookApp.password=''
else
    echo "🚀 Starting standalone JupyterLab..."
    exec jupyter lab \
        --ip=0.0.0.0 \
        --port=8888 \
        --no-browser \
        --allow-root \
        --notebook-dir=/home/eopfuser/notebooks \
        --NotebookApp.token='' \
        --NotebookApp.password=''
fi
