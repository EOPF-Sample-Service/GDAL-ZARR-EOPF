# Troubleshooting Guide

## Common Issues

### Plugin Not Loading

**Symptoms:**
- `gdalinfo --formats` doesn't show EOPFZARR
- Error: "Unable to load driver EOPFZarr"

**Solutions:**
1. **Check GDAL version:**
   ```bash
   gdalinfo --version
   # Must be 3.10 or higher
   ```

2. **Verify plugin location:**
   ```bash
   # Check if plugin file exists
   ls $(gdal-config --plugindir)/gdal_EOPFZarr*
   ```

3. **Set plugin path manually:**
   ```bash
   export GDAL_DRIVER_PATH="/path/to/plugin:$GDAL_DRIVER_PATH"
   ```

### File Not Opening

**Symptoms:**
- "Unable to open dataset"
- "Not recognized as a supported file format"

**Solutions:**
1. **Check file path format:**
   ```bash
   # Correct format
   gdalinfo EOPFZARR:/path/to/file.zarr
   
   # Incorrect - missing prefix
   gdalinfo /path/to/file.zarr
   ```

2. **Test with debug output:**
   ```bash
   export CPL_DEBUG=EOPFZARR
   gdalinfo EOPFZARR:/path/to/file.zarr
   ```

3. **Verify file structure:**
   ```bash
   # Check for .zarray, .zgroup files
   ls -la /path/to/file.zarr/
   ```

### Remote Access Issues

**Symptoms:**
- Timeouts with remote URLs
- "Network is unreachable"
- Slow performance with cloud data

**Solutions:**
1. **Test basic connectivity:**
   ```bash
   curl -I "https://example.com/data.zarr/.zgroup"
   ```

2. **Enable VSI caching:**
   ```bash
   export VSI_CACHE=TRUE
   export VSI_CACHE_SIZE=64000000  # 64MB
   ```

3. **Configure timeouts:**
   ```bash
   export GDAL_HTTP_TIMEOUT=30
   export GDAL_HTTP_CONNECTTIMEOUT=10
   ```

### Performance Issues

**Symptoms:**
- Slow dataset opening
- High memory usage
- Long processing times

**Solutions:**
1. **Enable performance monitoring:**
   ```bash
   export EOPF_ENABLE_PERFORMANCE_TIMERS=1
   export CPL_DEBUG=EOPFZARR_PERF
   ```

2. **Optimize caching:**
   ```bash
   export GDAL_CACHEMAX=512  # MB
   export VSI_CACHE=TRUE
   ```

3. **Use block-aligned access:**
   ```python
   # Read data in chunks matching Zarr blocks
   ds = gdal.Open("EOPFZARR:/path/to/data.zarr")
   block_size = ds.GetRasterBand(1).GetBlockSize()
   # Read in multiples of block_size
   ```

### Build Issues

**Symptoms:**
- CMake configuration fails
- Compilation errors
- Linking errors

**Solutions:**
1. **GDAL not found:**
   ```bash
   # Set GDAL paths explicitly
   cmake -DGDAL_ROOT=/path/to/gdal ..
   ```

2. **Compiler compatibility:**
   ```bash
   # Use same compiler as GDAL
   cmake -DCMAKE_CXX_COMPILER=g++ ..
   ```

3. **Missing dependencies:**
   ```bash
   # Install development packages
   sudo apt-get install libgdal-dev  # Ubuntu/Debian
   brew install gdal                 # macOS
   ```

### QGIS Integration Issues

**Symptoms:**
- QGIS doesn't recognize files
- Layer fails to load
- Crashes when opening

**Solutions:**
1. **Check QGIS GDAL version:**
   - Help → About → Show Details
   - Verify GDAL version is 3.10+

2. **Install plugin in QGIS GDAL directory:**
   ```bash
   # Find QGIS GDAL plugin directory
   # Usually different from system GDAL
   ```

3. **Use absolute paths:**
   - Avoid relative paths in QGIS
   - Use full EOPFZARR: prefix

## Debug Information

### Enable All Debug Output
```bash
export CPL_DEBUG=ON
export CPL_DEBUG=EOPFZARR
export EOPF_ENABLE_PERFORMANCE_TIMERS=1
export GDAL_DATA=/path/to/gdal/data
```

### Check System Configuration
```bash
# GDAL configuration
gdalinfo --version
gdalinfo --formats | grep EOPFZARR
gdal-config --plugindir

# System info
uname -a
which gdal-config
echo $PATH
echo $LD_LIBRARY_PATH  # Linux
echo $DYLD_LIBRARY_PATH  # macOS
```

### Test Dataset
```bash
# Test with minimal dataset
gdalinfo -checksum EOPFZARR:/path/to/test.zarr
gdalinfo -stats EOPFZARR:/path/to/test.zarr
```

## Getting Help

If issues persist:

1. **Check GitHub Issues** for similar problems
2. **Enable debug output** and include in issue report
3. **Provide system information** (OS, GDAL version, etc.)
4. **Include sample dataset** if possible
5. **Test with latest version** of the plugin
