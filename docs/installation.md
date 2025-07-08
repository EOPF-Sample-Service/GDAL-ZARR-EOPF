# Installation Guide

> 🚀 **Quick Start**: For most users, we recommend using our [pre-built releases](#method-1-pre-built-releases-recommended) which provide tested, compatible binaries for all platforms.

## System Requirements

### Operating Systems
- **Linux**: Ubuntu 20.04+, CentOS 8+, RHEL 8+
- **macOS**: 10.15+ (Intel and Apple Silicon)
- **Windows**: 10/11 (64-bit)

### GDAL Version Compatibility

**⚠️ Important**: Plugin compatibility depends on GDAL version:

| GDAL Version | Plugin Support | Notes |
|--------------|----------------|-------|
| 3.11.x | ✅ **Recommended** | Fully tested and supported |
| 3.10.x | ✅ **Supported** | Compatible, use matching build |
| 3.9.x and older | ❌ Not supported | Please upgrade GDAL |

**Check your GDAL version**:
```bash
gdalinfo --version
```

### Build Dependencies (Source Build Only)
- CMake 3.16 or higher
- C++ compiler with C++14 support
- Python 3.8+ (for testing)

### Required Libraries (Source Build Only)
- libgdal-dev (matching your GDAL version)
- libjson-c-dev (for JSON metadata parsing)
- libcurl4-openssl-dev (for cloud access)
- zlib1g-dev (for compression)

## Installation Methods

### Method 1: Pre-built Releases (Recommended)

**🎯 Best for**: Most users who want a quick, tested installation.

#### Step 1: Download the Latest Release

1. Visit our [GitHub Releases](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/releases)
2. Download the archive for your platform:
   - `gdal-eopf-plugin-windows-3.11.zip` (Windows)
   - `gdal-eopf-plugin-linux-3.11.tar.gz` (Linux)
   - `gdal-eopf-plugin-macos-3.11.tar.gz` (macOS)

> 💡 **Note**: Choose the version matching your GDAL version (check with `gdalinfo --version`)

#### Step 2: Extract and Install

**Windows**:
```batch
# Extract the zip file
# Run as Administrator:
install-windows.bat
```

**Linux**:
```bash
tar -xzf gdal-eopf-plugin-linux-3.11.tar.gz
cd gdal-eopf-plugin-linux-3.11
./install-linux.sh
```

**macOS**:
```bash
tar -xzf gdal-eopf-plugin-macos-3.11.tar.gz
cd gdal-eopf-plugin-macos-3.11
./install-macos.sh
```

#### Step 3: Verify Installation

```bash
# Check if driver is loaded
gdalinfo --formats | grep -i eopf

# Expected output:
# EOPFZarr (rw): Earth Observation Processing Framework Zarr
```

### Method 2: Building from Source

**🛠️ Best for**: Developers, custom builds, or unsupported GDAL versions.

#### Prerequisites Check

Before building, ensure compatibility:

```bash
# Check GDAL version
gdal-config --version

# Check GDAL installation path
gdal-config --prefix

# Verify development headers are installed
ls $(gdal-config --prefix)/include/gdal.h
```

#### Step 1: Install Build Dependencies

**Ubuntu/Debian**:
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

**macOS (with Homebrew)**:
```bash
brew install gdal cmake json-c curl zlib python3
```

**Windows (OSGeo4W)**:
```batch
# Install OSGeo4W with development packages
# Download from: https://trac.osgeo.org/osgeo4w/
# Select: gdal-devel, cmake, ninja
```

**CentOS/RHEL**:
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
```

#### Step 3: Install Plugin

**System-wide installation**:
```bash
sudo make install
```

**User-specific installation**:
```bash
# Copy plugin to GDAL plugins directory
GDAL_DRIVER_PATH=$(gdal-config --prefix)/lib/gdalplugins
mkdir -p $GDAL_DRIVER_PATH
cp gdal_EOPFZarr.so $GDAL_DRIVER_PATH/
```

### Method 3: Using Conda (Future)

> 🔮 **Coming Soon**: We're working on publishing to conda-forge for even easier installation.

```bash
# This will be available once published to conda-forge
conda install -c conda-forge gdal-eopf-zarr
```

### Method 4: Using Docker

**🐳 Best for**: Containerized applications, testing, or complex deployment scenarios.

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

### Quick Test

```bash
# Check if driver is loaded
gdalinfo --formats | grep -i eopf

# Expected output:
# EOPFZarr (rw): Earth Observation Processing Framework Zarr
```

### Comprehensive Test

```bash
# Test with sample data (if available)
gdalinfo /path/to/sample.zarr

# Test with EOPF-specific options
gdalinfo -oo EOPF_PROCESS=YES /path/to/eopf-data.zarr

# Test driver prefix
gdalinfo EOPFZARR:/path/to/data.zarr
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

#### ❌ Error 126: "The specified module could not be found"

**Cause**: GDAL version mismatch between plugin and system GDAL.

**Solution**:
1. Check your GDAL version: `gdalinfo --version`
2. Download the matching plugin version (e.g., GDAL 3.11.x plugin for GDAL 3.11.x)
3. Or update your GDAL to match the plugin version

#### ❌ "Driver not found" error

**Causes & Solutions**:

- **Plugin not in driver path**:
  ```bash
  export GDAL_DRIVER_PATH="/path/to/plugin:$GDAL_DRIVER_PATH"
  ```

- **Wrong plugin file extension**:
  - Linux: `gdal_EOPFZarr.so`
  - macOS: `gdal_EOPFZarr.dylib`
  - Windows: `gdal_EOPFZarr.dll`

- **Permission issues**:
  ```bash
  chmod +x /path/to/gdal_EOPFZarr.so
  ```

#### ❌ Build errors

**Common solutions**:

- **CMake too old**: Update to CMake 3.16+
- **Missing GDAL development headers**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libgdal-dev
  
  # macOS
  brew install gdal
  
  # Windows: Install OSGeo4W with gdal-devel
  ```

- **C++ compiler issues**: Ensure C++14 support
- **Library conflicts**: Check for multiple GDAL installations

#### ❌ Runtime errors

**Diagnostic steps**:

1. **Verify GDAL version**:
   ```bash
   gdal-config --version
   gdalinfo --version
   ```

2. **Check plugin loading**:
   ```bash
   export CPL_DEBUG=ON
   gdalinfo --formats | grep -i eopf
   ```

3. **Test with verbose output**:
   ```bash
   export CPL_DEBUG=ON
   gdalinfo your-data.zarr
   ```

### Platform-Specific Issues

#### Windows

- **Visual Studio Runtime**: Ensure Visual C++ Redistributable 2019+ is installed
- **Path issues**: Use full paths instead of relative paths
- **OSGeo4W conflicts**: Ensure only one GDAL installation is active

#### macOS

- **Code signing**: On Apple Silicon, you may need to allow unsigned binaries
- **Homebrew vs conda**: Avoid mixing package managers for GDAL
- **PATH conflicts**: Check which GDAL is being used: `which gdalinfo`

#### Linux

- **System vs user GDAL**: Check which GDAL installation is active
- **Library path issues**: May need to update `LD_LIBRARY_PATH`
- **SELinux**: May block plugin loading on RHEL/CentOS systems

### Getting Help

Having trouble? We're here to help!

1. **Check existing documentation**:
   - [FAQ](faq.md)
   - [Troubleshooting Guide](troubleshooting.md)
   - [User Guide](user-guide.md)

2. **Search existing issues**: [GitHub Issues](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues)

3. **Create a new issue**: Use our templates for faster resolution:
   - [🐛 Bug Report](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues/new?template=bug-report.md)
   - [🧪 Test User Report](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues/new?template=test-user-report.md)

**When reporting issues, please include**:
- Operating system and version
- GDAL version (`gdalinfo --version`)
- Installation method used
- Complete error messages
- Sample data (if possible)

## Uninstallation

### Pre-built Installation

**Windows**:
```batch
# Remove from OSGeo4W plugins directory
del "C:\OSGeo4W\apps\gdal\lib\gdalplugins\gdal_EOPFZarr.dll"
```

**Linux/macOS**:
```bash
# Find and remove plugin file
find /usr -name "gdal_EOPFZarr.*" -type f -delete 2>/dev/null
# Or manually remove from known locations:
sudo rm /usr/local/lib/gdalplugins/gdal_EOPFZarr.*
```

### Source Build

```bash
# If installed with make install
sudo make uninstall  # from build directory

# Manual removal
sudo rm /usr/local/lib/gdalplugins/gdal_EOPFZarr.so
```
