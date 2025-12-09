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

## Working with Swath Data and Geolocation Arrays

The EOPFZARR driver supports native geolocation arrays for satellite swath data (e.g., Sentinel-3 SLSTR). These datasets have curved observation geometries that cannot be accurately represented by a simple rectangular bounding box.

### Understanding Geolocation Arrays

When you open a swath dataset directly, GDAL provides a **GeoTransform** (rectangular bounding box approximation):
- Fast to compute and display
- Works in all applications
- **Distorts the geometry** - stretches curved swath to fit a rectangle

For **accurate georeferencing**, the driver sets **GEOLOCATION metadata** that points to native 2D latitude/longitude arrays:
- Pixel-by-pixel coordinate accuracy
- Preserves true curved swath geometry
- Requires warping for visualization

### Using gdalwarp with Geolocation Arrays

To create an accurately georeferenced output from swath data:

```bash
# Warp using native geolocation arrays
gdalwarp -geoloc \
  -t_srs EPSG:4326 \
  -tr 0.01 0.01 \
  -r near \
  "EOPFZARR:/path/to/sentinel3.zarr/measurements/inadir/s7_bt_in" \
  output_true_geometry.tif
```

**Key flags:**
- `-geoloc` - Use GEOLOCATION metadata (CRITICAL!)
- `-t_srs EPSG:4326` - Target CRS
- `-tr 0.01 0.01` - Target resolution (~1km at equator)
- `-r near` - Resampling method

**Without `-geoloc`**: Output will be stretched (uses GeoTransform)  
**With `-geoloc`**: Output shows TRUE curved swath geometry

### QGIS Workflow for Swath Data

When you load an EOPFZARR swath dataset in QGIS, it will initially show **stretched** (rectangular):

**Option 1: Using Warp Dialog**
1. Open: `Raster → Projections → Warp (Reproject)`
2. Select your EOPFZARR layer as input
3. In **"Additional command-line parameters"** field, enter: `-geoloc`
4. Click **Run**
5. Result: New layer with TRUE swath geometry

**Option 2: Using Processing Toolbox**
1. Search for `gdalwarp` in Processing Toolbox
2. Open: `GDAL → Warp (reproject)`
3. Input layer: Your EOPFZARR layer
4. **Additional command-line parameters**: `-geoloc`
5. Run

**Option 3: Python Console**
```python
import processing

processing.run("gdal:warpreproject", {
    'INPUT': 'EOPFZARR:/path/to/data.zarr/measurements/inadir/band',
    'TARGET_CRS': 'EPSG:4326',
    'RESAMPLING': 0,
    'EXTRA': '-geoloc',  # Key parameter!
    'OUTPUT': '/path/to/output.tif'
})
```

### Accessing Native Lat/Lon Arrays Directly

You can also access the geolocation arrays as separate datasets:

```python
from osgeo import gdal

# Open the main dataset
ds = gdal.Open("EOPFZARR:/path/to/data.zarr/measurements/inadir/s7_bt_in")

# Get geolocation metadata
geoloc_md = ds.GetMetadata('GEOLOCATION')
x_dataset_path = geoloc_md['X_DATASET']  # Longitude array
y_dataset_path = geoloc_md['Y_DATASET']  # Latitude array

# Open the lat/lon arrays
ds_lon = gdal.Open(x_dataset_path)
ds_lat = gdal.Open(y_dataset_path)

# Read as arrays (same size as data)
lon = ds_lon.ReadAsArray()
lat = ds_lat.ReadAsArray()

# Now you have pixel-by-pixel coordinates
# Each pixel in your data has corresponding lat[i,j], lon[i,j]
```

### Performance Considerations

Warping with geolocation arrays:
- **Slower** than simple GeoTransform (requires reading lat/lon arrays)
- **More accurate** - preserves true geometry
- **Creates larger outputs** - target grid must encompass entire swath

Tips:
- Use appropriate target resolution (`-tr`) to balance quality vs size
- Consider target extent (`-te`) to crop output area
- Use compression for output: `-co COMPRESS=LZW`

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
- Geolocation array detection
- Performance optimizations

## Configuration Options

### Suppressing Auxiliary File Warnings

When opening remote datasets, GDAL's PAM (Persistent Auxiliary Metadata) system may try to save `.aux.xml` files alongside the data, which fails for remote URLs and produces a warning:

```
Warning 1: Unable to save auxiliary information in ZARR:"/vsicurl/https://...".aux.xml.
```

By default, the EOPFZARR driver **suppresses this warning** for remote datasets. You can control this behavior:

**Using Configuration Option:**
```python
from osgeo import gdal

# Default: warnings are suppressed (same as YES)
gdal.SetConfigOption('EOPFZARR_SUPPRESS_AUX_WARNING', 'YES')

# To see warnings (for debugging):
gdal.SetConfigOption('EOPFZARR_SUPPRESS_AUX_WARNING', 'NO')
```

**Using Open Options:**
```python
# Suppress warnings (default)
ds = gdal.OpenEx(path, open_options=['SUPPRESS_AUX_WARNING=YES'])

# Show warnings (for debugging)
ds = gdal.OpenEx(path, open_options=['SUPPRESS_AUX_WARNING=NO'])
```

**Environment Variable:**
```bash
# Show warnings
export EOPFZARR_SUPPRESS_AUX_WARNING=NO
gdalinfo "EOPFZARR:/vsicurl/https://..."

# Suppress warnings (default)
export EOPFZARR_SUPPRESS_AUX_WARNING=YES
```

### Other Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `EOPFZARR_SUPPRESS_AUX_WARNING` | `YES` | Suppress `.aux.xml` save warnings for remote datasets |
| `EOPF_SHOW_ZARR_ERRORS` | `NO` | Show underlying Zarr driver errors |
| `EOPF_ENABLE_PERFORMANCE_TIMERS` | `NO` | Enable performance timing output |
| `CPL_DEBUG=EOPFZARR` | - | Enable debug logging |

