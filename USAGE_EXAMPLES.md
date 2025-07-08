# GDAL EOPF Plugin Usage Examples

This document provides comprehensive examples of how to use the GDAL EOPF-Zarr plugin in various scenarios.

## Setup and Verification

### Check Plugin Installation
```bash
# Verify the plugin is loaded
gdalinfo --formats | grep EOPFZARR

# Should show: EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

### Environment Setup
```bash
# If using custom plugin directory
export GDAL_DRIVER_PATH="/path/to/plugin/directory:$GDAL_DRIVER_PATH"

# Enable debug output (optional)
export CPL_DEBUG=EOPFZARR
```

## Basic Usage Patterns

### Method 1: Driver Prefix (Recommended)
```bash
# Most explicit and clean approach
gdalinfo EOPFZARR:/path/to/eopf/dataset.zarr

# Convert to GeoTIFF
gdal_translate EOPFZARR:/path/to/input.zarr output.tif

# Reproject and resample
gdalwarp -t_srs EPSG:4326 -tr 0.001 0.001 EOPFZARR:/path/to/data.zarr reprojected.tif
```

### Method 2: Open Options
```bash
# Explicit EOPF processing activation
gdalinfo -oo EOPF_PROCESS=YES /path/to/eopf/dataset.zarr

# Convert with explicit processing
gdal_translate -oo EOPF_PROCESS=YES /path/to/input.zarr output.tif

# Multi-band extraction
gdal_translate -oo EOPF_PROCESS=YES -b 1 -b 2 -b 3 /path/to/input.zarr rgb.tif
```

### Method 3: Auto-detection (Fallback)
```bash
# Uses regular Zarr driver for comparison
gdalinfo /path/to/dataset.zarr

# Note: This may not provide EOPF-specific enhancements
```

## Command Line Examples

### Dataset Inspection
```bash
# Basic information
gdalinfo EOPFZARR:/path/to/dataset.zarr

# Detailed JSON output
gdalinfo -json EOPFZARR:/path/to/dataset.zarr

# List subdatasets
gdalinfo -sd EOPFZARR:/path/to/dataset.zarr

# Check specific metadata domains
gdalinfo -mdd all EOPFZARR:/path/to/dataset.zarr
```

### Data Conversion
```bash
# Simple format conversion
gdal_translate EOPFZARR:/data.zarr output.tif

# With compression
gdal_translate -co COMPRESS=LZW EOPFZARR:/data.zarr compressed.tif

# Extract specific bands
gdal_translate -b 1 -b 2 -b 3 EOPFZARR:/data.zarr rgb.tif

# Apply scaling and data type conversion
gdal_translate -scale 0 10000 0 255 -ot Byte EOPFZARR:/data.zarr scaled.tif

# Create overview pyramids
gdal_translate -outsize 50% 50% EOPFZARR:/data.zarr preview.tif
```

### Reprojection and Warping
```bash
# Reproject to different CRS
gdalwarp -t_srs EPSG:4326 EOPFZARR:/data.zarr geographic.tif

# Resample to specific resolution
gdalwarp -tr 0.001 0.001 EOPFZARR:/data.zarr resampled.tif

# Crop to extent
gdalwarp -te 10.0 45.0 15.0 50.0 EOPFZARR:/data.zarr cropped.tif

# Multiple operations
gdalwarp -t_srs EPSG:3857 -tr 100 100 -te_srs EPSG:4326 \
         -te 10.0 45.0 15.0 50.0 EOPFZARR:/data.zarr web_mercator.tif
```

### Remote Data Access
```bash
# Access data via HTTP
gdalinfo EOPFZARR:https://example.com/data/dataset.zarr

# With authentication (if needed)
gdalinfo --config GDAL_HTTP_AUTH NTLM \
         --config GDAL_HTTP_USERPWD user:pass \
         EOPFZARR:https://private.example.com/data.zarr

# Network timeouts
gdalinfo --config GDAL_HTTP_TIMEOUT 60 \
         EOPFZARR:https://slow.example.com/data.zarr
```

## Python Usage

### Basic Operations
```python
from osgeo import gdal
import numpy as np

# Method 1: Driver prefix
ds = gdal.Open('EOPFZARR:/path/to/dataset.zarr')

# Method 2: Open options
ds = gdal.OpenEx('/path/to/dataset.zarr', 
                 open_options=['EOPF_PROCESS=YES'])

# Get dataset information
print(f"Size: {ds.RasterXSize} x {ds.RasterYSize}")
print(f"Bands: {ds.RasterCount}")
print(f"Projection: {ds.GetProjection()}")

# Get geotransform
gt = ds.GetGeoTransform()
print(f"Geotransform: {gt}")

# Read metadata
metadata = ds.GetMetadata()
for key, value in metadata.items():
    print(f"{key}: {value}")
