# Issue: EOPFZARR Driver Not Using Native Geolocation Arrays

## Problem Statement

The EOPFZARR driver currently uses STAC bbox metadata to create a linear GeoTransform for georeferencing. However, EOPF Zarr products contain **actual 2D latitude/longitude coordinate arrays** for each pixel that provide precise geolocation.

### Current Behavior

From the user's observation, when opening EOPF Zarr products with xarray's eopf-zarr engine, the datasets show:

```
Dimensions:         (lon: 3326, lat: 2965)
Coordinates:
  * lon             (lon) float64 27kB 40.79 40.79 40.79 ... 56.19 56.19 56.19
  * lat             (lat) float64 24kB 20.93 20.92 20.92 ... 7.622 7.618 7.613
    spatial_ref     int64 8B ...
Data variables: (12/29)
    elevation       (lat, lon) float64 79MB dask.array<chunksize=(2048, 2048)>
    s1_radiance_an  (lat, lon) float64 ...
```

The GDAL driver is **not using these coordinate arrays**. Instead, it's creating an approximate linear GeoTransform from the bbox metadata.

### Impact

1. **Incorrect Georeferencing**: The bounding box provides only corner coordinates, resulting in:
   - Linear interpolation between corners
   - No handling of satellite swath geometry
   - Inaccurate pixel locations, especially for:
     - SLSTR dual-view products (nadir vs oblique have different geometries)
     - Products with curved swaths
     - High-latitude regions with projection distortion

2. **Wrong Orientation**: The bbox-based approach puts the image in the "right box" but not at the "right orientation" - the actual pixel coordinates may show curvature or distortion that a linear transform cannot represent.

### Example: Sentinel-3 SLSTR

Product: `S3A_SL_1_RBT____20251019T064521_20251019T064821_20251019T085627_0179_131_348_2700_PS1_O_NR_004.zarr`

- Location: Djibouti/Horn of Africa (~40-56°E, 7-21°N)
- Current GDAL behavior: Creates linear GeoTransform from bbox
- Actual Zarr structure: Contains `lat` and `lon` coordinate arrays with precise per-pixel geolocation

### Solution Approaches

#### Option 1: Export Lat/Lon as Geolocation Arrays (RECOMMENDED)

GDAL supports **Geolocation Arrays** through metadata domains. The driver should:

1. Check if the Zarr group contains `latitude` and `longitude` arrays
2. If found, set them as GDAL Geolocation Arrays using:
   ```cpp
   char **papszMD = nullptr;
   papszMD = CSLSetNameValue(papszMD, "LINE_OFFSET", "0");
   papszMD = CSLSetNameValue(papszMD, "LINE_STEP", "1");
   papszMD = CSLSetNameValue(papszMD, "PIXEL_OFFSET", "0");
   papszMD = CSLSetNameValue(papszMD, "PIXEL_STEP", "1");
   papszMD = CSLSetNameValue(papszMD, "X_DATASET", "path/to/longitude/array");
   papszMD = CSLSetNameValue(papszMD, "X_BAND", "1");
   papszMD = CSLSetNameValue(papszMD, "Y_DATASET", "path/to/latitude/array");
   papszMD = CSLSetNameValue(papszMD, "Y_BAND", "1");
   poDS->SetMetadata(papszMD, "GEOLOCATION");
   ```

3. This allows:
   - Tools like `gdalwarp` to use precise pixel locations for reprojection
   - Applications to access actual lat/lon for each pixel
   - Accurate georeferencing without requiring a linear transform

#### Option 2: Create Subdatasets for Lat/Lon Arrays

Expose latitude and longitude as separate GDAL subdatasets:
- `EOPFZARR:"path":measurements/inadir/latitude`
- `EOPFZARR:"path":measurements/inadir/longitude`

This allows users to:
- Manually reference them in geolocation-aware workflows
- Use them with `gdallocationinfo` and other GDAL utilities

#### Option 3: Hybrid Approach

1. Keep the linear GeoTransform for basic compatibility
2. **Also** provide geolocation arrays for precise georeferencing
3. Let GDAL-aware applications choose which to use

