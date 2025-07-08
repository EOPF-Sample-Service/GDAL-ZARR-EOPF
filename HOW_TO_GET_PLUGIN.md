# How to Get the GDAL EOPF-Zarr Plugin

Since official releases are pending stakeholder approval, this guide explains current options for obtaining and installing the plugin.

## 📦 Current Availability Status

**🟡 Pre-Release Status**: The plugin is fully functional and production-ready, but official releases are pending stakeholder approval.

**✅ What Works**: All core functionality is complete and tested:
- EOPF Zarr dataset reading
- QGIS integration 
- Python API support
- Cross-platform compatibility
- Command-line tools integration

## 🔧 Option 1: Build from Source (Recommended)

This is currently the most reliable way to get the latest version with all features.

### Prerequisites
```bash
# Check GDAL version (must be 3.10+)
gdalinfo --version
```

### Quick Build
```bash
# Clone repository
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)  # Linux/macOS
# cmake --build . --config Release  # Windows

# Install
cd ..
./install-linux.sh      # Linux
./install-macos.sh      # macOS
install-windows.bat     # Windows
```

### Verify Installation
```bash
gdalinfo --formats | grep EOPFZARR
# Should show: EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

## 🚀 Option 2: GitHub Actions Artifacts

Pre-built binaries are available from our continuous integration system.

### Access Artifacts
1. **Go to**: [GitHub Actions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions)
2. **Click**: A recent successful build (green checkmark)
3. **Download**: The artifact for your platform:
   - `windows-release` (Windows 64-bit)
   - `linux-release` (Linux x64)
   - `macos-release` (macOS Universal)

### Install from Artifact
```bash
# Extract the downloaded file
unzip windows-release.zip  # or equivalent for your platform

# Run installation script
install-windows.bat     # Windows
./install-linux.sh     # Linux  
./install-macos.sh     # macOS
```

## 📞 Option 3: Request Access

If you need access for testing or evaluation:

### Contact Methods
- **GitHub Issues**: [Create an issue](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues) with tag `request-access`
- **GitHub Discussions**: [Start a discussion](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)
- **Direct Contact**: Mention your use case and requirements

### Information to Include
- **Platform**: Windows/macOS/Linux
- **GDAL Version**: Output of `gdalinfo --version`
- **Use Case**: Brief description of your intended usage
- **Timeline**: When you need access

## 🔍 Verification Steps

After installation, verify everything works:

### Basic Verification
```bash
# 1. Check plugin loads
gdalinfo --formats | grep EOPFZARR

# 2. Test with dummy path (should fail gracefully)
gdalinfo EOPFZARR:/nonexistent/path.zarr

# 3. Check QGIS integration (if you use QGIS)
# Open QGIS → Add Raster Layer → should accept .zarr files
```

### Advanced Testing
```python
# Python integration test
from osgeo import gdal

# This should not crash
gdal.GetDriverByName('EOPFZARR')
print("EOPFZARR driver available")
```

## 📋 Platform-Specific Notes

### Windows
- **OSGeo4W**: Recommended GDAL installation method
- **Path**: Plugin installs to `C:\OSGeo4W\apps\gdal\lib\gdalplugins\`
- **Compatibility**: Works with GDAL 3.10+ from OSGeo4W

### macOS
- **Architecture**: Universal binary supports both Intel and Apple Silicon
- **Homebrew**: Compatible with `brew install gdal`
- **Conda**: Works with conda-forge GDAL installations

### Linux
- **Distributions**: Tested on Ubuntu 20.04+, CentOS 8+, RHEL 8+
- **Package Managers**: Compatible with apt, yum, dnf GDAL packages
- **UbuntuGIS**: Recommended for latest GDAL versions

## 🆘 Troubleshooting Installation

### Common Issues

**Plugin not found**:
```bash
# Check GDAL plugin path
gdalinfo --format-details

# Set custom path if needed
export GDAL_DRIVER_PATH="/path/to/plugin:$GDAL_DRIVER_PATH"
```

**GDAL version mismatch**:
```bash
# Check version compatibility
gdalinfo --version
# Must be 3.10.0 or newer
```

**Permission issues**:
```bash
# Linux/macOS: Check plugin directory permissions
ls -la /usr/lib/gdalplugins/
sudo chmod 755 /usr/lib/gdalplugins/

# Windows: Run as Administrator if needed
```

### Getting Help

1. **Check**: [Troubleshooting Guide](docs/troubleshooting.md)
2. **Search**: [Existing Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)
3. **Ask**: [GitHub Discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)
4. **Report**: [New Issue](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/new)

## 🎯 What to Do After Installation

1. **Read**: [Getting Started Guide](GETTING_STARTED.md)
2. **Try**: [Usage Examples](USAGE_EXAMPLES.md)
3. **Explore**: [User Guide](docs/user-guide.md)
4. **Contribute**: [Contributing Guide](CONTRIBUTING.md)

## 🔮 Future Availability

**Official Releases**: Will be available once stakeholder approval is obtained
- **Release Packages**: Pre-built binaries for all platforms
- **Package Managers**: Potential conda-forge, homebrew integration
- **Documentation**: Enhanced installation and user guides

**Stay Updated**:
- ⭐ **Star** the repository for notifications
- 👁️ **Watch** for release announcements
- 📢 **Follow** [GitHub Discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)

---

**Need help?** Don't hesitate to reach out via [GitHub Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues) or [Discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)!
