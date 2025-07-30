# Test Data for EOPF-Zarr Driver Integration Tests

This directory contains test datasets for comprehensive integration testing.

## Test Dataset Structure

```text
data/
├── sample.zarr/               # Basic Zarr dataset
├── with_subdatasets.zarr/     # Dataset with multiple subdatasets  
├── georeferenced.zarr/        # Dataset with geospatial information
├── with_metadata.zarr/        # Dataset with EOPF metadata
├── performance_test.zarr/     # Large dataset for performance testing
└── network_test/              # Datasets for network access testing
```

## Dataset Requirements

### sample.zarr/

- Simple 2D array with basic Zarr metadata
- Used for basic open/read functionality tests

### with_subdatasets.zarr/

- Multiple arrays/groups representing subdatasets
- Tests subdataset enumeration and access

### georeferenced.zarr/

- Contains geospatial reference information
- Tests CRS and geotransform detection

### with_metadata.zarr/

- Contains EOPF-specific metadata
- Tests metadata parsing and attachment

## Creating Test Data

You can create these test datasets using:

```python
import zarr
import numpy as np

# Create sample.zarr
store = zarr.DirectoryStore('sample.zarr')
root = zarr.group(store=store)
array = root.create_dataset('data', data=np.random.randint(0, 255, (100, 100), dtype=np.uint8))
array.attrs['units'] = 'Digital Numbers'
```

## External Test Data

For network testing, the integration tests may reference publicly available EOPF datasets.
These are only used if accessible and don't cause test failures if unavailable.
