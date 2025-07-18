# Usage Guide

## Basic Usage

### Opening Datasets

**With QGIS:**
1. Open QGIS
2. Layer → Add Raster Layer
3. Browse to your `.zarr` file
4. Select and open

**Command Line:**
```bash
# Get dataset information
gdalinfo EOPFZARR:/path/to/data.zarr

# Convert to GeoTIFF
gdal_translate EOPFZARR:/path/to/data.zarr output.tif

# Reproject dataset
gdalwarp -t_srs EPSG:4326 EOPFZARR:/path/to/data.zarr reprojected.tif
```

**Python:**
```python
from osgeo import gdal

# Open dataset
ds = gdal.Open("EOPFZARR:/path/to/data.zarr")

# Read as array
array = ds.ReadAsArray()

# Get geospatial info
geotransform = ds.GetGeoTransform()
projection = ds.GetProjection()
```

## Working with Subdatasets

Many EOPF datasets contain multiple subdatasets:

```bash
# List subdatasets
gdalinfo EOPFZARR:/path/to/data.zarr

# Open specific subdataset
gdalinfo "EOPFZARR:/path/to/data.zarr/measurements/band1"
```

```python
# Python subdataset access
ds = gdal.Open("EOPFZARR:/path/to/data.zarr")
subdatasets = ds.GetMetadata("SUBDATASETS")

for key, value in subdatasets.items():
    if "_NAME" in key:
        sub_ds = gdal.Open(value)
        print(f"Subdataset: {value}")
```

## Remote Data Access

The plugin supports cloud-native access:

```bash
# Access remote data
gdalinfo "EOPFZARR:/vsicurl/https://example.com/data.zarr"

# Convert remote data
gdal_translate "EOPFZARR:/vsicurl/https://example.com/data.zarr" local.tif
```

## Performance Tips

- **Enable caching** for remote data:
  ```bash
  export GDAL_DISABLE_READDIR_ON_OPEN=EMPTY_DIR
  export VSI_CACHE=TRUE
  ```

- **Use overviews** for large datasets:
  ```bash
  gdaladdo input.tif 2 4 8 16
  ```

- **Optimize chunk reading** by accessing data in chunks that match the Zarr structure

## Debugging

Enable debug output to troubleshoot issues:

```bash
export CPL_DEBUG=EOPFZARR
export CPL_DEBUG=ON
```

This will show detailed information about:
- Dataset opening process
- Metadata loading
- Geospatial coordinate detection
- Performance optimizations
