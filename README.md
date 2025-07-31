# GDAL EOPF-Zarr Plugin

[![Build Status](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/main.yml/badge.svg)](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/main.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A GDAL driver plugin for reading EOPF (Earth Observation Processing Framework) Zarr datasets.

## Features

- **QGIS integration** - Works with "Add Raster Layer"
- **Geospatial intelligence** - Automatic CRS and geotransform detection  
- **Performance optimized** - Caching, lazy loading, block prefetching
- **Cloud native** - HTTP/HTTPS and virtual file system support
- **Cross-platform** - Windows, macOS, Linux

## Quick Start

1. **Check GDAL version** (requires 3.10+):

   ```bash
   gdalinfo --version
   ```

2. **Build and install**:

   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   cmake --install . --config Release
   ```

3. **Test installation**:

   ```bash
   gdalinfo --formats | grep EOPFZARR
   ```

4. **Use with QGIS**: Add raster layer with `EOPFZARR:/path/to/data.zarr`

## Usage

### Command Line

```bash
# Get dataset info
gdalinfo 'EOPFZARR:"/path/to/data.zarr"'

# Convert to GeoTIFF
gdal_translate 'EOPFZARR:"/path/to/data.zarr"' output.tif

# Reproject
gdalwarp -t_srs EPSG:4326 'EOPFZARR:"/path/to/data.zarr"' reprojected.tif
```

### Python

```python
from osgeo import gdal

# Open dataset
ds = gdal.Open('EOPFZARR:"/path/to/data.zarr"')

# Access subdatasets
subdatasets = ds.GetMetadata("SUBDATASETS")
sub_ds = gdal.Open(subdatasets["SUBDATASET_1_NAME"])

# Read as NumPy array
array = ds.ReadAsArray()
```

## Building

### Requirements

- GDAL 3.10+ with development headers
- CMake 3.16+
- C++14 compatible compiler

### Windows

```cmd
# Using vcpkg (recommended)
vcpkg install gdal[core]

# Build
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release
```

### Linux/macOS

```bash
# Install GDAL development packages
# Ubuntu/Debian: sudo apt-get install libgdal-dev
# macOS: brew install gdal

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## Testing

The project includes comprehensive testing following industry best practices:

### Test Types

- **Unit Tests (C++)** - Core functionality and GDAL integration
- **Integration Tests (Python)** - End-to-end driver functionality using pytest
- **Performance Tests** - Caching and optimization validation
- **Compatibility Tests** - Different Zarr format support


### Manual Testing

```bash
# C++ unit tests via CTest
cd build
ctest -C Release --verbose

# Python integration tests via pytest  
pytest tests/integration/ -v

# Generate test data (if needed)
python tests/generate_test_data.py
```

### Test Data

Integration tests use automatically generated Zarr datasets covering:

- Basic functionality
- Subdatasets
- Geospatial information
- EOPF metadata
- Performance testing

### Network Testing

The test suite includes comprehensive network testing:

- **HTTPS URLs**: Tests with real-world EODC datasets
- **VSI Wrappers**: `/vsicurl/` and `/vsis3/` path handling
- **Error Handling**: Network timeouts and invalid URLs
- **Performance**: Caching effectiveness with remote datasets

Example tested HTTPS URL:

```text
https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr
```

### CI/CD Testing

GitHub Actions automatically runs:

- Cross-platform builds (Windows, macOS, Linux)
- C++ unit tests via CTest
- Python integration tests via pytest
- Smoke tests for driver registration

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Documentation

- **[Installation Guide](INSTALLATION.md)** - Detailed setup instructions
- **[Usage Guide](USAGE.md)** - Examples and best practices  
- **[Contributing](CONTRIBUTING.md)** - Developer guidelines
- **[Troubleshooting](TROUBLESHOOTING.md)** - Common issues and solutions
- **[Changelog](CHANGELOG.md)** - Version history and changes