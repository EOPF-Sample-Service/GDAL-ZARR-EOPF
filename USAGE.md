# Usage Guide

## Path Format

The recommended format uses a colon separator between the base path and subdataset:

```
EOPFZARR:"<path_or_url>:<subdataset_path>"
```

For remote data, wrap the URL with `/vsicurl/`:

```
EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04
```

## Command Line Examples

```bash
# List subdatasets
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/product.zarr"'

# Open a subdataset
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04'

# Convert to GeoTIFF
gdal_translate \
  'EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04' \
  output.tif

# Sentinel-1 GRD multi-band
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr"' --oo GRD_MULTIBAND=YES

# Sentinel-1 SLC burst selection
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/S1_SLC.zarr"' --oo BURST=IW1_VV_001

# Geocode SAR data via GCPs
gdalwarp -geoloc -t_srs CRS:84 \
  'EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr":measurements/ew/hh_grd' \
  output_geocoded.tif
```

## Python Examples

```python
from osgeo import gdal

# List subdatasets
ds = gdal.Open('EOPFZARR:"/vsicurl/https://example.com/product.zarr"')
for key, val in ds.GetMetadata("SUBDATASETS").items():
    if "_NAME" in key:
        print(val)

# Read a subdataset
sub = gdal.Open('EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04')
array = sub.ReadAsArray()
gt = sub.GetGeoTransform()
proj = sub.GetProjection()

# Sentinel-1 GRD multi-band
ds = gdal.OpenEx(
    'EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr"',
    open_options=["GRD_MULTIBAND=YES"]
)
print(ds.RasterCount)  # 2 bands: VV, VH

# Sentinel-1 SLC burst
ds = gdal.OpenEx(
    'EOPFZARR:"/vsicurl/https://example.com/S1_SLC.zarr"',
    open_options=["BURST=IW1_VV_001"]
)

# Read EOPF metadata
meta = ds.GetMetadata("EOPF")
print(meta.get("EOPF_PRODUCT_TYPE"))
print(meta.get("EOPF_MISSION_DATATAKE_ID"))
```

## Using with rioxarray

```python
import rioxarray

da = rioxarray.open_rasterio(
    'EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04'
)
print(da.rio.crs)
print(da.rio.bounds())
da.rio.to_raster("output.tif")
```

## Using with rasterio

```python
import rasterio

with rasterio.open(
    'EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04'
) as src:
    print(src.crs, src.bounds)
    data = src.read(1)
```

## Open Options Reference

| Option | Values | Description |
|---|---|---|
| `GRD_MULTIBAND` | `YES` / `NO` (default) | Combine VV+VH or HH+HV polarizations into a 2-band dataset |
| `BURST` | e.g. `IW1_VV_001` | Select a specific SLC burst |

## Environment Variables

| Variable | Values | Description |
|---|---|---|
| `EOPF_SHOW_ZARR_ERRORS` | `YES` / `NO` (default) | Show Zarr driver messages during subdataset opening |
