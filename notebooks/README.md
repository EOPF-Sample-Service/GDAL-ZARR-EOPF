# GDAL-ZARR-EOPF Notebooks

This folder contains Jupyter notebooks demonstrating the EOPF-Zarr GDAL driver functionality and compatibility testing.

**📊 Current notebooks: 11 total (4 core + 7 additional)**

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

#### `03-EOPF-Zarr-Test.ipynb`

- **Purpose**: Docker environment testing and validation
- **Features**:
  - Environment setup verification
  - Driver installation testing
  - Sample data creation and testing
  - End-to-end functionality validation
- **Use Case**: CI/CD testing and environment validation

#### `04-Explore_sentinel2_EOPFZARR.ipynb`

- **Purpose**: Explores Sentinel-2 data using the EOPFZARR driver
- **Features**:
  - Real-world Sentinel-2 data exploration
  - EOPF Zarr product structure analysis
  - Demonstration of scientific data workflow
  - Comparison with standard GDAL Zarr driver
- **Use Case**: Scientific data analysis and research workflows

### Additional Notebooks

#### `05-GDAL-ZARR-Comparison.ipynb`

- **Purpose**: Demonstrates standard GDAL ZARR driver functionality for comparison
- **Features**:
  - Basic GDAL ZARR driver usage
  - Remote Sentinel-1 data access
  - Subdataset exploration
  - Metadata retrieval
- **Use Case**: Comparison baseline and standard GDAL workflow reference

#### `06-Data-Visualization.ipynb`

- **Purpose**: Data visualization examples for ZARR datasets
- **Features**:
  - Plotting and visualization techniques
  - Data exploration and analysis
  - Visual comparison workflows
- **Use Case**: Data analysis and presentation

#### `07-Sentinel-3-OLCI-Level-1-EFR.ipynb`

- **Purpose**: Demonstrates Sentinel-3 OLCI data access and visualization using EOPFZARR driver
- **Features**:
  - GDAL native approach to Zarr subdatasets
  - Xarray-style visualization with robust plotting
  - Geographic coordinates access (latitude/longitude)
  - Multi-product exploration (OLCI L1/L2, SLSTR products)
  - Mirrors xarray-eopf functionality using GDAL
- **Use Case**: Sentinel-3 ocean and land color instrument data analysis


#### `08-EOPFZARR-with-Rioxarray.ipynb`

- **Purpose**: Demonstrates how to use the EOPFZARR GDAL driver with rioxarray to work with EOPF Zarr datasets
- **Features**:
  - Open EOPFZarr subdatasets with `rioxarray.open_rasterio()`  
  - Automatic CRS (Coordinate Reference System) handling
  - Spatial bounds and transformations
  - Lazy loading with Dask for efficient memory usage
  - Full xarray functionality (slicing, computations, plotting)
- **Use Case**: NDVI calculation and visualization from Sentinel-3 OLCI data

#### `09-EOPFZARR-with-Rasterio.ipynb`

- **Purpose**: Demonstrates how to use the EOPFZARR GDAL driver with rasterio to work with EOPF Zarr datasets
- **Features**:
  - Open EOPF Zarr datasets with `rasterio.open()`
  - List and access subdatasets
  - Read geospatial metadata (CRS, transform, bounds)
  - Efficient windowed reading
  - NumPy array integration
  - Fast and lightweight (no eager subdataset loading like rioxarray)
- **Use Case**: NDVI calculation and visualization from Sentinel-3 OLCI data

#### `10-Sentinel-3-Multi-Product-Demo.ipynb`

- **Purpose**: Demonstrates working with multiple Sentinel-3 products
- **Features**:
  - Multi-product exploration (OLCI L1/L2, SLSTR products)
  - Product structure comparison
  - Data discovery and navigation
- **Use Case**: Exploring different Sentinel-3 product types

#### `11-Sentinel-1-GRD-Demo.ipynb`

- **Purpose**: Demonstrates accessing Sentinel-1 Ground Range Detected (GRD) SAR data
- **Features**:
  - Sentinel-1 product structure exploration
  - SAR amplitude (GRD) data access and visualization
  - Ground Control Point (GCP) array inspection
  - Understanding sparse geolocation grids
  - Current driver capabilities and limitations with GCP-based data
- **Use Case**: SAR data analysis and understanding Sentinel-1 geolocation differences from optical sensors



## Quick Start

### Installation Options

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

## Usage Examples

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

# Direct access to datasets
with rasterio.open('EOPFZARR:"/vsicurl/https://example.com/data.zarr"') as src:
    data = src.read(1)
    
# Access subdatasets (note the colon separator)
with rasterio.open('EOPFZARR:"/vsicurl/https://example.com/data.zarr":measurements/band') as src:
    data = src.read(1)
```

### Working with Rioxarray

```python
import rioxarray

# Direct access to subdatasets
path = 'EOPFZARR:"/vsicurl/https://example.com/data.zarr":measurements/band'
da = rioxarray.open_rasterio(path)

