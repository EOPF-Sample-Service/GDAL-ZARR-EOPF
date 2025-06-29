# User Guide

## Quick Start

This guide shows how to use the EOPF-Zarr GDAL plugin to work with Earth Observation data.

## Basic Usage

### Using Command Line Tools

#### View Dataset Information
```bash
# Get basic info about an EOPF dataset
gdalinfo sample_data/sentinel2_l1c.zarr

# Get detailed information including metadata
gdalinfo -json sample_data/sentinel2_l1c.zarr

# List all subdatasets
gdalinfo -sd sample_data/sentinel2_l1c.zarr
```

#### Convert Data
```bash
# Convert EOPF dataset to GeoTIFF
gdal_translate sample_data/sentinel2_l1c.zarr output.tif

# Extract specific bands
gdal_translate -b 1 -b 2 -b 3 sample_data/sentinel2_l1c.zarr rgb.tif

# Apply scaling and offset
gdal_translate -scale 0 10000 0 255 sample_data/sentinel2_l1c.zarr scaled.tif
```

#### Reproject Data
```bash
# Reproject to different coordinate system
gdalwarp -t_srs EPSG:4326 sample_data/sentinel2_l1c.zarr reprojected.tif

# Resample to different resolution
gdalwarp -tr 0.001 0.001 sample_data/sentinel2_l1c.zarr resampled.tif
```

### Using with QGIS

1. **Open QGIS**
2. **Add Raster Layer:**
   - Go to `Layer` → `Add Layer` → `Add Raster Layer`
   - Browse to your `.zarr` file
   - Click `Open`

3. **Multi-band Visualization:**
   - Right-click on layer → `Properties`
   - Go to `Symbology` tab
   - Select `Multiband color` for RGB visualization
   - Choose appropriate bands for Red, Green, Blue channels

### Using with Python

#### Basic Reading
```python
from osgeo import gdal
import numpy as np

# Open dataset
dataset = gdal.Open('sample_data/sentinel2_l1c.zarr')
if dataset is None:
    print("Failed to open dataset")
    exit(1)

# Get dataset information
print(f"Size: {dataset.RasterXSize} x {dataset.RasterYSize}")
print(f"Bands: {dataset.RasterCount}")
print(f"Projection: {dataset.GetProjection()}")

# Read first band
band = dataset.GetRasterBand(1)
data = band.ReadAsArray()

print(f"Band data type: {gdal.GetDataTypeName(band.DataType)}")
print(f"Band min/max: {band.GetMinimum()}/{band.GetMaximum()}")

# Clean up
dataset = None
```

#### Advanced Reading with Windowing
```python
from osgeo import gdal
import numpy as np

# Open dataset
dataset = gdal.Open('sample_data/sentinel2_l1c.zarr')

# Read specific window (x_offset, y_offset, width, height)
x_off, y_off = 1000, 1000
width, height = 512, 512

# Read multiple bands
data = dataset.ReadAsArray(x_off, y_off, width, height)

# data shape: (bands, height, width)
print(f"Data shape: {data.shape}")
print(f"Data type: {data.dtype}")

# Process specific bands
if data.ndim == 3:  # Multi-band
    red_band = data[0]    # Assuming band 1 is red
    green_band = data[1]  # Assuming band 2 is green
    blue_band = data[2]   # Assuming band 3 is blue
    
    # Create RGB composite
    rgb = np.stack([red_band, green_band, blue_band], axis=-1)
```

#### Metadata Access
```python
from osgeo import gdal

dataset = gdal.Open('sample_data/sentinel2_l1c.zarr')

# Get dataset metadata
metadata = dataset.GetMetadata()
for key, value in metadata.items():
    print(f"{key}: {value}")

# Get band metadata
band = dataset.GetRasterBand(1)
band_metadata = band.GetMetadata()
for key, value in band_metadata.items():
    print(f"Band 1 {key}: {value}")

# Get geotransform
geotransform = dataset.GetGeoTransform()
print(f"Geotransform: {geotransform}")

# Get coordinate system
projection = dataset.GetProjection()
print(f"Projection: {projection}")
```

