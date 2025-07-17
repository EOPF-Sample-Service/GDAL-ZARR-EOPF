# Project Structure

This document outlines the clean, organized structure of the GDAL EOPF-Zarr plugin project.

## 📁 Root Directory Structure

```
GDAL-ZARR-EOPF/
├── src/                          # Source code
│   ├── eopfzarr_driver.cpp      # Main driver implementation
│   ├── eopfzarr_dataset.cpp     # Dataset handling
│   └── eopf_metadata.cpp        # Metadata processing
├── include/                      # Header files
│   ├── eopf_metadata.h          # Metadata interfaces
│   └── eopf_zarr.h              # Main plugin headers
├── tests/                        # Unit testing framework
│   ├── test_path_parsing.cpp    # Path parsing tests
│   ├── test_driver_integration.cpp # Driver integration tests
│   ├── test_compatibility.cpp   # Backward compatibility tests
│   ├── test_implementation.cpp  # Test utilities
│   └── test_utils.h            # Test interfaces
├── docs/                        # Documentation
│   ├── README.md               # Documentation index
│   ├── installation.md        # Installation guide
│   ├── user-guide.md          # User documentation
│   ├── architecture.md        # Technical architecture
│   ├── api.md                 # API reference
│   ├── development.md         # Development guide
│   ├── faq.md                 # FAQ
│   ├── troubleshooting.md     # Troubleshooting
│   ├── benchmarks.md          # Performance benchmarks
│   ├── zarr_v3_spec.md        # Zarr specifications
│   ├── zarr_gdal_roadmap.md   # Development roadmap
│   └── *.md                   # Additional technical docs
├── notebooks/                  # Jupyter examples
├── cmake/                     # CMake modules
├── .github/                  # GitHub workflows
└── build/                    # Build artifacts (gitignored)
```

## 📋 File Descriptions

### Core Source Files

- **`src/eopfzarr_driver.cpp`** - Main GDAL driver implementation with registration and dataset opening logic
- **`src/eopfzarr_dataset.cpp`** - Dataset class handling raster data, metadata, and coordinate systems
- **`src/eopf_metadata.cpp`** - EOPF metadata processing and Zarr attribute parsing

### Testing Framework

- **`tests/test_path_parsing.cpp`** - Unit tests for URL/path parsing with 100% coverage validation
- **`tests/test_driver_integration.cpp`** - GDAL driver registration and integration tests
- **`tests/test_compatibility.cpp`** - Backward compatibility tests with live cloud data
- **`tests/test_implementation.cpp`** - Shared test utilities and helper functions

### Build System

- **`CMakeLists.txt`** - Main CMake configuration with GDAL detection, test framework, and cross-platform build
- **`cmake/`** - Additional CMake modules and find scripts

### Documentation

- **`docs/README.md`** - Comprehensive documentation index and navigation
- **Installation guides** - Platform-specific installation instructions
- **Technical docs** - Architecture, API reference, and development guides
- **Specification docs** - Zarr format specifications and roadmaps

## 🧹 Cleaned Up (Removed)

The following redundant files and directories have been removed:

### Redundant Build Directories
- `build-refactored/`, `build-test/`, `test-build*/`, `release-build/`, `build_static/` - Multiple old build artifacts
- `test_workspace/`, `out/` - Temporary directories

### Redundant Documentation
- `CONTRIBUTION.md` - Empty duplicate of `CONTRIBUTING.md`
- `REFACTORING_SUMMARY.md` - Outdated refactoring notes
- `INSTALLATION_SCRIPTS_README.md`, `HOW_TO_GET_PLUGIN.md` - Content merged into main docs
- `TEST_CHECKLIST.md`, `RELEASE_CHECKLIST.md` - Development artifacts

### Redundant Test Files
- `test_*.bat` files - Replaced by proper unit testing framework
- `watch.ps1` - Development utility script

### Redundant Folders
- `markdown/` - Content moved to `docs/`
- `miscellaneous/` - Single README file, content relocated

## ✅ Benefits of Clean Structure

1. **Clear separation of concerns** - Source, tests, docs, and build artifacts are properly organized
2. **Reduced cognitive load** - No redundant files to confuse developers
3. **Better CI/CD** - Cleaner build processes and testing workflows
4. **Improved maintainability** - Easier to find and modify specific components
5. **Professional presentation** - Clean repository suitable for open source collaboration

## 🚀 Next Steps

With the cleaned structure in place, the project is now ready for:

1. **Performance optimization** - Focus on driver efficiency improvements
2. **Feature enhancement** - Add new capabilities with clear code organization
3. **Documentation updates** - Maintain high-quality docs in the organized structure
4. **Community contributions** - Easy for new contributors to understand and participate

---

*This structure follows GDAL plugin best practices and modern C++ project organization standards.*
