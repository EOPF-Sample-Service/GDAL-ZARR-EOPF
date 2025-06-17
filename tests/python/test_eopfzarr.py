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

def test_dataset_open(zarr_path, expected_driver="EOPFZARR"):
    # Test opening a dataset
    ds = gdal.Open(zarr_path)
    assert ds is not None, f"Failed to open dataset: {zarr_path}"
    driver_name = ds.GetDriver().ShortName
    assert driver_name == expected_driver, f"Expected driver {expected_driver}, got {driver_name}"
    print(f"? Successfully opened {zarr_path} with {driver_name} driver")
    return ds

def test_subdatasets(zarr_path):
    # Test listing subdatasets
    ds = gdal.Open(zarr_path)
    subdatasets = ds.GetSubDatasets()
    print(f"Found {len(subdatasets)} subdatasets")
    
    # Test opening first subdataset if any exist
    if subdatasets:
        subds_path = subdatasets[0][0]
        print(f"Testing subdataset: {subds_path}")
        subds = gdal.Open(subds_path)
        assert subds is not None, f"Failed to open subdataset: {subds_path}"
        print(f"? Successfully opened subdataset {subds_path}")
        subds = None
    
    ds = None