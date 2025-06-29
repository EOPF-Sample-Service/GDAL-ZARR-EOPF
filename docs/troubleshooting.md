# Build Troubleshooting Guide

This guide helps resolve common build issues with the EOPF-Zarr GDAL plugin.

## Common Build Errors

### 1. "Could NOT find GDAL"

**Error Message:**
```
CMake Error: Could NOT find GDAL (missing: GDAL_LIBRARY GDAL_INCLUDE_DIR)
```

**Solutions:**

#### Ubuntu/Debian:
```bash
# Install GDAL development packages
sudo apt-get update
sudo apt-get install -y libgdal-dev gdal-bin

# Verify installation
gdal-config --version
gdal-config --prefix
```

#### macOS:
```bash
# Using Homebrew
brew install gdal

# Using MacPorts
sudo port install gdal
```

#### CentOS/RHEL:
```bash
# Enable EPEL repository first
sudo yum install -y epel-release
sudo yum install -y gdal-devel gdal
```

#### Manual GDAL Installation:
If system packages don't work, specify GDAL location:
```bash
cmake .. -DGDAL_ROOT=/usr/local -DGDAL_LIBRARY=/usr/local/lib/libgdal.so
```

### 2. "pkg-config not found"

**Error Message:**
```
Could not find PkgConfig
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install -y pkg-config

# macOS
brew install pkg-config

# CentOS/RHEL
sudo yum install -y pkgconfig
```

### 3. Missing Dependencies

**Error Messages:**
- `fatal error: json-c/json.h: No such file`
- `fatal error: curl/curl.h: No such file`

**Solutions:**
```bash
# Ubuntu/Debian
sudo apt-get install -y libjson-c-dev libcurl4-openssl-dev zlib1g-dev

# macOS
brew install json-c curl

# CentOS/RHEL
sudo yum install -y json-c-devel libcurl-devel zlib-devel
```

### 4. CMake Version Issues

**Error Message:**
```
CMake 3.14 or higher is required
```

**Solutions:**

#### Ubuntu (if default version is too old):
```bash
# Remove old version
sudo apt-get remove --purge cmake

# Install from Kitware repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update
sudo apt-get install -y cmake
```

#### macOS:
```bash
brew install cmake
```

### 5. Compiler Issues

**Error Message:**
```
C++ compiler does not support C++17
```

**Solutions:**
```bash
# Ubuntu/Debian - install newer GCC
sudo apt-get install -y gcc-9 g++-9
export CC=gcc-9
export CXX=g++-9

# macOS - install Xcode command line tools
xcode-select --install
```

### 6. Plugin Loading Issues

**Error Message:**
```
EOPF driver not found in formats list
```

**Solutions:**

#### Check Plugin Path:
```bash
# Set plugin directory
export GDAL_DRIVER_PATH="/path/to/plugin:$GDAL_DRIVER_PATH"

# Verify plugin file exists
ls -la /path/to/plugin/gdal_EOPFZarr.*

# Check plugin can be loaded
file /path/to/plugin/gdal_EOPFZarr.so
```

#### Debug Plugin Loading:
```bash
# Enable debug output
export CPL_DEBUG=ON
gdalinfo --formats 2>&1 | grep -A5 -B5 -i eopf
```

## Build Environment Setup

### Complete Development Environment

#### Ubuntu/Debian:
```bash
#!/bin/bash
# Complete setup script for Ubuntu/Debian

# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# Install GDAL and dependencies
sudo apt-get install -y \
    libgdal-dev \
    gdal-bin \
    libjson-c-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    libproj-dev \
    libsqlite3-dev

# Verify installations
echo "=== Verification ==="
gcc --version
cmake --version
gdal-config --version
pkg-config --version

echo "=== GDAL Configuration ==="
gdal-config --cflags
gdal-config --libs
```

#### macOS:
```bash
#!/bin/bash
# Complete setup script for macOS

# Install Homebrew if not present
if ! command -v brew &> /dev/null; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install dependencies
brew install \
    cmake \
    gdal \
    json-c \
    curl \
    pkg-config

# Verify installations
echo "=== Verification ==="
clang --version
cmake --version
gdal-config --version
```

### Docker Development Environment

For consistent builds across platforms:

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgdal-dev \
    gdal-bin \
    libjson-c-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    pkg-config

# Set working directory
WORKDIR /workspace

# Copy source code
COPY . .

# Build
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Verify build
RUN cd build && \
    export GDAL_DRIVER_PATH="$(pwd):$GDAL_DRIVER_PATH" && \
    gdalinfo --formats | grep -i eopf
```

## Debugging Build Issues

### Enable Verbose Output

```bash
# CMake verbose configuration
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# Make with verbose output
make VERBOSE=1

# Show detailed compiler commands
make VERBOSE=1 2>&1 | tee build.log
```

### Check CMake Variables

Add to CMakeLists.txt for debugging:
```cmake
# Debug CMake variables
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "GDAL_VERSION: ${GDAL_VERSION}")
message(STATUS "GDAL_INCLUDE_DIRS: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL_LIBRARIES: ${GDAL_LIBRARIES}")
```

### System Information

Collect system information for bug reports:
```bash
#!/bin/bash
echo "=== System Information ==="
lsb_release -a 2>/dev/null || cat /etc/os-release
uname -a

echo "=== Compiler Information ==="
gcc --version
g++ --version

echo "=== CMake Information ==="
cmake --version

echo "=== GDAL Information ==="
gdal-config --version
gdal-config --prefix
gdal-config --formats

echo "=== Environment Variables ==="
echo "CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
echo "GDAL_DRIVER_PATH: $GDAL_DRIVER_PATH"
echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
```

## GitHub Actions Debugging

For CI/CD issues:

### Check Workflow Logs
1. Go to Actions tab in GitHub repository
2. Click on failed workflow run
3. Expand failed job steps
4. Look for error messages in detailed logs

### Common CI Issues

#### Dependency Installation:
```yaml
# Add debugging to workflow
- name: Debug environment
  run: |
    echo "=== System Info ==="
    cat /etc/os-release
    echo "=== Available packages ==="
    apt-cache search gdal
    echo "=== Installed packages ==="
    dpkg -l | grep gdal
```

#### Build Debugging:
```yaml
- name: Debug build
  run: |
    cd build
    echo "=== CMake Cache ==="
    cat CMakeCache.txt | grep -i gdal
    echo "=== Build files ==="
    ls -la
```

## Getting Help

### Information to Include in Bug Reports

1. **System Information:**
   - Operating system and version
   - Compiler version
   - CMake version
   - GDAL version

2. **Build Information:**
   - Complete error message
   - CMake configuration command used
   - Build output (with VERBOSE=1)

3. **Environment:**
   - Relevant environment variables
   - Custom installation paths
   - Virtual environments (if applicable)

### Useful Commands for Diagnostics

```bash
# System diagnostics
uname -a
lsb_release -a

# Compiler diagnostics
gcc --version
g++ --version
which gcc g++

# GDAL diagnostics
gdal-config --version --prefix --cflags --libs
gdalinfo --version --formats

# CMake diagnostics
cmake --version
cmake --help | grep -A10 "The following generators"

# Library diagnostics
ldconfig -p | grep gdal
pkg-config --list-all | grep gdal
```

This troubleshooting guide should help resolve most common build issues. For specific problems not covered here, please create an issue in the GitHub repository with detailed system information and error messages.
