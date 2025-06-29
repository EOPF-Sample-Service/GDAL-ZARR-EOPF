# Frequently Asked Questions (FAQ)

## General Questions

### What is the EOPF-Zarr GDAL plugin?
The EOPF-Zarr GDAL plugin extends GDAL with support for the Earth Observation Processing Framework (EOPF) data format, which is commonly stored in Zarr format. It allows you to open, read, and process EOPF datasets using any GDAL-compatible application.

### What file formats are supported?
- **Zarr v2**: Standard Zarr format with chunked arrays
- **EOPF Zarr**: EOPF-specific Zarr with metadata extensions  
- **NetCDF-like**: Zarr stores with NetCDF-compatible metadata
- File extensions: `.zarr`, `.eopf`

### Is this plugin officially supported by GDAL?
This is a community-developed plugin. While it follows GDAL's plugin architecture and standards, it is not part of the official GDAL distribution.

## Installation Issues

### The plugin doesn't load after installation
**Solution:**
1. Check that the plugin is in GDAL's plugin directory:
   ```bash
   gdal-config --prefix
   ls $(gdal-config --prefix)/lib/gdalplugins/
   ```
2. Set the `GDAL_DRIVER_PATH` environment variable:
   ```bash
   export GDAL_DRIVER_PATH="/path/to/plugins:$GDAL_DRIVER_PATH"
   ```
3. Verify GDAL can see the driver:
   ```bash
   gdalinfo --formats | grep -i eopf
   ```

### Build fails with "GDAL not found"
**Solution:**
1. Install GDAL development headers:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libgdal-dev
   
   # macOS
   brew install gdal
   
   # CentOS/RHEL  
   sudo yum install gdal-devel
   ```
2. If GDAL is installed in a custom location:
   ```bash
   cmake .. -DGDAL_ROOT=/path/to/gdal
   ```

### CMake can't find dependencies
**Solution:**
```bash
# Update CMake
sudo apt-get update && sudo apt-get install cmake

# For custom installations, set paths:
export CMAKE_PREFIX_PATH="/usr/local:/opt/local:$CMAKE_PREFIX_PATH"
```

## Usage Questions

### How do I open an EOPF dataset?
```bash
# Command line
gdalinfo dataset.zarr

# Python
from osgeo import gdal
dataset = gdal.Open('dataset.zarr')
```

### Can I use this with QGIS?
Yes! Once the plugin is installed:
1. Open QGIS
2. `Layer` → `Add Layer` → `Add Raster Layer`
3. Browse to your `.zarr` file
4. Click `Open`

### How do I access subdatasets?
Many EOPF datasets contain multiple subdatasets (e.g., different variables or time slices):

```bash
# List subdatasets
gdalinfo dataset.zarr

# Open specific subdataset
gdalinfo 'EOPF:dataset.zarr:variable_name'
```

### Performance is slow when reading data
**Solutions:**
1. **Enable caching:**
   ```bash
   export GDAL_CACHEMAX=512  # MB
   export VSI_CACHE=TRUE
   ```

2. **Read data in chunks:**
   ```python
   # Align reads with dataset chunks
   data = dataset.ReadAsArray(x, y, chunk_size, chunk_size)
   ```

3. **Use appropriate data types:**
   ```python
   # Don't unnecessarily convert to float64
   data = band.ReadAsArray().astype(np.float32)
   ```

## Cloud Access

### How do I access data from cloud storage?
```bash
# S3 (requires credentials)
export AWS_ACCESS_KEY_ID=your_key
export AWS_SECRET_ACCESS_KEY=your_secret
gdalinfo /vsis3/bucket-name/dataset.zarr

# Public S3 bucket
gdalinfo /vsis3/public-bucket/dataset.zarr

# HTTP/HTTPS
gdalinfo /vsicurl/https://example.com/dataset.zarr
```

### Authentication for private cloud data?
**S3:**
```bash
export AWS_ACCESS_KEY_ID=your_key
export AWS_SECRET_ACCESS_KEY=your_secret
# or use ~/.aws/credentials
```

**Azure:**
```bash
export AZURE_STORAGE_ACCOUNT=account_name
export AZURE_STORAGE_ACCESS_KEY=access_key
```

**Google Cloud:**
```bash
export GOOGLE_APPLICATION_CREDENTIALS=/path/to/service-account.json
```

## Error Messages

### "Failed to open dataset"
**Possible causes:**
1. File doesn't exist or no read permissions
2. Invalid Zarr format
3. Missing required metadata

**Debug steps:**
```bash
export CPL_DEBUG=ON
gdalinfo dataset.zarr
```

### "No such driver: EOPF"
**Solution:**
The plugin isn't loaded. Check installation and `GDAL_DRIVER_PATH`.

### "Cannot read chunk"
**Possible causes:**
1. Corrupted data
2. Unsupported compression
3. Network issues (for cloud data)

**Solutions:**
1. Verify data integrity
2. Check supported compression formats
3. Test network connectivity

## Data Format Questions

### What Zarr versions are supported?
Currently only Zarr v2 is supported. Zarr v3 support is planned for future releases.

### Are compressed datasets supported?
Yes, the following compression formats are supported:
- gzip
- lz4 (if available)
- blosc (if available)

### Can I write data to Zarr format?
Write support is planned but not yet implemented. Currently, the plugin is read-only.

### How are multi-dimensional arrays handled?
- 2D arrays: Standard raster bands
- 3D arrays: Multiple bands or subdatasets
- 4D arrays: Subdatasets for each time slice or level

## Development Questions

### How can I contribute?
1. Check the [Contributing Guide](CONTRIBUTING.md)
2. Look for issues labeled "good first issue"
3. Fork the repository and create feature branches
4. Submit pull requests with tests and documentation

### How do I report bugs?
1. Check existing [GitHub Issues](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues)
2. Create a new issue with:
   - Operating system and versions
   - Sample data (if possible)
   - Steps to reproduce
   - Error messages

### Where can I get help?
1. Check this FAQ
2. Read the [User Guide](docs/user-guide.md)
3. Search GitHub Issues
4. Create a new issue for specific problems

## Performance Questions

### What's the recommended chunk size?
- For local files: 256-512 pixels typically optimal
- For cloud access: Larger chunks (1024+) often better
- Match your typical access patterns

### How much memory does the plugin use?
Memory usage depends on:
- Chunk sizes being read
- Number of bands accessed simultaneously
- GDAL cache settings (`GDAL_CACHEMAX`)

### Can I process very large datasets?
Yes, the plugin supports:
- Streaming access (doesn't load entire dataset)
- Block-wise processing
- Virtual file systems for cloud data

## Compatibility

### Which GDAL versions are supported?
- **Minimum:** GDAL 3.4.0
- **Recommended:** GDAL 3.6.0+
- **Latest:** Continuously tested against latest GDAL

### Which operating systems work?
- Linux (Ubuntu 20.04+, CentOS 8+)
- macOS (10.15+)
- Windows (with appropriate build environment)

### Python version requirements?
- Python 3.8+ for testing and examples
- Python bindings follow GDAL's Python support policy

## Still Need Help?

If your question isn't answered here:
1. Check the [User Guide](user-guide.md) for detailed usage examples
2. Review the [API Documentation](api.md) for technical details
3. Search or create a [GitHub Issue](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues)
4. Join the discussion in our GitHub Discussions (if enabled)
