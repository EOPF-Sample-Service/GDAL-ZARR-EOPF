# GDAL EOPF-Zarr Plugin Documentation

Welcome to the comprehensive documentation for the GDAL EOPF-Zarr plugin! This plugin enables seamless integration of Earth Observation Processing Framework (EOPF) datasets with GDAL-based workflows.

## 🚀 Quick Start

**New to the plugin?** Start here:

1. **[Getting Started Guide](../GETTING_STARTED.md)** - Installation and first steps
2. **[Usage Examples](../USAGE_EXAMPLES.md)** - Command-line and Python examples  
3. **[Installation Guide](installation.md)** - Detailed installation instructions

## 📚 Documentation Index

### User Documentation

| Document | Description | Best For |
|----------|-------------|----------|
| **[Getting Started](../GETTING_STARTED.md)** | Quick setup and basic usage | New users |
| **[User Guide](user-guide.md)** | Comprehensive usage examples | All users |
| **[Installation Guide](installation.md)** | Detailed installation instructions | System administrators |
| **[Usage Examples](../USAGE_EXAMPLES.md)** | Command-line and Python recipes | Developers |
| **[FAQ](faq.md)** | Frequently asked questions | Troubleshooting |
| **[Troubleshooting](troubleshooting.md)** | Common issues and solutions | Problem-solving |

### Technical Documentation

| Document | Description | Best For |
|----------|-------------|----------|
| **[API Documentation](api.md)** | Developer API reference | Plugin developers |
| **[Architecture](architecture.md)** | Technical architecture overview | Contributors |
| **[Development Guide](development.md)** | Building and contributing | Developers |
| **[Benchmarks](benchmarks.md)** | Performance metrics | Performance analysis |

## 🎯 Use Case Guides

### By Application

- **QGIS Users**: See [User Guide § QGIS Integration](user-guide.md#qgis-integration)
- **Python Developers**: See [Usage Examples § Python Usage](../USAGE_EXAMPLES.md#python-usage)
- **Command Line**: See [Usage Examples § Command Line](../USAGE_EXAMPLES.md#command-line-examples)
- **Remote Data**: See [Usage Examples § Remote Data Access](../USAGE_EXAMPLES.md#remote-data-access)

### By Data Type

- **Sentinel Data**: See [User Guide § Sentinel Examples](user-guide.md#sentinel-data-examples)
- **STAC Catalogs**: See [User Guide § STAC Integration](user-guide.md#stac-integration)
- **Cloud Data**: See [User Guide § Cloud Access](user-guide.md#cloud-data-access)

## 🔧 Getting the Plugin

Since official releases are pending stakeholder approval, here are your current options:

### Option 1: Build from Source (Recommended)
```bash
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Option 2: GitHub Actions Artifacts
1. Go to [GitHub Actions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions)
2. Download the latest successful build for your platform
3. Extract and use installation scripts

### Option 3: Request Access
Contact maintainers via [GitHub Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues) for testing builds.

## ⚡ Quick Verification

After installation, verify the plugin works:

```bash
# Check plugin is loaded
gdalinfo --formats | grep EOPFZARR

# Test with a dataset
gdalinfo EOPFZARR:/path/to/your/dataset.zarr
```

## 🎓 Learning Path

**Recommended learning progression:**

1. **Start**: [Getting Started Guide](../GETTING_STARTED.md)
2. **Practice**: [Usage Examples](../USAGE_EXAMPLES.md)
3. **Deep Dive**: [User Guide](user-guide.md)
4. **Troubleshoot**: [FAQ](faq.md) & [Troubleshooting](troubleshooting.md)
5. **Contribute**: [Development Guide](development.md)

## 🆘 Getting Help

- **Issues & Bugs**: [GitHub Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)
- **Questions**: [GitHub Discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)
- **Community**: [Contributing Guide](../CONTRIBUTING.md)

## 📋 System Requirements

- **GDAL**: 3.10+ (3.11+ recommended)
- **Platforms**: Windows, macOS, Linux
- **Languages**: C++17, Python 3.8+ (optional)

## 🎯 What Makes This Plugin Special

✅ **Zero Configuration** - Works immediately with QGIS and all GDAL tools  
✅ **Smart Geospatial** - Automatic CRS detection and geotransform calculation  
✅ **Production Ready** - Thread-safe, memory-efficient, cross-platform  
✅ **Python Friendly** - NumPy integration and standard GDAL API  
✅ **Cloud Native** - HTTP/HTTPS access and STAC metadata support

---

**Ready to start?** Head to the [Getting Started Guide](../GETTING_STARTED.md) or jump straight to [Usage Examples](../USAGE_EXAMPLES.md)!
- **[Zarr Specification](https://zarr.readthedocs.io/)** - Zarr format specification
- **[EOPF Documentation](https://eopf-cpm.eumetsat.int/)** - Earth Observation Processing Framework

### Community
- **[GitHub Repository](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF)** - Source code and issues
- **[GitHub Discussions](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/discussions)** - Community discussions

## Documentation Structure

```
docs/
├── README.md           # This file - documentation overview
├── installation.md     # Installation and setup guide
├── user-guide.md       # User guide with examples
├── api.md             # API reference documentation
├── architecture.md     # Technical architecture
├── faq.md             # Frequently asked questions
└── development.md      # Development guide
```

## Contributing to Documentation

We welcome improvements to our documentation! Please see the [Contributing Guide](../CONTRIBUTING.md) for details on how to contribute.

### Documentation Standards
- Use clear, concise language
- Include code examples where helpful
- Keep information up to date
- Follow Markdown best practices
- Test all code examples

## Getting Help

If you can't find what you're looking for in the documentation:

1. Check the [FAQ](faq.md) for common questions
2. Search [GitHub Issues](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/issues)
3. Create a new issue if your question isn't answered
4. Join the discussion in [GitHub Discussions](https://github.com/Yuvraj198920/GDAL-ZARR-EOPF/discussions)