### Working with Cloud Data

#### Accessing S3 Data
```bash
# Set AWS credentials (if needed)
export AWS_ACCESS_KEY_ID=your_access_key
export AWS_SECRET_ACCESS_KEY=your_secret_key

# Access data from S3
gdalinfo /vsis3/bucket-name/path/to/dataset.zarr

# Or use anonymous access
gdalinfo /vsis3/public-bucket/dataset.zarr
```

#### Using with HTTP/HTTPS
```bash
# Access data via HTTP
gdalinfo /vsicurl/https://example.com/data/dataset.zarr

# With authentication
gdalinfo --config GDAL_HTTP_USERPWD username:password \
    /vsicurl/https://example.com/private/dataset.zarr
```

## Data Formats Supported

### Input Formats
- **Zarr v2**: Standard Zarr format with chunked arrays
- **EOPF Zarr**: EOPF-specific Zarr with metadata extensions
- **NetCDF-like**: Zarr stores with NetCDF-compatible metadata

### Supported Features
- Multi-dimensional arrays (2D, 3D, 4D)
- Multiple data types (int8, int16, int32, float32, float64)
- Chunked storage for efficient access
- Compression (gzip, lz4, blosc)
- Metadata attributes

## Performance Tips

### Optimal Chunk Access
```python
# Read data in chunk-aligned windows for better performance
chunk_size = 512  # Match your dataset's chunk size
x_off = (x_off // chunk_size) * chunk_size
y_off = (y_off // chunk_size) * chunk_size
```

### Memory Management
```python
# For large datasets, process in blocks
def process_large_dataset(dataset, block_size=1024):
    for y in range(0, dataset.RasterYSize, block_size):
        for x in range(0, dataset.RasterXSize, block_size):
            # Calculate actual block size (handle edges)
            x_size = min(block_size, dataset.RasterXSize - x)
            y_size = min(block_size, dataset.RasterYSize - y)
            
            # Read block
            data = dataset.ReadAsArray(x, y, x_size, y_size)
            
            # Process data
            # ... your processing code ...
```

### Caching
```bash
# Enable GDAL caching for better performance
export GDAL_CACHEMAX=512  # MB
export VSI_CACHE=TRUE
export VSI_CACHE_SIZE=25000000  # bytes
```

## Common Use Cases

### 1. Time Series Analysis
```python
# Access time dimension in 4D dataset
dataset = gdal.Open('time_series.zarr')
subdatasets = dataset.GetSubDatasets()

for subdataset_name, subdataset_desc in subdatasets:
    if 'time=' in subdataset_name:
        time_slice = gdal.Open(subdataset_name)
        # Process time slice
        data = time_slice.ReadAsArray()
        # ... analysis ...
```

### 2. Multi-spectral Processing
```python
# Process all bands in a multi-spectral dataset
dataset = gdal.Open('multispectral.zarr')

# Calculate NDVI (assuming bands 4=NIR, 3=Red)
nir = dataset.GetRasterBand(4).ReadAsArray().astype(float)
red = dataset.GetRasterBand(3).ReadAsArray().astype(float)

ndvi = (nir - red) / (nir + red)
```

### 3. Subset Extraction
```bash
# Extract geographic subset
gdal_translate -projwin <ulx> <uly> <lrx> <lry> \
    input.zarr subset.tif

# Extract by pixel coordinates
gdal_translate -srcwin <xoff> <yoff> <xsize> <ysize> \
    input.zarr subset.tif
```

## Troubleshooting

### Performance Issues
- Use appropriate chunk sizes
- Enable caching
- Consider data locality (local vs. remote)

### Memory Issues
- Process data in blocks
- Increase GDAL cache size
- Use appropriate data types

### Reading Errors
- Check file permissions
- Verify EOPF metadata format
- Enable debug logging: `export CPL_DEBUG=ON`