# Full xarray functionality available
subset = da.isel(x=slice(0, 100), y=slice(0, 100))
result = subset.compute()
```

## 🌍 Data Sources

The notebooks use publicly accessible Earth Observation data:

- **Sentinel-1 SAR**: Synthetic Aperture Radar data
  - Provider: European Space Agency (ESA) via EODC
  - Products: GRD (Ground Range Detected) - Extra Wide mode
  - Access: Public HTTPS endpoints
  - Features: SAR backscatter with Ground Control Points (GCPs)

- **Sentinel-2 MSI L1C**: Multispectral optical imagery
  - Provider: European Space Agency (ESA) via Copernicus Data Space
  - Access: Public S3-compatible object storage

- **Sentinel-3 OLCI**: Ocean and Land Color Instrument data
  - Provider: European Space Agency (ESA) via Copernicus Data Space
  - Products: Level-1 EFR (Earth Observation Full Resolution)
  - Access: Public HTTPS endpoints

- **Custom Test Data**: Small synthetic datasets for testing
  - Generated within notebooks for demonstration purposes

All remote data URLs in notebooks point to publicly accessible endpoints and require no authentication.

## Key Findings and Best Practices

### Path Formatting

- ✅ **Local data**: `EOPFZARR:"/path/to/data.zarr"`
- ✅ **Remote data**: `EOPFZARR:"/vsicurl/https://example.com/data.zarr"`
- ⚠️ **Important**: Always use quotes around paths containing special characters

### Rasterio Compatibility

- ✅ **EOPFZARR driver**: Full rasterio support with correct path format
- ✅ **Rioxarray**: Full support using `EOPFZARR:"path":subdataset` format
- ⚠️ **Path format**: Use `EOPFZARR:"/vsicurl/url":subdataset` for subdatasets (colon-separated)

### Performance Tips

- Use `gdal.SetConfigOption("ZARR_ALLOW_BIG_TILE_SIZE", "YES")` for large tiles
- Enable caching for repeated access to remote data
- Use subdataset URLs for direct array access

## 📖 Recommended Learning Path

### For Beginners
1. **`01-Basic-Functionality-Demo.ipynb`** - Start here to understand the basics
2. **`02-Remote-Data-Access-Demo.ipynb`** - Learn remote data access patterns
3. **`04-Explore_sentinel2_EOPFZARR.ipynb`** - Real-world Sentinel-2 examples
4. **`06-Data-Visualization.ipynb`** - Visualization techniques

### For Python Library Users
1. **`09-EOPFZARR-with-Rasterio.ipynb`** - Rasterio integration (fast, lightweight)
2. **`08-EOPFZARR-with-Rioxarray.ipynb`** - Rioxarray integration (xarray ecosystem)
3. **`07-Sentinel-3-OLCI-Level-1-EFR.ipynb`** - Advanced Sentinel-3 analysis
4. **`11-Sentinel-1-GRD-Demo.ipynb`** - SAR data access and GCP handling

### For Developers
1. **`03-EOPF-Zarr-Test.ipynb`** - Environment setup and validation
2. **`05-GDAL-ZARR-Comparison.ipynb`** - Compare with standard GDAL Zarr driver
3. All other notebooks - Reference implementations

## Common Use Cases

### Scientific Data Analysis

1. Start with `01-Basic-Functionality-Demo.ipynb`
2. Use `05-GDAL-ZARR-Comparison.ipynb` for standard GDAL comparison
3. Explore `04-Explore_sentinel2_EOPFZARR.ipynb` or `07-Sentinel-3-OLCI-Level-1-EFR.ipynb` for real-world examples
4. Apply `06-Data-Visualization.ipynb` for visualization
5. Use `08-EOPFZARR-with-Rioxarray.ipynb` for xarray-based workflows

### Production Integration

1. Review `02-Remote-Data-Access-Demo.ipynb` for remote data patterns
2. Check `09-EOPFZARR-with-Rasterio.ipynb` for efficient integration
3. Use `03-EOPF-Zarr-Test.ipynb` for deployment validation

### Development and Testing

1. Use `03-EOPF-Zarr-Test.ipynb` for environment setup
2. Apply `05-GDAL-ZARR-Comparison.ipynb` for baseline testing
3. Reference all notebooks for comprehensive examples

## 📊 Compatibility Matrix

| Operation | GDAL + EOPFZARR | Rasterio + EOPFZARR | Rioxarray + EOPFZARR |
|-----------|------------------|---------------------|----------------------|
| Local data | ✅ Full support | ✅ Full support | ✅ Full support |
| Remote data | ✅ Full support | ✅ Full support | ✅ Full support |
| Subdatasets | ✅ Full support | ✅ Full support | ✅ Full support (use proper format) |
| CRS/Transform | ✅ Full support | ✅ Full support | ✅ Full support |
| Lazy loading | N/A | ✅ Full support | ✅ Full support (with Dask) |

## 🐛 Troubleshooting

### Common Issues

#### "EOPFZARR driver not found"

- Ensure driver is compiled and installed
- Check `GDAL_DRIVER_PATH` environment variable
- Verify with `gdal.GetDriverByName('EOPFZARR')`

#### Subdataset path format errors

- Use colon separator: `EOPFZARR:"base/path":subdataset`
- NOT slash: `EOPFZARR:"base/path/subdataset"` ❌
- See notebooks 08 and 09 for correct examples

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
