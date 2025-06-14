#!/usr/bin/env python
"""
Tests for the EOPFZarr GDAL driver plugin.
"""

import os
import pytest
from osgeo import gdal

# Find the sample data relative to this test file
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SAMPLE_DATA_DIR = os.path.join(os.path.dirname(SCRIPT_DIR), '..', 'src', 'sample_data')

def test_driver_registration():
    """Test that the EOPFZARR driver is registered with GDAL."""
    driver = gdal.GetDriverByName('EOPFZARR')
    assert driver is not None, "EOPFZARR driver not found - check GDAL_DRIVER_PATH"
    assert driver.GetDescription() == 'EOPFZARR'
    
def test_open_sample_dataset():
    """Test opening a sample dataset with the EOPFZARR driver."""
    # Make sure GDAL can find our plugin
    if 'GDAL_DRIVER_PATH' not in os.environ:
        pytest.skip("GDAL_DRIVER_PATH not set - plugin may not be found")
    
    zarr_path = SAMPLE_DATA_DIR
    zarr_path = "eopf:"+zarr_path
    ds = gdal.OpenEx(zarr_path, gdal.OF_READONLY)
    
    assert ds is not None, f"Failed to open sample dataset at {zarr_path}"
    
    # Check if driver name is correct
    driver_name = ds.GetDriver().ShortName
    assert driver_name == 'EOPFZARR', f"Expected driver EOPFZARR, got {driver_name}"
    
    # Get metadata
    metadata = ds.GetMetadata_Dict()
    print(f"Dataset metadata: {metadata}")
    
    # For now, we'll just check if the dataset object is valid
    # Later you can uncomment the raster band checks once that's implemented
    
    # If raster bands are expected (uncomment when implemented):
    # assert ds.RasterCount > 0, "Dataset has no raster bands"
    # band = ds.GetRasterBand(1)
    # assert band is not None, "Could not get first raster band"
    
    # Clean up
    ds = None
