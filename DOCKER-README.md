# EOPF-Zarr Docker Deployment

This directory contains Docker configuration for deploying the EOPF-Zarr GDAL driver in a clean Ubuntu 25 environment with GDAL 3.10 and the complete EOPF Python environment.

## 🚀 Quick Start

### Local Testing
```bash
# Build the Docker image
./build-docker.sh

# Start the container
docker-compose up

# Access JupyterLab
open http://localhost:8888
```

### JupyterHub Deployment
```bash
# Build and tag for registry
docker build -t your-registry/eopf-zarr-driver:latest .

# Push to registry
docker push your-registry/eopf-zarr-driver:latest

# Configure in JupyterHub at https://jupyterhub.user.eopf.eodc.eu
```

## 📋 Environment Details

### Base Image
- **OS**: Ubuntu 25.04
- **GDAL**: 3.10.x (system package)
- **Python**: 3.11 (via conda)
- **Architecture**: x86_64

### EOPF Environment
Based on: https://github.com/EOPF-Sample-Service/eopf-sample-notebooks/blob/main/environment.yml

**Key Packages:**
- xarray, xarray-eopf, xcube-eopf
- gdal, rioxarray, cartopy
- zarr, dask, geopandas
- jupyter, jupyterlab, ipywidgets
- matplotlib, numpy, scikit-image

### EOPF-Zarr Driver
- **Location**: `/opt/eopf-zarr/drivers/gdal_EOPFZarr.so`
- **Environment**: `GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers`
- **Build**: CMake Release configuration

## 🧪 Testing

### Verify Driver Installation
```bash
docker-compose run --rm eopf-zarr python -c "
from osgeo import gdal
gdal.AllRegister()
print(f'Total drivers: {gdal.GetDriverCount()}')

driver = gdal.GetDriverByName('EOPFZARR')
if driver:
    print(f'✅ EOPF-Zarr: {driver.GetDescription()}')
else:
    print('❌ EOPF-Zarr driver not found')
"
```

### Test with Sample Data
```bash
# Run interactive shell
docker-compose run --rm eopf-zarr bash

# Test in Python
python -c "
from osgeo import gdal
# Test your Zarr files here
"
```

## 📁 File Structure

```
├── Dockerfile              # Main Docker image definition
├── docker-compose.yml      # Local testing configuration
├── docker-entrypoint.sh    # Container startup script
├── environment.yml         # EOPF Python environment
├── build-docker.sh         # Build and test script
├── .dockerignore           # Exclude files from build
└── notebooks/              # Jupyter notebooks (mounted)
```

## 🔧 Customization

### Modify Python Environment
Edit `environment.yml` and rebuild:
```bash
./build-docker.sh
```

### Change GDAL Version
Update the Dockerfile base image or install specific GDAL version.

### JupyterHub Configuration
The image includes both `jupyter` and `jupyterhub` packages for compatibility.

## 🐛 Troubleshooting

### Driver Not Found
```bash
# Check driver path
docker-compose run --rm eopf-zarr ls -la /opt/eopf-zarr/drivers/

# Check environment variables
docker-compose run --rm eopf-zarr env | grep GDAL
```

### Build Issues
```bash
# Clean build
docker system prune -f
./build-docker.sh
```

### JupyterLab Access
- **URL**: http://localhost:8888
- **Token**: None (disabled for local testing)
- **Notebooks**: Mounted from `./notebooks/`

## 📞 Support

For issues with:
- **EOPF-Zarr Driver**: Check build logs and CMake configuration
- **EOPF Environment**: Verify environment.yml packages
- **JupyterHub**: Check container logs and network configuration
