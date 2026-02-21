# EOPF-Zarr Driver Tests

## Quick Start

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Test Suites

### Unit Tests (no external dependencies)

| Test | File | Description |
|------|------|-------------|
| `basic_functionality` | `test_basic_functionality.cpp` | Core string operations and path validation |
| `unit_path_parsing` | `test_path_parsing.cpp` | URL parsing and subdataset extraction |
| `unit_path_parsing_simple` | `test_path_parsing_unit.cpp` | Lightweight path parsing (no GDAL link) |
| `unit_bbox_ordering` | `test_bbox_ordering_unit.cpp` | EOPF bbox ordering detection (Issue #135) |
| `sample_data_structure` | `test_sample_data_structure.cpp` | Sample data directory validation |

### Integration Tests (require GDAL runtime + `GDAL_DRIVER_PATH`)

| Test | File | Description |
|------|------|-------------|
| `driver_registration` | *(CMakeLists.txt)* | Verifies `gdalinfo --formats` lists EOPFZARR |
| `integration_driver` | `test_driver_integration.cpp` | Driver registration and dataset opening |
| `compatibility_urls` | `test_compatibility.cpp` | Backward compatibility and format support |
| `subdataset_formats` | `test_subdataset_formats.cpp` | Subdataset path format handling |
| `real_data_integration` | `test_real_data_integration.cpp` | Tests with sample data files |
| `geolocation_arrays` | `test_geolocation_arrays.cpp` | Geolocation array detection (Issue #137) |
| `gdal_basic` | `test_gdal_basic.cpp` | Basic GDAL API functionality |

### Summary Tests

| Test | Description |
|------|-------------|
| `core_test_summary` | Passes when all unit tests pass |
| `full_test_summary` | Passes when all tests (unit + integration) pass |

## Running Specific Tests

```bash
# Unit tests only
ctest --test-dir build -R "unit_|basic_|sample_"

# Integration tests only
ctest --test-dir build -R "integration_|compatibility_|subdataset_|geolocation_"

# Single test
ctest --test-dir build -R "unit_bbox_ordering" --verbose
```

## Support Files

| File | Purpose |
|------|---------|
| `test_implementation.cpp` | Shared test utilities |
| `test_utils.h` | Common test headers |
| `sample_data/` | Test data files |
