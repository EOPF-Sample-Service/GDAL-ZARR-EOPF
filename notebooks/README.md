# GDAL-ZARR-EOPF Notebooks

This folder contains Jupyter notebooks demonstrating the EOPF-Zarr GDAL driver functionality and compatibility testing.

## 📚 Available Notebooks

### Core Functionality Notebooks

#### `01-Basic-Functionality-Demo.ipynb`

- **Purpose**: Demonstrates basic EOPFZARR driver functionality
- **Features**:
  - Driver registration and detection
  - Local and remote data access
  - Subdataset enumeration
  - Basic data reading operations
- **Use Case**: Getting started with EOPFZARR driver

#### `02-Remote-Data-Access-Demo.ipynb`

- **Purpose**: Focuses on remote data access capabilities
- **Features**:
  - VSI/curl integration for remote URLs
  - Authentication handling
  - Performance considerations
  - Error handling and troubleshooting
- **Use Case**: Working with cloud-hosted ZARR data

#### `03-Rasterio-Compatibility-Demo.ipynb`

- **Purpose**: Comprehensive rasterio integration testing
- **Features**:
  - ZARR vs EOPFZARR compatibility analysis
  - URL scheme recognition issues
  - Workarounds and solutions
  - Performance comparisons
- **Use Case**: Understanding rasterio limitations and workarounds

### Testing and Analysis Notebooks

#### `EOPF-Zarr-Test.ipynb`

- **Purpose**: Docker environment testing and validation
- **Features**:
  - Environment setup verification
  - Driver installation testing
  - Sample data creation and testing
  - End-to-end functionality validation
- **Use Case**: CI/CD testing and environment validation

#### `zarr_rasterio_testing.ipynb`

- **Purpose**: Clean ZARR + rasterio testing framework
- **Features**:
  - Local and remote data testing
  - Subdataset access patterns
  - Data visualization examples
  - GDAL vs rasterio comparison
- **Use Case**: Systematic testing of ZARR driver with rasterio

#### `visualize_zarr.ipynb`

- **Purpose**: Data visualization and analysis examples
- **Features**:
  - ZARR and EOPFZARR side-by-side comparison
  - Data decimation and visualization
  - Performance optimization techniques
  - Matplotlib integration
- **Use Case**: Visualizing geospatial ZARR data

## 🚀 Quick Start

### 📦 Installation Options

#### Option 1: Docker Environment (Recommended)

**Pre-built Image (Fastest)**:
```bash
# Pull the ready-to-use Docker image
docker pull yuvraj1989/eopf-zarr-driver:latest

# Run with JupyterLab
docker run -p 8888:8888 yuvraj1989/eopf-zarr-driver:latest
# ➜ Access Jupyter at: http://localhost:8888
```

**Docker Compose (Development)**:
```bash
# Clone repository and use docker-compose
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF
docker-compose up --build
# ➜ Access Jupyter at: http://localhost:8888
```

#### Option 2: Manual Installation

**Download Pre-built Driver**:
```bash
# Get latest release for your platform
# Linux
wget https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/releases/latest/download/gdal_EOPFZarr-Linux.zip

# macOS  
wget https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/releases/latest/download/gdal_EOPFZarr-macOS.zip

# Windows
curl -L https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/releases/latest/download/gdal_EOPFZarr-Windows.zip -o driver.zip
```

**Install Dependencies**:
```bash
# Required packages
pip install gdal rasterio xarray zarr matplotlib numpy pandas jupyter
```

**Configure Environment**:
```bash
# Set environment variables (Linux/macOS)
export GDAL_DRIVER_PATH=/path/to/extracted/driver
export GDAL_DATA=/usr/share/gdal

# Windows PowerShell
$env:GDAL_DRIVER_PATH="C:\path\to\extracted\driver"
$env:GDAL_DATA="C:\Program Files\GDAL\share\gdal"
```

**Verify Installation**:
```bash
# Check if driver is loaded
gdalinfo --formats | grep EOPF
# Should show: EOPFZARR -raster- (rw+): EOPF Zarr
```

#### Option 3: Build from Source

```bash
# Clone and build
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Set environment
export GDAL_DRIVER_PATH=$PWD/build
```

### 📖 Complete Documentation

