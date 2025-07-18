# Contributing to GDAL EOPF-Zarr Plugin

Thank you for your interest in contributing! This guide provides information for developers.

## Development Setup

### Prerequisites
- GDAL 3.10+ with development headers
- CMake 3.16+
- C++14 compatible compiler
- Git

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
   cd GDAL-ZARR-EOPF
   ```

2. **Create build directory:**
   ```bash
   mkdir build && cd build
   ```

3. **Configure and build:**
   ```bash
   cmake ..
   cmake --build . --config Release
   ```

4. **Run tests:**
   ```bash
   ctest -C Release --verbose
   ```

## Code Structure

```
src/
├── eopfzarr_driver.cpp       # GDAL driver registration
├── eopfzarr_dataset.cpp      # Main dataset implementation
├── eopfzarr_dataset.h        # Dataset header
├── eopfzarr_performance.cpp  # Performance optimizations
└── eopf_metadata.cpp          # Metadata handling

include/
├── eopfzarr_performance.h    # Performance framework
└── eopfzarr_config.h         # Configuration

tests/
├── test_compatibility.cpp    # Compatibility tests
├── test_performance.cpp      # Performance benchmarks
└── test_driver_integration.cpp # Driver integration tests
```

## Development Guidelines

### Code Style
- Follow existing code formatting
- Use meaningful variable names
- Add comments for complex logic
- Include performance considerations

### Testing
- Add tests for new features
- Ensure existing tests pass
- Test with various data formats
- Verify performance doesn't regress

### Performance
- Profile code changes with large datasets
- Use the performance framework in `eopfzarr_performance.h`
- Consider memory usage and caching
- Test with remote data sources

## Submitting Changes

### Pull Request Process
1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes
4. Add/update tests
5. Ensure all tests pass
6. Commit with clear messages
7. Push and create a pull request

### Commit Messages
Use clear, descriptive commit messages:
```
Add caching for metadata operations

- Implement TTL-based metadata cache
- Reduce redundant network calls
- Improve performance for repeated access
```

## Architecture Notes

### Dataset Wrapper Pattern
The plugin wraps GDAL's Zarr driver to add EOPF-specific functionality:
- Metadata enhancement
- Geospatial coordinate detection
- Performance optimizations

### Performance Framework
The performance system includes:
- Metadata caching with TTL
- Network operation optimization
- Block access pattern tracking
- Lazy loading for expensive operations

### Const-Correctness
Be careful with const methods and GDAL's non-const interface:
- Use const_cast sparingly and document why
- Prefer mutable members for caching
- Consider lazy initialization patterns

## Debugging

### Build Issues
```bash
# Verbose CMake output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Runtime Debugging
```bash
# Enable GDAL debug output
export CPL_DEBUG=ON

# Enable plugin-specific debug
export CPL_DEBUG=EOPFZARR

# Enable performance timers
export EOPF_ENABLE_PERFORMANCE_TIMERS=1
```

## Getting Help

- **Issues**: Use GitHub Issues for bug reports and feature requests
- **Discussions**: Use GitHub Discussions for questions
- **Code Review**: All changes require code review

## Release Process

1. Update version numbers
2. Update CHANGELOG.md
3. Tag release: `git tag v1.0.0`
4. Build and test across platforms
5. Create GitHub release with binaries
