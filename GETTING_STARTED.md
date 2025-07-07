# EOPF Zarr GDAL Plugin - Getting Started Guide for Test Users

Welcome! Thank you for helping test the EOPF Zarr GDAL plugin. This guide will help you install and test the plugin on your system.

## What is this plugin?

The EOPF Zarr GDAL plugin enables you to open and process Earth Observation Processing Framework (EOPF) Zarr datasets using standard GDAL tools and applications like QGIS, Python scripts, and command-line utilities.

## Quick Start

### 1. Download the Plugin

1. Go to the [Releases page](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/releases)
2. Download the latest release package
3. Extract the files to a folder on your computer

### 2. Install the Plugin

Navigate to the extracted folder and run the appropriate installation script:

#### Windows
```cmd
install-windows.bat
```

#### macOS
```bash
# Auto-detect your Mac's architecture (recommended)
./install-macos.sh

# Or use universal binary (works on both Intel and Apple Silicon)
./install-macos.sh universal
```

#### Linux
```bash
# Install release version (recommended)
./install-linux.sh

# Or install debug version for troubleshooting
./install-linux.sh debug
```

### 3. Verify Installation

Test that the plugin is working:

```bash
gdalinfo --formats | grep EOPFZARR
```

You should see output like:
```
EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

## Testing the Plugin

### Basic Functionality Test

1. **Test plugin registration**:
   ```bash
   gdalinfo --formats | grep EOPFZARR
   ```

2. **Test with sample data** (if you have Zarr datasets):
   ```bash
   gdalinfo your-dataset.zarr
   ```

3. **Test with remote data** (if available):
   ```bash
   gdalinfo "https://your-zarr-url.zarr"
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
