# Windows Installation Guide

## Quick Installation

### Option 1: Using Pre-built Binaries (Recommended)

1. Download the latest Windows release from [GitHub Releases](https://github.com/your-repo/GDAL-ZARR-EOPF/releases)
2. Extract the archive
3. Run `install-windows.bat` as Administrator

### Option 2: Manual Installation

1. **Download the Plugin DLL**
   - Download `gdal_EOPFZarr.dll` from the latest release
   
2. **Find Your GDAL Plugins Directory**

   ```cmd
   # For OSGeo4W installations:
   C:\OSGeo4W\bin\gdal\plugins\
   
   # For GDAL installed elsewhere, check:
   gdalinfo --formats
   # Look for other plugins to determine the path
   ```

3. **Copy the Plugin**

   ```cmd
   copy gdal_EOPFZarr.dll "C:\OSGeo4W\bin\gdal\plugins\"
   ```

4. **Set Environment Variable**

   ```cmd
   set GDAL_DRIVER_PATH=C:\OSGeo4W\bin\gdal\plugins;%GDAL_DRIVER_PATH%
   ```

## Verification

```cmd
gdalinfo --formats | findstr EOPF
```

You should see:

```text
EOPFZARR -raster- (rw): EOPF Zarr format
```

## Compatibility Matrix

| OSGeo4W Version | GDAL Version | Plugin Compatibility |
|----------------|--------------|---------------------|
| OSGeo4W v2     | 3.10.x       | ✅ Recommended      |
| OSGeo4W v2     | 3.11.x       | ⚠️ May work         |
| Conda GDAL     | 3.10.x       | ✅ Should work      |
| Custom GDAL    | 3.9.x        | ❌ Not supported    |

## Troubleshooting

### "DLL not found" or "Entry point not found"

This usually indicates a version mismatch between the plugin and your GDAL installation.

1. **Check your GDAL version:**

   ```cmd
   gdalinfo --version
   ```

2. **Check DLL dependencies:**

   ```cmd
   dumpbin /DEPENDENTS gdal_EOPFZarr.dll
   ```

3. **Solutions:**
   - Use the plugin build that matches your GDAL version
   - Update to OSGeo4W with GDAL 3.10.x
   - Build the plugin yourself (see [Development Guide](../docs/development.md))

### "The specified module could not be found"

1. **Check all dependencies are available:**

   ```cmd
   # Make sure these DLLs exist in your PATH:
   where gdal310.dll
   where MSVCP140.dll
   where VCRUNTIME140.dll
   ```

2. **Install Visual C++ Redistributables:**
   - Download from [Microsoft](https://aka.ms/vs/17/release/vc_redist.x64.exe)

### "Driver not loading"

1. **Verify plugin path:**

   ```cmd
   echo %GDAL_DRIVER_PATH%
   dir "%GDAL_DRIVER_PATH%"
   ```

2. **Check plugin permissions:**

   ```cmd
   icacls gdal_EOPFZarr.dll
   ```

## Building from Source

See [Development Guide](../docs/development.md) for detailed build instructions.

### Quick Build (Advanced Users)

```cmd
# Install OSGeo4W with development tools
# Install Visual Studio 2022 or Build Tools

git clone https://github.com/your-repo/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

cmake -S . -B build ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_PREFIX_PATH="C:/OSGeo4W;C:/OSGeo4W/apps/gdal" ^
  -DGDAL_DIR="C:/OSGeo4W/apps/gdal/lib/cmake/gdal"

cmake --build build --config Release
```

## Support

- **Issues:** [GitHub Issues](https://github.com/your-repo/GDAL-ZARR-EOPF/issues)
- **Documentation:** [Main README](../README.md)
- **Development:** [Contributing Guide](../CONTRIBUTING.md)
