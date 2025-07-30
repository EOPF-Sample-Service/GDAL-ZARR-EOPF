#!/usr/bin/env python3
"""
Test data generation script for EOPF-Zarr driver integration tests.

This script creates various Zarr datasets for testing different scenarios:
- Basic Zarr datasets
- Datasets with subdatasets/groups
- Georeferenced datasets with CRS information
- Datasets with EOPF-specific metadata
"""

import os
import numpy as np
import zarr
from pathlib import Path


def create_sample_zarr(data_dir: Path):
    """Create a basic Zarr dataset for simple read/write tests."""
    zarr_path = data_dir / "sample.zarr"
    if zarr_path.exists():
        import shutil
        shutil.rmtree(zarr_path)
    
    store = zarr.DirectoryStore(str(zarr_path))
    root = zarr.group(store=store)
    
    # Create 2D data array
    data = np.random.randint(0, 255, (100, 100), dtype=np.uint8)
    array = root.create_dataset('data', data=data, chunks=(50, 50))
    
    # Add basic attributes
    array.attrs['units'] = 'Digital Numbers'
    array.attrs['description'] = 'Sample test data for EOPF-Zarr driver'
    array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
    
    root.attrs['title'] = 'Sample Zarr Dataset'
    root.attrs['created_by'] = 'EOPF-Zarr test suite'
    
    print(f"Created sample.zarr at {zarr_path}")


def create_subdatasets_zarr(data_dir: Path):
    """Create Zarr dataset with multiple subdatasets (groups)."""
    zarr_path = data_dir / "with_subdatasets.zarr"
    if zarr_path.exists():
        import shutil
        shutil.rmtree(zarr_path)
    
    store = zarr.DirectoryStore(str(zarr_path))
    root = zarr.group(store=store)
    
    # Create multiple groups representing subdatasets
    band1_group = root.create_group('band1')
    band2_group = root.create_group('band2')
    metadata_group = root.create_group('metadata')
    
    # Band 1 data
    band1_data = np.random.randint(0, 1000, (200, 300), dtype=np.uint16)
    band1_array = band1_group.create_dataset('data', data=band1_data, chunks=(100, 150))
    band1_array.attrs['wavelength'] = 665.0
    band1_array.attrs['band_name'] = 'Red'
    band1_array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
    
    # Band 2 data
    band2_data = np.random.randint(0, 1000, (200, 300), dtype=np.uint16)
    band2_array = band2_group.create_dataset('data', data=band2_data, chunks=(100, 150))
    band2_array.attrs['wavelength'] = 842.0
    band2_array.attrs['band_name'] = 'NIR'
    band2_array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
    
    # Metadata
    metadata_group.attrs['sensor'] = 'Test Sensor'
    metadata_group.attrs['acquisition_date'] = '2024-01-15'
    
    root.attrs['title'] = 'Multi-band Zarr Dataset'
    root.attrs['subdatasets'] = ['band1', 'band2']
    
    print(f"Created with_subdatasets.zarr at {zarr_path}")


def create_georeferenced_zarr(data_dir: Path):
    """Create georeferenced Zarr dataset with CRS information."""
    zarr_path = data_dir / "georeferenced.zarr"
    if zarr_path.exists():
        import shutil
        shutil.rmtree(zarr_path)
    
    store = zarr.DirectoryStore(str(zarr_path))
    root = zarr.group(store=store)
    
    # Create georeferenced data
    height, width = 150, 200
    data = np.random.randint(0, 4095, (height, width), dtype=np.uint16)
    array = root.create_dataset('data', data=data, chunks=(75, 100))
    
    # Add geospatial attributes
    array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
    array.attrs['units'] = 'Digital Numbers'
    
    # Add coordinate reference system information
    root.attrs['crs'] = 'EPSG:4326'
    root.attrs['geotransform'] = [
        -180.0,  # x_origin (top-left x)
        360.0 / width,  # pixel_width
        0.0,     # rotation_x
        90.0,    # y_origin (top-left y)
        0.0,     # rotation_y
        -180.0 / height  # pixel_height (negative for north-up)
    ]
    
    # Add coordinate arrays
    x_coords = np.linspace(-180.0, 180.0, width)
    y_coords = np.linspace(90.0, -90.0, height)
    
    x_array = root.create_dataset('x', data=x_coords)
    x_array.attrs['units'] = 'degrees_east'
    x_array.attrs['long_name'] = 'longitude'
    
    y_array = root.create_dataset('y', data=y_coords)
    y_array.attrs['units'] = 'degrees_north'
    y_array.attrs['long_name'] = 'latitude'
    
    root.attrs['title'] = 'Georeferenced Zarr Dataset'
    
    print(f"Created georeferenced.zarr at {zarr_path}")


