# GDAL EOPF Plugin Usage Examples

## Installation
```bash
export GDAL_DRIVER_PATH=/path/to/plugin/directory:$GDAL_DRIVER_PATH
```

## Basic Usage

### Method 1: Driver Prefix (Recommended)
```bash
# Most explicit and clean approach
gdalinfo EOPFZARR:/path/to/eopf/dataset.zarr
gdal_translate EOPFZARR:/path/to/input.zarr output.tif
```

### Method 2: Open Options
```bash
# Explicit EOPF processing
gdalinfo -oo EOPF_PROCESS=YES /path/to/eopf/dataset.zarr
gdal_translate -oo EOPF_PROCESS=YES /path/to/input.zarr output.tif
```

### Method 3: Auto-detection
```bash
# Uses regular Zarr driver (for comparison)
gdalinfo /path/to/dataset.zarr
```

## Python Usage
```python
from osgeo import gdal

# Method 1: Driver prefix
ds = gdal.Open('EOPFZARR:/path/to/dataset.zarr')

# Method 2: Open options
ds = gdal.OpenEx('/path/to/dataset.zarr', 
                 open_options=['EOPF_PROCESS=YES'])
```

## Verification
```bash
# Check if driver is available
gdalinfo --formats | grep EOPF

# Should show: EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver 1
```
