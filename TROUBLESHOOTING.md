# Troubleshooting

## Driver Not Loading

**Symptom:** `gdalinfo --formats` does not show `EOPFZARR`

1. Check GDAL version (requires 3.10+):
   ```bash
   gdalinfo --version
   ```

2. Verify the plugin is in the right directory:
   ```bash
   ls $(gdal-config --plugindir)/gdal_EOPFZarr*
   ```

3. Set the plugin path manually:
   ```bash
   export GDAL_DRIVER_PATH="/path/to/plugin:$GDAL_DRIVER_PATH"
   ```

## File Not Opening

**Symptom:** "Unable to open dataset" or "Not recognized as a supported file format"

- Always include the `EOPFZARR:` prefix:
  ```bash
  # Correct
  gdalinfo 'EOPFZARR:"/path/to/product.zarr"'

  # Wrong — missing prefix
  gdalinfo /path/to/product.zarr
  ```

- Use the colon-separated subdataset format:
  ```bash
  # Correct
  gdalinfo 'EOPFZARR:"/path/to/product.zarr":measurements/band'

  # Avoid
  gdalinfo 'EOPFZARR:"/path/to/product.zarr/measurements/band"'
  ```

## Zarr Error Messages in Output

Expected during subdataset opening. Suppress with the default settings or show for debugging:

```bash
# Show errors (for debugging)
EOPF_SHOW_ZARR_ERRORS=YES gdalinfo 'EOPFZARR:"/path/to/product.zarr"'
```

## GRD Multi-Band Not Working

Ensure the product is a Sentinel-1 GRD with dual polarization (VV/VH or HH/HV):

```bash
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr"' --oo GRD_MULTIBAND=YES
```

## Geocoding Issues

Use `CRS:84` (not `EPSG:4326`) to avoid axis order issues with GDAL 3+:

```bash
gdalwarp -geoloc -t_srs CRS:84 \
  'EOPFZARR:"/path/to/product.zarr":measurements/band' output.tif
```

## Debug Mode

```bash
export CPL_DEBUG=EOPFZARR
gdalinfo 'EOPFZARR:"/path/to/product.zarr"'
```
