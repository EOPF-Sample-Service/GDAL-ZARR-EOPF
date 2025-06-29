# Changelog

All notable changes to the EOPF-Zarr GDAL plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial development of EOPF-Zarr GDAL driver
- Basic Zarr format reading support
- CMake build system
- GitHub Actions workflows for CI/CD
- Comprehensive documentation structure
- CODEOWNERS file for code review process
- Pull request templates

### Changed
- Improved branch protection workflows
- Enhanced CODEOWNERS patterns

### Deprecated
- None

### Removed
- None

### Fixed
- None

### Security
- Added signed commit requirements
- Implemented branch protection rules

## [0.1.0] - 2024-XX-XX (Planned)

### Added
- Basic Zarr v2 format support
- Read-only access to EOPF datasets
- Multi-band raster support
- GDAL dataset and raster band implementations
- Basic metadata extraction
- CMake build configuration
- Unit tests framework
- Documentation structure

### Technical Details
- Implemented `EOPFDataset` class extending `GDALDataset`
- Implemented `EOPFRasterBand` class extending `GDALRasterBand`
- Added driver registration and identification
- Basic error handling and logging
- Memory management following GDAL conventions

## [0.2.0] - 2024-XX-XX (Planned)

### Added
- Improved metadata handling
- Georeferencing support
- Coordinate system detection
- Enhanced error reporting
- Performance optimizations for chunked data access

### Changed
- Optimized memory usage for large datasets
- Improved chunk alignment for better performance

### Fixed
- Memory leaks in dataset opening
- Thread safety issues
- Metadata parsing edge cases

## [0.3.0] - 2024-XX-XX (Planned)

### Added
- Cloud storage support (S3, Azure, GCS)
- Compression format support (gzip, lz4, blosc)
- Multi-dimensional array support (3D, 4D)
- Time series data handling
- Advanced metadata attributes

### Changed
- Enhanced EOPF-specific metadata parsing
- Improved virtual file system integration

## [1.0.0] - 2024-XX-XX (Planned)

### Added
- Full EOPF specification compliance
- Write support for Zarr format
- Complete test coverage
- Performance benchmarks
- User documentation
- QGIS integration validation

### Changed
- Stable API with backward compatibility guarantees
- Production-ready error handling
- Comprehensive logging system

### Performance
- Optimized for large-scale Earth Observation datasets
- Efficient memory usage patterns
- Parallel processing support where applicable

## Version Numbering

This project uses semantic versioning:
- **MAJOR** version for incompatible API changes
- **MINOR** version for backward-compatible functionality additions  
- **PATCH** version for backward-compatible bug fixes

## Release Process

1. All changes are developed in feature branches
2. Pull requests required for all changes to main branch
3. Automated testing must pass before merging
4. Version numbers updated in CMakeLists.txt
5. Changelog updated with release notes
6. Git tags created for each release
7. Release artifacts built and published

## Development Milestones

- **Alpha**: Basic functionality, development testing
- **Beta**: Feature complete, user testing
- **Release Candidate**: Production testing, bug fixes only
- **General Availability**: Production ready, full support

## Breaking Changes Policy

Breaking changes will be:
- Documented in this changelog under "Changed" or "Removed"
- Announced in advance through GitHub issues
- Accompanied by migration guides where applicable
- Only introduced in major version releases (after 1.0.0)

## Support Policy

- **Latest major version**: Full support including new features
- **Previous major version**: Security and critical bug fixes only
- **Older versions**: Community support only

## Contributing to Releases

- Feature requests via GitHub issues
- Bug reports with reproduction steps
- Pull requests with comprehensive testing
- Documentation improvements welcome
- Performance optimizations and benchmarks valued
