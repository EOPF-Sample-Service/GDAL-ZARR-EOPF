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
    
    # For now, we'll just check if the dataset object is valid
    # Later you can uncomment the raster band checks once that's implemented
    
    # If raster bands are expected (uncomment when implemented):
    # assert ds.RasterCount > 0, "Dataset has no raster bands"
    # band = ds.GetRasterBand(1)
    # assert band is not None, "Could not get first raster band"
