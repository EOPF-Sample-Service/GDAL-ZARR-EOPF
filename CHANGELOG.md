# Changelog

All notable changes to the GDAL EOPF-Zarr Plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Configurable Warning Suppression** ([#172](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/172))
  - Added `EOPFZARR_SUPPRESS_AUX_WARNING` configuration option to control `.aux.xml` save warnings
  - Default behavior: warnings are suppressed for remote datasets (improves user experience)
  - Users can enable warnings with `EOPFZARR_SUPPRESS_AUX_WARNING=NO` for debugging
  - Available via configuration option, open option (`SUPPRESS_AUX_WARNING`), or environment variable

### Planning

- Future enhancements and improvements

## [3.0.0] - 2025-12-05

### 🎉 Major Release - Geolocation Arrays & Multi-Product Support

This release introduces native geolocation arrays support, comprehensive Sentinel-3 multi-product validation, and significant improvements to build system hardening and cross-platform compatibility.

### Added

- **Native Geolocation Arrays Support** ([#137](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/137), [#138](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/138))
  - Automatic detection and exposure of latitude/longitude arrays via GDAL GEOLOCATION metadata
  - Enables `gdal.Warp()` for reprojecting swath data to regular grids
  - Works with QGIS/ArcGIS for proper geographic display without manual coordinate handling
  - Supports command-line reprojection via `gdalwarp`
  - Full test coverage for geolocation arrays functionality

- **Sentinel-3 Multi-Product Validation** ([#134](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/134), [#146](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/146))
  - New comprehensive demo notebook: `10-Sentinel-3-Multi-Product-Demo.ipynb`
  - Validated support for all 5 Sentinel-3 product types:
    - OLCI L1 EFR (Full Resolution TOA Radiances)
    - OLCI L1 ERR (Reduced Resolution TOA Radiances)
    - OLCI L2 LFR (Land Products: GIFAPAR, OTCI, IWV)
    - SLSTR L1 RBT (Radiances & Brightness Temperatures)
    - SLSTR L2 LST (Land Surface Temperature)
  - Geographic visualization demonstrating Array→Geographic transformation

- **Rioxarray Compatibility** ([#101](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/101), [#121](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/121))
  - Full compatibility with rioxarray library for xarray-based workflows
  - Preserved GDAL subdataset format for seamless integration
  - Added rioxarray usage documentation to README
  - CI/CD pipeline integration tests for rioxarray

- **Docker Images for QGIS** ([#108](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/108), [#109](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/109))
  - Publicly available Docker image: `eopf-sample-service/eopfzarr-qgis`
  - Docker VNC support for remote QGIS access
  - Dual-service startup script for running QGIS and JupyterLab simultaneously

- **Build System Hardening** ([#141](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/141), [#142](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/142))
  - Enhanced CI/CD pipeline with AddressSanitizer and clang-tidy
  - UbuntuGIS PPA integration for latest GDAL versions
  - Comprehensive compiler warning flags for code quality

- **New Jupyter Notebooks**
  - `07-Sentinel-3-OLCI-Level-1-EFR.ipynb` - OLCI L1 EFR exploration
  - `10-Sentinel-3-Multi-Product-Demo.ipynb` - Multi-product validation demo

### Fixed

- **EOPF Bounding Box Ordering** ([#135](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/135), [#136](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/136))
  - Fixed handling of EOPF bbox ordering for correct geotransform calculation
  - Added support for UTM products without `proj:bbox` metadata
  - Comprehensive tests for bbox ordering scenarios

- **GDAL 3.12 Compatibility** ([#144](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/144))
  - Fixed GDALGeoTransform API changes for GDAL 3.12
  - Fixed cache method calls for new GeoTransform reference type
  - MSVC compiler warnings resolved for Windows CI

- **Linux Build Compatibility** ([#145](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/145))
  - Refactored `eopfzarr_dataset.cpp` for Linux compiler compatibility
  - Fixed CMakeLists.txt for cross-platform builds
  - Resolved GitHub workflow errors

- **QGIS Layer Naming** ([#118](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/pull/118))
  - Fixed layer naming for URL-based datasets
  - Improved URL parsing for subdataset paths

### Changed

- Upgraded CI/CD to Ubuntu 24.04 with ubuntugis-unstable PPA
- Improved subdataset path format handling (colon-separated format)
- Enhanced integration test error handling for unavailable remote data
- Streamlined notebook documentation with table of contents

### Documentation

- Updated README with rioxarray usage examples
- Added comprehensive troubleshooting for geolocation arrays
- Docker image usage documentation
- Multi-product validation results

## [2.0.0] - 2025-08-06

### Added

- Sentinel-2 exploration notebooks
- Unit tests and integration tests expansion
- Subdataset path format improvements

### Fixed

- Linux build compatibility with cstring header
- CMakeLists.txt improvements for cross-platform builds

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
