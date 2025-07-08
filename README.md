# GDAL EOPF-Zarr Plugin 🌍

[![Build Status](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/main.yml/badge.svg)](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/main.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GDAL Version](https://img.shields.io/badge/GDAL-3.10%2B-blue.svg)](https://gdal.org)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF)

**Seamless Earth Observation Data Access** 🚀  
The GDAL EOPF-Zarr plugin brings **Earth Observation Processing Framework (EOPF)** datasets directly into your existing GIS workflows. Open Zarr-based Earth observation data in [QGIS](https://qgis.org), process it with standard GDAL tools, or analyze it with Python—all with **zero configuration** and **full geospatial intelligence**.

> 💡 **Why this matters:** Transform complex Earth observation data formats into standard GDAL datasets that work everywhere—from desktop GIS to cloud analytics pipelines.

---

## ✨ What Makes This Special

🎯 **Instant QGIS Integration** - Just click "Add Raster Layer" and it works  
🔍 **Smart Geospatial Detection** - Automatic CRS and geotransform from metadata  
🚀 **Production Ready** - Cross-platform, thread-safe, memory-efficient  
🌐 **Cloud Native** - HTTP/HTTPS access, STAC metadata support  
🐍 **Python Friendly** - NumPy integration, standard GDAL bindings  

---

## 🚀 Key Capabilities

### **Data Access & Integration**

- ✅ **Multiple access patterns**: `EOPFZARR:/path/to/data.zarr`, open options, auto-detection
- ✅ **Subdataset support** for hierarchical data structures
- ✅ **Virtual file system** support (`/vsicurl/`, network paths)
- ✅ **Chunked data optimization** for large datasets

### **Geospatial Intelligence**

- ✅ **Automatic CRS detection** from EPSG codes and STAC metadata
- ✅ **Smart geotransform calculation** from corner coordinates
- ✅ **Multi-coordinate system support** (UTM, Geographic)
- ✅ **Sentinel-2 tile naming** convention inference

### **Application Compatibility**

- ✅ **QGIS seamless integration** - zero configuration required
- ✅ **Python API support** with NumPy integration
- ✅ **Command-line tools** (`gdalinfo`, `gdal_translate`, `gdalwarp`)
- ✅ **Cross-platform support** (Windows, macOS, Linux)

### **Performance & Reliability**

- ✅ **Thread-safe operations** for concurrent access
- ✅ **Memory-efficient** block-based I/O
- ✅ **Robust error handling** with detailed diagnostics
- ✅ **Production deployment** ready

---

## 📊 Current Status & Roadmap

| Feature | Status | GDAL Version | Notes |
|---------|--------|--------------|-------|
| **EOPF Zarr Reading** | ✅ **Complete** | 3.10+ | Production ready |
| **Multi-band Support** | ✅ **Complete** | 3.10+ | Individual band access |
| **Geospatial Metadata** | ✅ **Complete** | 3.10+ | CRS, geotransform, STAC |
| **QGIS Integration** | ✅ **Complete** | 3.10+ | Zero-config compatibility |
| **Python API** | ✅ **Complete** | 3.10+ | NumPy integration |
| **Cross-platform** | ✅ **Complete** | 3.10+ | Windows, macOS, Linux |
| **Write Support** | 🔄 **Planned** | Future | Read-only currently |
| **S3 Integration** | 🔄 **Planned** | Future | Via `/vsis3/` |

## 🎯 Quick Examples

### Command Line Power

```bash
# Inspect any EOPF dataset
gdalinfo EOPFZARR:/path/to/sentinel2.zarr

# Convert to standard formats  
gdal_translate EOPFZARR:/data.zarr output.tif

# Reproject and resample
gdalwarp -t_srs EPSG:4326 -tr 0.001 0.001 EOPFZARR:/data.zarr reprojected.tif
```

### Python Data Science

```python
from osgeo import gdal
import numpy as np

# Open and process EOPF data
ds = gdal.Open('EOPFZARR:/path/to/data.zarr')
data = ds.ReadAsArray()  # Direct NumPy integration

# Works with any GDAL-compatible library
import rasterio
with rasterio.open('EOPFZARR:/data.zarr') as src:
    array = src.read()
```

### QGIS Integration

1. **Open QGIS** → **Layer** → **Add Raster Layer**
2. **Browse to** your `.zarr` file or directory  
3. **Click Open** - that's it! No plugins, no configuration needed.

---

## 📖 Table of Contents

1. [🚀 Quick Start](#-quick-start)
2. [💻 Installation](#-installation)  
3. [📋 Requirements](#-requirements)
4. [🔧 Building from Source](#-building-from-source)
5. [🧪 Testing](#-testing)
6. [🤝 Contributing](#-contributing)
7. [📄 License](#-license)
8. [🙋 Support & Community](#-support--community)

---

## 🚀 Quick Start

**Get up and running in 3 steps:**

1. **Get the plugin** - See [Getting Started Guide](GETTING_STARTED.md) for installation options:
   - [Build from source](GETTING_STARTED.md#option-1-build-from-source-recommended) (recommended)
   - [Use CI artifacts](GETTING_STARTED.md#option-2-github-actions-artifacts) from successful builds
   - [Request access](GETTING_STARTED.md#option-3-request-access) to development builds

2. **Install** with our automated scripts:

   ```bash
   # Windows
   install-windows.bat
   
   # macOS  
   ./install-macos.sh
   
   # Linux
   ./install-linux.sh
   ```

3. **Verify** it's working:

   ```bash
   gdalinfo --formats | grep EOPFZARR
   ```

**That's it!** Now open any `.zarr` file in QGIS or use `EOPFZARR:/path/to/data.zarr` in your workflows.

---

## 💻 Installation

**Before installing:** Make sure you have **GDAL 3.10+** installed. Check with `gdalinfo --version`.

### 📦 Getting the Plugin

Since official releases are pending approval, current options are:

1. **[Build from source](GETTING_STARTED.md#option-1-build-from-source-recommended)** (recommended)
2. **[Use CI artifacts](GETTING_STARTED.md#option-2-github-actions-artifacts)** from GitHub Actions
3. **[Request access](GETTING_STARTED.md#option-3-request-access)** to development builds

See our [Getting Started Guide](GETTING_STARTED.md) for detailed instructions on each method.

### ⚡ Installing the Plugin

Once you have the plugin binary, use our installation scripts for automatic setup:

**Windows:**

```cmd
install-windows.bat
```

- Supports OSGeo4W, Program Files, and custom GDAL installations
- Automatic GDAL version detection and plugin directory discovery

**macOS:**

```bash
./install-macos.sh          # Auto-detect architecture
./install-macos.sh universal # Use universal binary
```

- Works with Homebrew, Conda, and system GDAL installations
- Intel and Apple Silicon support

**Linux:**

```bash
./install-linux.sh         # Install built plugin
./install-linux.sh debug   # Install debug version for troubleshooting
```

- Compatible with package manager and custom GDAL installations

### 🔧 Manual Installation

If you prefer manual installation or the automatic scripts don't work for your setup:

**Copy to GDAL Plugin Directory:**

- **Linux**: `/usr/lib/gdalplugins/` or `/usr/local/lib/gdalplugins/`
- **Windows**: `C:\OSGeo4W\apps\gdal\lib\gdalplugins\` or `C:\Program Files\GDAL\gdalplugins\`
- **macOS**: `/usr/local/lib/gdalplugins/` or your Homebrew/Conda path

**Or Set Environment Variable:**

```bash
export GDAL_DRIVER_PATH="/path/to/plugin/directory:$GDAL_DRIVER_PATH"
```

---

## 📋 Requirements

- **GDAL 3.10+** (3.11+ recommended) - [Download](https://gdal.org/download.html)
- **Compatible OS**: Windows, macOS, Linux
- **For building**: C++17 compiler, CMake 3.10+

**Quick GDAL version check:**

```bash
gdalinfo --version
```

**Installation guides:**

- **Windows**: Use [OSGeo4W](https://trac.osgeo.org/osgeo4w/) for latest GDAL
- **macOS**: `brew install gdal` for Homebrew users
- **Linux**: Use your package manager or [UbuntuGIS](https://launchpad.net/~ubuntugis/+archive/ubuntu/ppa) PPA

---

## 🔧 Building from Source

**This is currently the primary way to get the plugin.** Pre-built releases are pending stakeholder approval.

### Prerequisites

- **GDAL 3.10+** (check with `gdalinfo --version`)
- **C++17 compiler** (Visual Studio 2019+, GCC 8+, Clang 7+)
- **CMake 3.10+**

### Build Steps

```bash
# Clone repository
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Install (use our automated scripts)
cd ..
./install-windows.bat    # Windows
./install-macos.sh       # macOS  
./install-linux.sh       # Linux
```

**Alternative methods:** See [GETTING_STARTED.md](GETTING_STARTED.md) for GitHub Actions artifacts and other options.

---

## 🧪 Testing

**Verify installation:**

```bash
gdalinfo --formats | grep EOPFZARR
# Should show: EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

**Test with sample data:**

- Check out our [Getting Started Guide](GETTING_STARTED.md) for detailed testing instructions
- See [User Guide](docs/user-guide.md) for comprehensive examples
- Browse [Usage Examples](USAGE_EXAMPLES.md) for quick reference

---

## 🤝 Contributing

We welcome contributions! Here's how to get involved:

- **🐛 Found a bug?** [Open an issue](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)
- **💡 Have an idea?** [Start a discussion](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)
- **🧪 Testing?** Use our [test user template](.github/ISSUE_TEMPLATE/test-user-report.md)
- **🔧 Want to code?** See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup

---

## 📄 License

**MIT License** - See [LICENSE](LICENSE) for details. Compatible with GDAL's open-source licensing.

---

## 🙋 Support & Community

- **📚 Documentation**: [User Guide](docs/user-guide.md) | [API Docs](docs/api.md) | [FAQ](docs/faq.md)
- **🆘 Need help?** [Troubleshooting Guide](docs/troubleshooting.md) | [GitHub Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)
- **💬 Discussions**: [GitHub Discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)
- **📧 Contact**: [Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues) for bug reports and feature requests

**Credits:** Thanks to ESA for EOPF specifications, and the GDAL & QGIS communities for the geospatial foundation.

---

<div align="center">

**⭐ Star this repository if you find it useful!**

[Getting Started Guide](GETTING_STARTED.md) • [View Documentation](docs/) • [Report Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)

</div>
