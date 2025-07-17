# EOPF-Zarr GDAL Driver Documentation

Welcome to the comprehensive documentation for the EOPF-Zarr GDAL Plugin. This driver enables GDAL to read Zarr datasets with EOPF (Earth Observation Processing Framework) metadata.

## 📚 Documentation Index

### Getting Started

- **[Installation Guide](installation.md)** - How to install and configure the plugin
- **[User Guide](user-guide.md)** - Basic usage and examples
- **[Getting Started](../GETTING_STARTED.md)** - Quick start tutorial

### Technical Documentation

- **[Architecture](architecture.md)** - Technical architecture and design
- **[API Reference](api.md)** - Complete API documentation
- **[Development Guide](development.md)** - Contributing and development setup

### Advanced Topics

- **[Zarr Specifications](zarr_v3_spec.md)** - Zarr v3 specification details
- **[GDAL Driver Development](zarr%20driver%20development%20in%20gdal.md)** - GDAL driver development insights
- **[Roadmap](zarr_gdal_roadmap.md)** - Development roadmap and future plans
- **[Current Capabilities](__Current%20Capabilities%20of%20the%20GDAL%20Zarr%20Driver__.md)** - Feature overview

### Support & Troubleshooting

- **[FAQ](faq.md)** - Frequently asked questions
- **[Troubleshooting](troubleshooting.md)** - Common issues and solutions
- **[Benchmarks](benchmarks.md)** - Performance benchmarks and optimization

## 🚀 Quick Links

### For Users

1. [Install the plugin](installation.md)
2. [Read the user guide](user-guide.md)
3. [Check usage examples](../USAGE_EXAMPLES.md)

### For Developers

1. [Development setup](development.md)
2. [Architecture overview](architecture.md)
3. [Contributing guidelines](../CONTRIBUTING.md)

## 📖 Additional Resources

- **[Main README](../README.md)** - Project overview
- **[Changelog](../CHANGELOG.md)** - Version history
- **[Contributing](../CONTRIBUTING.md)** - How to contribute
- **[Security](../SECURITY.md)** - Security policies
- **[License](../LICENSE)** - Project license

## 🔧 Testing

The project includes comprehensive unit tests:

- Path parsing validation
- Driver integration tests  
- Backward compatibility verification
- Live cloud data testing

Run tests with: `cmake --build . && ctest`

---

*For questions or support, please check the [FAQ](faq.md) or open an issue on GitHub.*
