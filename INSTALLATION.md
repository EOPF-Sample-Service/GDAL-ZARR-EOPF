# Installation Guide

## Prerequisites

### GDAL Version
This plugin requires **GDAL 3.10+**. Check your version:
```bash
gdalinfo --version
```

### System Requirements
- **Windows**: Visual Studio 2019+ or MinGW-w64
- **Linux**: GCC 8+ or Clang 7+
- **macOS**: Xcode 11+ or Homebrew GCC
- **CMake**: 3.16 or higher

## Installation Methods

### Option 1: Using Install Scripts (Recommended)

**Windows:**
```cmd
install-windows.bat
```

**Linux:**
```bash
./install-linux.sh
```

**macOS:**
```bash
./install-macos.sh
```

### Option 2: Manual Installation

1. **Build the plugin:**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

2. **Find your GDAL plugins directory:**
   ```bash
   # Linux/macOS
   gdal-config --plugindir
   
   # Windows (common locations)
   # C:\OSGeo4W\apps\gdal\lib\gdalplugins\
   # C:\Program Files\GDAL\gdalplugins\
   ```

3. **Copy plugin to GDAL plugins directory:**
   ```bash
   # Linux/macOS
   sudo cp gdal_EOPFZarr.so $(gdal-config --plugindir)
   
   # Windows
   copy gdal_EOPFZarr.dll "C:\OSGeo4W\apps\gdal\lib\gdalplugins\"
   ```

### Option 3: Environment Variable

Set the plugin path environment variable:
```bash
export GDAL_DRIVER_PATH="/path/to/plugin:$GDAL_DRIVER_PATH"
```

## Verification

Test that the plugin is loaded:
```bash
gdalinfo --formats | grep EOPFZARR
```

You should see:
```
EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

## Troubleshooting

### Plugin Not Found
- Verify GDAL version is 3.10+
- Check plugin file permissions
- Ensure no missing dependencies

### Build Issues
- Verify CMake can find GDAL development headers
- On Windows, use the same compiler as your GDAL installation
- Check that vcpkg/package manager GDAL version matches

### Runtime Issues
- Enable debug output: `export CPL_DEBUG=EOPFZARR`
- Check GDAL error messages: `export CPL_DEBUG=ON`
