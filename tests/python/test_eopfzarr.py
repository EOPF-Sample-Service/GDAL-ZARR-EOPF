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
    if 'GDAL_DRIVER_PATH' not in os.environ:
        pytest.skip("GDAL_DRIVER_PATH not set - plugin may not be found")
    
    zarr_path = SAMPLE_DATA_DIR
    zarr_path = "eopf:"+zarr_path
    ds = gdal.OpenEx(zarr_path, gdal.OF_READONLY)
    
    if ds is None:
        print(f"Failed to open dataset at {zarr_path}. Check if the plugin and sample data are correctly set up.")
    assert ds is not None, f"Failed to open sample dataset at {zarr_path}"
