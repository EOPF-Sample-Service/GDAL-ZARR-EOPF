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
    ds = gdal.OpenEx(zarr_path, gdal.OF_READONLY, open_options=['EOPF_PROCESS=YES'])
    
    assert ds is not None, f"Failed to open sample dataset at {zarr_path}"
    
    # Basic validations - adjust based on your sample dataset's properties
    assert ds.RasterCount > 0, "Dataset has no raster bands"
    assert ds.RasterXSize > 0, "Dataset has no width"
    assert ds.RasterYSize > 0, "Dataset has no height"
    
    # Test metadata if available
    metadata = ds.GetMetadata_Dict()
    assert len(metadata) > 0, "No metadata found in dataset"
    
    # Clean up
    ds = None