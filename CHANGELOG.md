# Changelog

All notable changes to the GDAL EOPF-Zarr Plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Apple Silicon (arm64) support in GitHub Actions CI/CD pipeline
- Separate build artifacts for macOS Intel (x86_64) and Apple Silicon (arm64)
- Architecture-specific artifact naming: `gdal_EOPFZarr-macOS-x86_64` and `gdal_EOPFZarr-macOS-arm64`

### Changed

- GitHub Actions now builds on both `macos-13` (Intel) and `macos-14` (Apple Silicon)
- Improved artifact upload specificity with platform-specific steps
- Enhanced error detection with `if-no-files-found: error` for all artifacts

### Planning

- Future enhancements and improvements

## [1.0.0] - 2025-07-29

### Added

- Comprehensive unit testing framework
- Cross-platform build system with CMake
- GDAL driver for EOPF Zarr datasets
- QGIS integration with "Add Raster Layer" support
- Automatic CRS and geotransform detection
- Subdataset support for hierarchical data
- Cloud-native access via HTTP/HTTPS and VSI
- Cross-platform support (Windows, macOS, Linux)
- Basic metadata attachment from EOPF specifications

### Changed

- Improved const-correctness for Linux compiler compatibility
- Enhanced geospatial coordinate detection from corner coordinates
- Optimized metadata loading with smart caching strategies
- Streamlined documentation for production focus

### Fixed

- Linux build compilation errors with const-correctness
- Memory management in metadata operations
- Performance issues with repeated metadata access
- Subdataset path handling for URL-based datasets

## Development History

### Performance Optimization Phase

- Implemented comprehensive caching framework
- Added performance monitoring and profiling tools
- Optimized network operations and metadata access
- Enhanced block-level data access patterns

### Cleanup and Documentation Phase

- Removed excessive verbose documentation
- Streamlined to essential user and developer guides
- Focused on production-ready documentation
- Maintained practical examples and troubleshooting

### Build System Improvements

- Cross-platform CMake configuration
- Automated installation scripts for Windows, macOS, Linux
- CI/CD integration with GitHub Actions
- Comprehensive testing framework

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for information on how to contribute to this project.

## Support

- **Issues**: [GitHub Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)
- **Documentation**: [Installation](INSTALLATION.md) | [Usage](USAGE.md) | [Troubleshooting](TROUBLESHOOTING.md)