- **Installation Guide**: [../INSTALLATION.md](../INSTALLATION.md)
- **Docker Quick Start**: [../DOCKER_QUICKSTART.md](../DOCKER_QUICKSTART.md)
- **Troubleshooting**: [../TROUBLESHOOTING.md](../TROUBLESHOOTING.md)

## 📖 Usage Examples

### Basic EOPFZARR Usage

```python
from osgeo import gdal

# Local data
ds = gdal.Open('EOPFZARR:"/path/to/data.zarr"')

# Remote data  
ds = gdal.Open('EOPFZARR:"/vsicurl/https://example.com/data.zarr"')
```

### Working with Rasterio

```python
import rasterio

# Use ZARR scheme for rasterio compatibility
with rasterio.open('ZARR:"/vsicurl/https://example.com/data.zarr"') as src:
    data = src.read(1)
```

## 🔧 Key Findings and Best Practices

### Path Formatting

- ✅ **Local data**: `EOPFZARR:"/path/to/data.zarr"`
- ✅ **Remote data**: `EOPFZARR:"/vsicurl/https://example.com/data.zarr"`
- ⚠️ **Important**: Always use quotes around paths containing special characters

### Rasterio Compatibility

- ✅ **ZARR driver**: Full rasterio support
- ❌ **EOPFZARR driver**: Limited rasterio support due to URL scheme recognition
- 🔧 **Workaround**: Use GDAL directly or ZARR scheme for rasterio compatibility

### Performance Tips

- Use `gdal.SetConfigOption("ZARR_ALLOW_BIG_TILE_SIZE", "YES")` for large tiles
- Enable caching for repeated access to remote data
- Use subdataset URLs for direct array access

## 🎯 Common Use Cases

### Scientific Data Analysis

1. Start with `01-Basic-Functionality-Demo.ipynb`
2. Use `zarr_rasterio_testing.ipynb` for data exploration
3. Apply `visualize_zarr.ipynb` for visualization

### Production Integration

1. Review `02-Remote-Data-Access-Demo.ipynb` for remote data patterns
2. Check `03-Rasterio-Compatibility-Demo.ipynb` for integration considerations
3. Use `EOPF-Zarr-Test.ipynb` for deployment validation

### Development and Testing

1. Use `EOPF-Zarr-Test.ipynb` for environment setup
2. Apply `zarr_rasterio_testing.ipynb` for systematic testing
3. Reference all notebooks for comprehensive examples

## 📊 Compatibility Matrix

| Operation | GDAL + EOPFZARR | GDAL + ZARR | Rasterio + ZARR | Rasterio + EOPFZARR |
|-----------|------------------|-------------|------------------|---------------------|
| Local data | ✅ Full support | ✅ Full support | ✅ Full support | ❌ URL scheme issue |
| Remote data | ✅ Full support | ✅ Full support | ✅ Full support | ❌ URL scheme issue |
| Subdatasets | ✅ Full support | ✅ Full support | ✅ Full support | ❌ URL scheme issue |

## 🐛 Troubleshooting

### Common Issues

#### "EOPFZARR driver not found"

- Ensure driver is compiled and installed
- Check `GDAL_DRIVER_PATH` environment variable
- Verify with `gdal.GetDriverByName('EOPFZARR')`

#### "No such file or directory" with rasterio

- This is expected - rasterio doesn't recognize EOPFZARR scheme
- Use ZARR scheme or GDAL directly

#### Remote data access fails

- Check URL format: `"/vsicurl/https://..."`
- Verify network connectivity
- Check authentication if required

### Debug Commands

```python
# Check available drivers
for i in range(gdal.GetDriverCount()):
    driver = gdal.GetDriver(i)
    if 'zarr' in driver.GetDescription().lower():
        print(f"Found: {driver.GetDescription()}")

# Enable GDAL debugging
gdal.SetConfigOption('CPL_DEBUG', 'ON')
```

## 📚 Additional Resources

- [GDAL ZARR Driver Documentation](https://gdal.org/drivers/raster/zarr.html)
- [Rasterio Documentation](https://rasterio.readthedocs.io/)
- [EOPF Project Documentation](https://eopf.readthedocs.io/)

## 🤝 Contributing

When adding new notebooks:

1. Follow the naming convention: `NN-Purpose-Demo.ipynb`
2. Include clear documentation and examples
3. Test with both local and remote data when applicable
4. Update this README with the new notebook description

## 📝 License

See the main repository LICENSE file for details.
