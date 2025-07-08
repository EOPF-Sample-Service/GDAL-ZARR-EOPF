# EOPF Zarr GDAL Plugin - Getting Started Guide

Welcome! Thank you for your interest in the EOPF Zarr GDAL plugin. This guide will help you get the plugin installed and running on your system.

## What is this plugin?

The EOPF Zarr GDAL plugin enables you to open and process **Earth Observation Processing Framework (EOPF) Zarr datasets** using standard GDAL tools and applications like QGIS, Python scripts, and command-line utilities. It provides seamless integration with existing GIS workflows.

## 🚀 Quick Overview

- **Zero configuration** - Works immediately with QGIS and all GDAL-based tools
- **Smart geospatial detection** - Automatically finds CRS and geotransform data
- **Production ready** - Thread-safe, memory-efficient, cross-platform
- **Python friendly** - Direct NumPy integration and standard GDAL API

## Prerequisites

⚠️ **Important**: This plugin requires **GDAL 3.10.x or newer**.

### Check Your GDAL Version

```bash
gdalinfo --version
```

If you have an older version, please update GDAL before installing the plugin:

- **Windows**: Update [OSGeo4W](https://trac.osgeo.org/osgeo4w/) to the latest version
- **macOS**: Update via Homebrew (`brew upgrade gdal`) or install latest from OSGeo
- **Linux**: Update via your package manager or install from OSGeo repositories

## 📦 Getting the Plugin

Since official releases are pending stakeholder approval, here are current options for getting the plugin:

### Option 1: Build from Source (Recommended)

This is currently the most reliable way to get the latest version:

```bash
# Clone the repository
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

# Build the plugin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Option 2: GitHub Actions Artifacts

If you have access to the repository, you can download pre-built binaries from GitHub Actions:

1. Go to [Actions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions)
2. Click on a recent successful build
3. Download the artifact for your platform
4. Extract and follow installation instructions below

### Option 3: Request Access

Contact the maintainers via [GitHub Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues) if you need access to testing builds.

## 💻 Installation

Once you have the plugin files (either built from source or downloaded), use our installation scripts:

### Automatic Installation

**Windows:**
```cmd
install-windows.bat
```

**macOS:**
```bash
./install-macos.sh          # Auto-detect architecture
./install-macos.sh universal # Universal binary
```

**Linux:**
```bash
./install-linux.sh         # Release version
./install-linux.sh debug   # Debug version for troubleshooting
```

### Manual Installation

If the scripts don't work, you can install manually:

1. **Find your GDAL plugin directory:**
   - **Windows**: `C:\OSGeo4W\apps\gdal\lib\gdalplugins\` (OSGeo4W) or `C:\Program Files\GDAL\gdalplugins\`
   - **macOS**: `/usr/local/lib/gdalplugins/` (Homebrew) or your Conda path
   - **Linux**: `/usr/lib/gdalplugins/` or `/usr/local/lib/gdalplugins/`

2. **Copy the plugin file:**
   - Copy `gdal_EOPFZarr.dll` (Windows) or `gdal_EOPFZarr.so` (Linux/macOS) to the plugin directory

3. **Or set environment variable:**
   ```bash
   export GDAL_DRIVER_PATH="/path/to/plugin/directory:$GDAL_DRIVER_PATH"
   ```

## ✅ Verify Installation

Test that the plugin is working:

```bash
gdalinfo --formats | grep EOPFZARR
```

You should see:
```text
EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

## 🧪 Testing the Plugin

### Basic Functionality Test

1. **Test plugin registration:**
   ```bash
   gdalinfo --formats | grep EOPFZARR
   ```

2. **Test with sample data** (if you have Zarr datasets):
   ```bash
   gdalinfo EOPFZARR:/path/to/your-dataset.zarr
   ```

3. **Test with Python:**
   ```python
   from osgeo import gdal
   ds = gdal.Open('EOPFZARR:/path/to/data.zarr')
   print(f"Size: {ds.RasterXSize}x{ds.RasterYSize}")
   ```

### Testing in QGIS

1. Open QGIS
2. Go to **Layer** → **Add Layer** → **Add Raster Layer**
3. Try to open a `.zarr` file or directory
4. The plugin should automatically detect and open EOPF Zarr datasets

### Testing with Python

```python
from osgeo import gdal

# Open a Zarr dataset
dataset = gdal.OpenEx("path/to/your/dataset.zarr", gdal.OF_MULTIDIM_RASTER)

if dataset:
    print("✓ Dataset opened successfully")
    print(f"Driver: {dataset.GetDriver().GetDescription()}")
    
    # Get root group for EOPF datasets
    root_group = dataset.GetRootGroup()
    if root_group:
        print(f"Root group: {root_group.GetName()}")
        
        # List attributes
        for attribute in root_group.GetAttributes():
            print(f"Attribute: {attribute.GetName()} = {attribute.ReadAsString()}")
else:
    print("✗ Failed to open dataset")
```

## Expected Behavior

### ✅ What Should Work
- Plugin loads without errors (or with harmless warnings on Windows)
- Plugin appears in `gdalinfo --formats`
- Can open Zarr datasets with EOPF structure
- Can read metadata and attributes from EOPF datasets
- Works in QGIS and other GDAL-based applications

### ⚠️ Known Issues

#### Windows
- You might see "Error 126: Can't load requested DLL" - **this is harmless**
- The plugin still works correctly despite this error message
- This is a known issue with the GitHub Actions build environment

#### macOS
- Make sure you're using the correct architecture binary:
  - **Intel Macs**: Use x86_64 or universal binary
  - **Apple Silicon (M1/M2/M3)**: Use arm64 or universal binary
- If you get "architecture mismatch" errors, try the universal binary:
  ```bash
  ./install-macos.sh universal
  ```

#### Linux
- Make sure your GDAL version is 3.8 or higher
- You might need to install GDAL development packages

## Troubleshooting

### Plugin Not Found
If `gdalinfo --formats | grep EOPFZARR` returns nothing:

1. **Check GDAL_DRIVER_PATH**:
   ```bash
   echo $GDAL_DRIVER_PATH
   ```

2. **Set GDAL_DRIVER_PATH manually**:
   ```bash
   # Windows
   set GDAL_DRIVER_PATH=C:\path\to\gdal\plugins

   # macOS/Linux
   export GDAL_DRIVER_PATH=/path/to/gdal/plugins
   ```

3. **Find your GDAL plugins directory**:
   ```bash
   # Common locations:
   # Windows: C:\OSGeo4W\bin\gdal\plugins
   # macOS: /opt/homebrew/lib/gdal/plugins (Homebrew)
   # Linux: /usr/lib/gdal/plugins
   ```

### Dataset Won't Open
If `gdalinfo your-dataset.zarr` fails:

1. **Try with explicit driver**:
   ```bash
   gdalinfo -oo EOPF_PROCESS=YES your-dataset.zarr
   ```

2. **Try with driver prefix**:
   ```bash
   gdalinfo EOPFZARR:your-dataset.zarr
   ```

3. **Check dataset structure** - make sure it's a valid EOPF Zarr dataset

### Architecture Issues (macOS)
If you get architecture mismatch errors:

```bash
# Check your Mac's architecture
uname -m

# Use the appropriate binary or universal binary
./install-macos.sh universal
```

## Test Data

If you need test data to verify the plugin works:

1. **Create a simple test**:
   ```bash
   # This should show the plugin is available
   gdalinfo --formats | grep -i zarr
   ```

2. **Request test datasets** from the development team

3. **Check for sample data** in the plugin repository

## Reporting Issues

When reporting issues, please include:

### System Information
```bash
# GDAL version
gdalinfo --version

# Operating system
uname -a  # Linux/macOS
systeminfo  # Windows

# Plugin status
gdalinfo --formats | grep EOPFZARR
```

### Error Information
- Full error messages
- Commands you ran
- Whether the plugin appears in `gdalinfo --formats`
- Any warning or error messages during installation

### Test Results
- What works
- What doesn't work
- Any unexpected behavior

## Getting Help

- **GitHub Issues**: [Report bugs and ask questions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)
- **Discussions**: [Community discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)

## Success Criteria

The plugin is working correctly if:

1. ✅ Shows up in `gdalinfo --formats`
2. ✅ Can open valid EOPF Zarr datasets
3. ✅ Extracts metadata and attributes correctly
4. ✅ Works in your preferred GDAL-based applications

Even if you see harmless error messages during loading (especially on Windows), the plugin can still be considered working if it passes the above criteria.

Thank you for testing! Your feedback helps make this plugin better for everyone.

---

**Version**: Test Release  
**Date**: July 2025  
**Platform Support**: Windows, macOS (Intel + Apple Silicon), Linux
