# Installation Guide

## System Requirements

### Operating Systems
- Linux (Ubuntu 20.04+, CentOS 8+, etc.)
- macOS (10.15+)
- Windows (10/11 with WSL2 or native)

### Dependencies
- GDAL 3.4.0 or higher
- CMake 3.16 or higher
- C++ compiler with C++14 support
- Python 3.8+ (for testing)

### Required Libraries
- libgdal-dev
- libjson-c-dev (for JSON metadata parsing)
- libcurl4-openssl-dev (for cloud access)
- zlib1g-dev (for compression)

## Installation Methods

### Method 1: Building from Source (Recommended)

#### Step 1: Install System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libgdal-dev \
    gdal-bin \
    libjson-c-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    python3-dev \
    python3-pip
```

**macOS (with Homebrew):**
```bash
brew install gdal cmake json-c curl zlib python3
```

**CentOS/RHEL:**
```bash
sudo yum install -y \
    gcc-c++ \
    cmake \
    gdal-devel \
    json-c-devel \
    libcurl-devel \
    zlib-devel \
    python3-devel
```

#### Step 2: Clone and Build

```bash
# Clone the repository
git clone https://github.com/Yuvraj198920/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# Build the plugin
make -j$(nproc)

# Install (optional)
sudo make install
```

#### Step 3: Install Plugin

**Option A: System-wide installation**
```bash
sudo make install
```

**Option B: User-specific installation**
```bash
# Copy plugin to GDAL plugins directory
GDAL_DRIVER_PATH=$(gdal-config --prefix)/lib/gdalplugins
mkdir -p $GDAL_DRIVER_PATH
cp gdal_EOPFZarr.so $GDAL_DRIVER_PATH/
```

### Method 2: Using Conda (Future)

```bash
# This will be available once published to conda-forge
conda install -c conda-forge gdal-eopf-zarr
```

### Method 3: Using Docker

```dockerfile
FROM osgeo/gdal:ubuntu-small-latest

# Install build dependencies
RUN apt-get update && apt-get install -y \
    git cmake build-essential \
    libjson-c-dev libcurl4-openssl-dev

# Clone and build
WORKDIR /src
RUN git clone https://github.com/Yuvraj198920/GDAL-ZARR-EOPF.git
WORKDIR /src/GDAL-ZARR-EOPF

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

# Verify installation
RUN gdalinfo --formats | grep -i eopf
```

## Verification

### Test Installation
```bash
# Check if driver is loaded
gdalinfo --formats | grep -i eopf

# Expected output:
# EOPF (ro): Earth Observation Processing Framework

# Test with sample data (if available)
gdalinfo sample_data/test.zarr
```

### Environment Variables

You may need to set these environment variables:

```bash
# Add plugin directory to GDAL driver path
export GDAL_DRIVER_PATH="/usr/local/lib/gdalplugins:$GDAL_DRIVER_PATH"

# For debugging
export CPL_DEBUG=ON
export GDAL_DATA=/usr/share/gdal
```

## Troubleshooting

### Common Issues

#### "Driver not found" error
- Ensure the plugin is in GDAL's driver path
- Check `GDAL_DRIVER_PATH` environment variable
- Verify GDAL version compatibility

#### Build errors
- Update CMake to latest version
- Check that all dependencies are installed
- Ensure C++ compiler supports C++14

#### Runtime errors
- Check GDAL version: `gdal-config --version`
- Verify plugin loading: `gdalinfo --formats`
- Enable debug output: `export CPL_DEBUG=ON`

### Getting Help

If you encounter issues:
1. Check the [FAQ](faq.md)
2. Search existing [GitHub Issues](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues)
3. Create a new issue with:
   - Operating system and version
   - GDAL version
   - Build output/error messages
   - Sample data (if possible)

## Uninstallation

```bash
# Remove plugin file
sudo rm /usr/local/lib/gdalplugins/gdal_EOPFZarr.so

# Or if installed system-wide
sudo make uninstall  # from build directory
```
