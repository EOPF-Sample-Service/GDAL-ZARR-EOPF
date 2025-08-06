# EOPF-Zarr Docker Image - Quick Start Guide

## 🚀 Ready-to-Use Docker Image

The EOPF-Zarr GDAL driver is now available as a public Docker image on Docker Hub!

```bash
docker pull yuvraj1989/eopf-zarr-driver:v2.0.0
```

## 🎯 What's Included

- **Ubuntu 25.04** with **GDAL 3.10.2**
- **EOPF-Zarr GDAL driver** built and ready to use
- **Complete rasterio integration** (compiled against system GDAL)
- **JupyterLab environment** with all geospatial packages
- **Network access** for remote Zarr datasets
- **Compression support** (blosc, LZ4, ZSTD)

## 🏃 Quick Start

### Option 1: Direct Docker Run
```bash
# Run JupyterLab directly
docker run -p 8888:8888 yuvraj1989/eopf-zarr-driver:v2.0.0

# Access JupyterLab at: http://localhost:8888
```

### Option 2: Using Docker Compose (Recommended)
```bash
# Clone the repository for docker-compose.yml
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

# Start the service
docker-compose up -d

# Access JupyterLab at: http://localhost:8888
```

### Option 3: Interactive Shell
```bash
# Run interactive shell for testing
docker run -it yuvraj1989/eopf-zarr-driver:v2.0.0 /bin/bash

# Test GDAL driver
gdalinfo --formats | grep EOPF

# Test with a Zarr dataset
gdalinfo "EOPFZARR:'/vsicurl/https://example.com/dataset.zarr'"
```

## 🧪 Testing EOPF-Zarr Functionality

### GDAL Integration
```python
from osgeo import gdal

# Open a remote Zarr dataset
url = "EOPFZARR:'/vsicurl/https://objects.eodc.eu/Sentinel-3/OLCI/OL_2_WFR___/2022/01/01/S3A_OL_2_WFR____20220101T001159_20220101T001459_20220101T021013_0179_081_016_1980_MAR_O_NR_003.zarr'"
dataset = gdal.Open(url)
print(f"Dataset size: {dataset.RasterXSize} x {dataset.RasterYSize}")
```

### rasterio Integration  
```python
import rasterio

# Same dataset, using rasterio
with rasterio.open(url) as src:
    print(f"Dataset CRS: {src.crs}")
    print(f"Dataset bounds: {src.bounds}")
    data = src.read(1)  # Read first band
```

## 🔧 Environment Variables

The image comes pre-configured with:
- `GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers`
- `GDAL_DATA=/usr/share/gdal`
- `PROJ_LIB=/usr/share/proj`
- `PYTHONPATH=/usr/local/lib/python3.13/site-packages`

## 📊 Available Packages

- **Core**: gdal, rasterio, xarray, zarr, dask
- **Geospatial**: geopandas, rioxarray, fiona, shapely, pyproj
- **Scientific**: numpy, scipy, matplotlib, netcdf4, h5py
- **Visualization**: cartopy, jupyter ecosystem
- **Compression**: blosc, numcodecs

## 🆕 What's New in v2.0.0

- ✅ Fixed rasterio + EOPF-Zarr integration
- ✅ Enhanced URL format support with proper quoting
- ✅ Complete compression codec support
- ✅ Production-ready environment matching

## 🐛 Troubleshooting

### Common Issues

1. **URL Format**: Ensure you use single quotes around the URL:
   ```python
   # ✅ Correct
   url = "EOPFZARR:'/vsicurl/https://...'"
   
   # ❌ Incorrect  
   url = "EOPFZARR:/vsicurl/https://..."
   ```

2. **Network Access**: The container needs internet access for remote datasets.

3. **Memory**: Large datasets may require increased Docker memory limits.

## 📞 Support

- **GitHub**: https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF
- **Issues**: Report bugs and feature requests on GitHub
- **Docker Hub**: https://hub.docker.com/r/yuvraj1989/eopf-zarr-driver

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.