### Implementation Tasks

- [x] Detect if latitude/longitude coordinate arrays exist in Zarr group
- [x] Read latitude/longitude arrays and expose as GDAL Geolocation Arrays
- [x] Update metadata to indicate geolocation arrays are available
- [ ] Test with `gdalinfo` to verify GEOLOCATION metadata
- [ ] Test with `gdalwarp` to verify reprojection uses precise coordinates
- [ ] Test with rioxarray to ensure compatibility
- [ ] Create C++ unit tests
- [ ] Document in USAGE.md how to access geolocation arrays

### Implementation Details

**Branch**: `137-issue-eopfzarr-driver-not-using-native-geolocation-arrays`

**Implementation Approach**: Hybrid (Option 3)
- Keeps existing GeoTransform for backwards compatibility
- Adds GEOLOCATION metadata domain pointing to lat/lon arrays
- GDAL-aware tools can choose which to use

**Key Changes**:

1. **eopfzarr_dataset.h**: Added `void ProcessGeolocationArrays();` method declaration

2. **eopfzarr_dataset.cpp**: 
   - Implemented `ProcessGeolocationArrays()` method (called from `LoadGeospatialInfo()`)
   - Detection logic:
     - Checks for lat/lon arrays in the same Zarr group as the measurement data
     - Looks for common naming: "latitude"/"longitude", "lat"/"lon"
     - Searches through Zarr subdatasets metadata
   - Creates EOPFZARR subdataset paths for lat/lon arrays
   - Sets GEOLOCATION metadata following GDAL conventions:
     ```cpp
     SetMetadataItem("X_DATASET", lonDataset.c_str(), "GEOLOCATION");
     SetMetadataItem("X_BAND", "1", "GEOLOCATION");
     SetMetadataItem("Y_DATASET", latDataset.c_str(), "GEOLOCATION");
     SetMetadataItem("Y_BAND", "1", "GEOLOCATION");
     SetMetadataItem("PIXEL_OFFSET", "0", "GEOLOCATION");
     SetMetadataItem("LINE_OFFSET", "0", "GEOLOCATION");
     SetMetadataItem("PIXEL_STEP", "1", "GEOLOCATION");
     SetMetadataItem("LINE_STEP", "1", "GEOLOCATION");
     SetMetadataItem("SRS", WGS84_WKT, "GEOLOCATION");
     ```

**Reference Implementation**: Followed HDF4 driver pattern (hdf4imagedataset.cpp lines 1928-1957)

**Testing**:
- Created `test_geolocation_arrays.py` for validation
- Expected output from `gdalinfo`:
  ```
  Metadata (GEOLOCATION):
    X_DATASET=EOPFZARR:"<url>":/measurements/inadir/longitude
    X_BAND=1
    Y_DATASET=EOPFZARR:"<url>":/measurements/inadir/latitude
    Y_BAND=1
    PIXEL_OFFSET=0
    LINE_OFFSET=0
    PIXEL_STEP=1
    LINE_STEP=1
    SRS=GEOGCS["WGS 84",...]
  ```

**Usage with gdalwarp**:
```bash
# Use geolocation arrays for precise reprojection
gdalwarp -geoloc EOPFZARR:"url":/measurements/inadir/s7_bt_in output.tif

# Without -geoloc flag, uses bbox-based GeoTransform
gdalwarp EOPFZARR:"url":/measurements/inadir/s7_bt_in output.tif
```

### References

- GDAL Geolocation Arrays: https://gdal.org/tutorials/geolocation.html
- HDF5 Swath example (similar concept): https://gdal.org/drivers/raster/hdf5.html#geolocation-information
- EOPF Zarr specification: Check for coordinate array standards

### Related Issues

- #135 - EOPF bbox ordering and UTM geotransform bugs (fixed bbox ordering, but still using bbox-based approach)

### Priority

**HIGH** - This affects the accuracy of all georeferenced EOPF products opened with the EOPFZARR driver. The current bbox-based approach provides only approximate locations.