def create_metadata_rich_zarr(data_dir: Path):
    """Create Zarr dataset with EOPF-specific metadata."""
    zarr_path = data_dir / "with_metadata.zarr"
    if zarr_path.exists():
        import shutil
        shutil.rmtree(zarr_path)
    
    store = zarr.DirectoryStore(str(zarr_path))
    root = zarr.group(store=store)
    
    # Create data array
    data = np.random.randint(0, 10000, (180, 240), dtype=np.uint16)
    array = root.create_dataset('reflectance', data=data, chunks=(90, 120))
    
    # Add EOPF-style metadata
    array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
    array.attrs['units'] = 'reflectance'
    array.attrs['scale_factor'] = 0.0001
    array.attrs['add_offset'] = 0.0
    array.attrs['valid_range'] = [0, 10000]
    array.attrs['_FillValue'] = 65535
    
    # Root level EOPF metadata
    root.attrs['eopf_version'] = '2.0'
    root.attrs['product_type'] = 'L2A'
    root.attrs['mission'] = 'Sentinel-2'
    root.attrs['instrument'] = 'MSI'
    root.attrs['processing_level'] = 'L2A'
    root.attrs['acquisition_start'] = '2024-01-15T10:30:00Z'
    root.attrs['acquisition_stop'] = '2024-01-15T10:30:15Z'
    root.attrs['orbit_number'] = 12345
    root.attrs['tile_id'] = 'T32TPS'
    
    # Quality metadata
    quality_group = root.create_group('quality')
    quality_data = np.random.randint(0, 100, (180, 240), dtype=np.uint8)
    quality_array = quality_group.create_dataset('data', data=quality_data, chunks=(90, 120))
    quality_array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
    quality_array.attrs['description'] = 'Quality flags'
    
    print(f"Created with_metadata.zarr at {zarr_path}")


def create_performance_test_zarr(data_dir: Path):
    """Create a larger Zarr dataset for performance testing."""
    zarr_path = data_dir / "performance_test.zarr"
    if zarr_path.exists():
        import shutil
        shutil.rmtree(zarr_path)
    
    store = zarr.DirectoryStore(str(zarr_path))
    root = zarr.group(store=store)
    
    # Create larger dataset (1000x1000 pixels)
    height, width = 1000, 1000
    chunk_size = 256
    
    # Create multiple bands for performance testing
    for band_num in range(1, 5):  # 4 bands
        band_data = np.random.randint(0, 4095, (height, width), dtype=np.uint16)
        array = root.create_dataset(
            f'band_{band_num}', 
            data=band_data, 
            chunks=(chunk_size, chunk_size),
            compressor=zarr.Blosc(cname='zstd', clevel=3)
        )
        array.attrs['_ARRAY_DIMENSIONS'] = ['y', 'x']
        array.attrs['band_number'] = band_num
        array.attrs['wavelength'] = 400 + band_num * 100  # Example wavelengths
    
    root.attrs['title'] = 'Performance Test Dataset'
    root.attrs['description'] = 'Large dataset for performance and caching tests'
    root.attrs['total_size_mb'] = (height * width * 4 * 2) / (1024 * 1024)  # Approximate size
    
    print(f"Created performance_test.zarr at {zarr_path}")


def main():
    """Generate all test datasets."""
    # Get the data directory
    script_dir = Path(__file__).parent
    data_dir = script_dir / "data"
    data_dir.mkdir(exist_ok=True)
    
    print("Generating test datasets for EOPF-Zarr driver...")
    
    try:
        create_sample_zarr(data_dir)
        create_subdatasets_zarr(data_dir)
        create_georeferenced_zarr(data_dir)
        create_metadata_rich_zarr(data_dir)
        create_performance_test_zarr(data_dir)
        
        print(f"\nAll test datasets created successfully in {data_dir}")
        print("\nDataset summary:")
        for zarr_dir in data_dir.glob("*.zarr"):
            size_mb = sum(f.stat().st_size for f in zarr_dir.rglob('*') if f.is_file()) / (1024 * 1024)
            print(f"  {zarr_dir.name}: {size_mb:.2f} MB")
            
    except ImportError as e:
        print(f"Error: Required package not found: {e}")
        print("Please install zarr and numpy: pip install zarr numpy")
    except Exception as e:
        print(f"Error creating test datasets: {e}")


if __name__ == "__main__":
    main()