```

### Working with Bands
```python
# Access specific band
band = ds.GetRasterBand(1)

# Band information
print(f"Band data type: {gdal.GetDataTypeName(band.DataType)}")
print(f"Band size: {band.XSize} x {band.YSize}")

# Statistics
stats = band.GetStatistics(False, True)
if stats:
    print(f"Min: {stats[0]}, Max: {stats[1]}")
    print(f"Mean: {stats[2]}, StdDev: {stats[3]}")

# Read data
data = band.ReadAsArray()
print(f"Data shape: {data.shape}")
print(f"Data type: {data.dtype}")
```

### Array Operations
```python
# Read full dataset as array
full_data = ds.ReadAsArray()

# Read specific window
x_off, y_off = 1000, 1000
x_size, y_size = 512, 512
window_data = ds.ReadAsArray(x_off, y_off, x_size, y_size)

# Read multiple bands
band_data = ds.ReadAsArray(band_list=[1, 2, 3])

# Process with NumPy
processed = np.clip(full_data * 0.0001, 0, 1)  # Scale to 0-1

# Clean up
ds = None
```

### Integration with Rasterio
```python
import rasterio

# Open with rasterio (works through GDAL)
with rasterio.open('EOPFZARR:/path/to/data.zarr') as src:
    # Read data
    array = src.read()
    
    # Get metadata
    print(f"CRS: {src.crs}")
    print(f"Transform: {src.transform}")
    print(f"Bounds: {src.bounds}")
    
    # Read specific bands
    rgb = src.read([1, 2, 3])
```

### Working with Xarray
```python
import xarray as xr

# Open with xarray (through rasterio engine)
da = xr.open_rasterio('EOPFZARR:/path/to/data.zarr')

# Basic operations
print(da.dims)
print(da.coords)

# Clip to bounds
clipped = da.sel(x=slice(10.0, 15.0), y=slice(50.0, 45.0))

# Resample
resampled = da.coarsen(x=2, y=2).mean()
```

## QGIS Integration

### Loading Data
1. **Open QGIS**
2. **Add Raster Layer:**
   - Go to `Layer` → `Add Layer` → `Add Raster Layer`
   - Select "File" as source type
   - Browse to your `.zarr` file or directory
   - Click `Open`

### Advanced QGIS Usage
```python
# QGIS Python console
from qgis.core import QgsRasterLayer

# Load layer programmatically
layer = QgsRasterLayer('EOPFZARR:/path/to/data.zarr', 'EOPF Data')
QgsProject.instance().addMapLayer(layer)

# Check if layer is valid
if layer.isValid():
    print("Layer loaded successfully")
else:
    print("Error loading layer")
```

### Multi-band Visualization
1. **Right-click layer** → **Properties**
2. **Symbology tab:**
   - Select "Multiband color" for RGB visualization
   - Choose appropriate bands for Red, Green, Blue channels
   - Adjust min/max values for proper contrast

### Processing in QGIS
- Use standard QGIS processing tools
- Raster calculator works with EOPF layers
- Geoprocessing algorithms are fully compatible

## Troubleshooting Examples

### Debug Output
```bash
# Enable debug logging
export CPL_DEBUG=EOPFZARR
gdalinfo EOPFZARR:/path/to/data.zarr
```

### Error Handling
```python
from osgeo import gdal

# Enable error handling
gdal.UseExceptions()

try:
    ds = gdal.Open('EOPFZARR:/path/to/data.zarr')
    if ds is None:
        print("Failed to open dataset")
except Exception as e:
    print(f"Error: {e}")
```

### Version Compatibility
```bash
# Check versions
echo "GDAL Version:"
gdalinfo --version

echo "Plugin Status:"
gdalinfo --formats | grep -i eopf

echo "Available Drivers:"
gdalinfo --formats | grep -i zarr
```

## Performance Tips

### Large Datasets
```python
# Use windowed reading for large datasets
def read_in_chunks(dataset, chunk_size=1024):
    for y in range(0, dataset.RasterYSize, chunk_size):
        for x in range(0, dataset.RasterXSize, chunk_size):
            x_size = min(chunk_size, dataset.RasterXSize - x)
            y_size = min(chunk_size, dataset.RasterYSize - y)
            
            chunk = dataset.ReadAsArray(x, y, x_size, y_size)
            yield chunk, (x, y, x_size, y_size)
```

### Memory Management
```bash
# Set GDAL cache size for better performance
export GDAL_CACHEMAX=512  # 512 MB

# For very large datasets
export GDAL_CACHEMAX=2048  # 2 GB
```

### Network Performance
```bash
# Optimize for remote data
export GDAL_HTTP_TIMEOUT=300
export GDAL_HTTP_MAX_RETRY=3
export GDAL_HTTP_RETRY_DELAY=1
```
