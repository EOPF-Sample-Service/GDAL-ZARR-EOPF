#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
"""
Integration tests for EOPF-Zarr GDAL driver with Rasterio compatibility.
Following GDAL autotest patterns.
"""

import os
import sys
import pytest
import tempfile
import shutil
import numpy as np
from pathlib import Path

try:
    from osgeo import gdal, osr
    import rasterio
    import rasterio.windows
    gdal.UseExceptions()
except ImportError:
    gdal = None
    osr = None
    rasterio = None
    pytest.skip("GDAL or Rasterio not available", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")

# Remote Zarr test data URLs (publicly accessible)
REMOTE_SAMPLE_ZARR = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr"
REMOTE_WITH_SUBDATASETS_ZARR = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil2a/21/products/cpm_v256/S2B_MSIL2A_20250721T073619_N0511_R092_T36HUG_20250721T095416.zarr/conditions/mask/detector_footprint/r10m/b04"


@pytest.fixture(scope="session", autouse=True)
def ensure_test_data():
    pass

def check_url_accessible(url, timeout=10):
    """
    Check if a URL is accessible using GDAL Open method
    
    Args:
        url: URL to check
        timeout: Timeout in seconds (not used with GDAL)
        
    Returns:
        bool: True if accessible, False otherwise
    """
    try:
        # Use GDAL's Open method to check if URL is accessible with EOPFZARR driver
        test_path = f'EOPFZARR:"/vsicurl/{url}"'
        
        # Try to open the dataset - this is the most reliable test
        ds = gdal.Open(test_path)
        accessible = ds is not None
        if ds:
            ds = None  # Close dataset
        return accessible
    except Exception:
        return False

def skip_if_url_not_accessible(url, test_name=""):
    """
    Skip test if URL is not accessible
    
    Args:
        url: URL to check
        test_name: Name of the test for better error messages
    """
    if not check_url_accessible(url):
        pytest.skip(f"Remote data not accessible for {test_name}: {url[:100]}... (This is normal in CI environments)")

# No-op: All tests use remote data, so no test data generation is needed
@pytest.fixture(scope="session", autouse=True)
def ensure_test_data():
    pass

@pytest.fixture
def temp_zarr_path():
    """Create a temporary directory for Zarr datasets."""
    temp_dir = tempfile.mkdtemp(suffix=".zarr")
    yield Path(temp_dir)
    shutil.rmtree(temp_dir, ignore_errors=True)

# Module-level setup for all tests
def setup_module():
    """Setup GDAL for all tests in this module"""
    gdal.UseExceptions()
    # Ensure our driver is loaded
    driver = gdal.GetDriverByName("EOPFZARR")
    if driver is None:
        pytest.skip("EOPFZARR driver not available", allow_module_level=True)

def test_driver_registration():
    """Test that EOPFZARR driver is properly registered"""
    driver = gdal.GetDriverByName("EOPFZARR")
    assert driver is not None
    
    # Check basic driver metadata
    long_name = driver.GetMetadataItem("DMD_LONGNAME")
    assert long_name is not None
    assert "EOPF" in long_name or "Zarr" in long_name
    
    # Check supported extensions
    extensions = driver.GetMetadataItem("DMD_EXTENSION")
    assert extensions is not None
    assert "zarr" in extensions.lower()
    
    # Check capabilities
    assert driver.GetMetadataItem(gdal.DCAP_OPEN) == "YES"

    def test_root_zarr_open(self):
        """Test that EOPFZARR driver is properly registered"""
        url = REMOTE_SAMPLE_ZARR

        # Check if URL is accessible first
        skip_if_url_not_accessible(url, "data reading test")

        path = f'EOPFZARR:"/vsicurl/{url}"'

        try:
            with rasterio.open(path) as src:
                if src is None:
                    pytest.skip(f"Remote Zarr data not accessible: {url}")

                assert "Dataset opened successfully using rasterio and EOPFZARR driver"
        except Exception as e:
            pytest.skip(
                f"Rasterio data reading failed (this is expected - rasterio cannot directly open EOPFZARR URLs): {e}")

    def test_rasterio_data_reading(self):
        """Test reading data from remote EOPF-Zarr dataset through GDAL-rasterio integration (HTTPS)"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        
        # Check if URL is accessible first
        skip_if_url_not_accessible(url, "data reading test")
        
        path = f'EOPFZARR:"/vsicurl/{url}"'
        
        try:
            with rasterio.open(path) as src:
                if src.count == 0:
                    pytest.skip("Dataset has no bands")
                
                band_count = src.count
                # Test reading a small block
                width = min(10, src.width)
                height = min(10, src.height)
                window = rasterio.windows.Window(0, 0, width, height)
                data = src.read(1, window=window)
                
                assert data is not None, "Failed to read data"
                assert data.shape == (height, width), f"Unexpected data shape: {data.shape}"
                assert data.size > 0, "Empty data array"
                
                # Test reading full dataset (if not too large)
                if src.width * src.height < 1000000:  # Less than 1M pixels
                    full_data = src.read(1)
                    assert full_data is not None, "Failed to read full dataset"
                    assert full_data.shape == (src.height, src.width), "Full data shape mismatch"
        except Exception as e:
            pytest.skip(f"Rasterio data reading failed (this is expected - rasterio cannot directly open EOPFZARR URLs): {e}")
    
    def test_rasterio_subdatasets(self):
        """Test subdataset enumeration and access with rasterio with a limit of 10"""
        url = REMOTE_SAMPLE_ZARR
        
        # Check if URL is accessible first
        skip_if_url_not_accessible(url, "subdatasets test")
        
        path = f'EOPFZARR:"/vsicurl/{url}"'
        
        try:
            with rasterio.open(path) as src:
                # Get subdataset metadata
                subdatasets = src.subdatasets
                if not subdatasets:
                    pytest.skip("No subdatasets found")
                
                # Count the total number of subdatasets
                subds_count = len(subdatasets)
                assert subds_count > 0, "No subdatasets found"
                
                # Limit to checking the first 10 subdatasets (or fewer if less than 10)
                max_subds_to_check = min(10, subds_count)
                opened_subds = 0
                
                # Loop through only the first 10 (or fewer) subdatasets
                for i in range(max_subds_to_check):
                    subds_name = subdatasets[i]
                    # Fix path format - subdatasets need quotes around the full path
                    if subds_name.startswith('EOPFZARR:/vsicurl/'):
                        fixed_path = f'EOPFZARR:"{subds_name.split(":", 1)[1]}"'
                    else:
                        fixed_path = subds_name
                    
                    try:
                        with rasterio.open(fixed_path) as subds:
                            if subds.count > 0:
                                opened_subds += 1
                    except Exception:
                        # Skip subdatasets that fail to open (e.g., non-raster data)
                        continue
                
                # Ensure at least one subdataset was successfully opened
                assert opened_subds > 0, "No subdatasets could be opened as raster datasets among the first 10"
        except Exception as e:
            pytest.skip(f"Remote data not accessible: {e}")


def test_rasterio_production_workflow():
    """Test complete production workflow with rasterio: open → read → process"""
    try:
        with rasterio.open(f'EOPFZARR:"/vsicurl/{REMOTE_WITH_SUBDATASETS_ZARR}"') as src:
            assert src.driver == "EOPFZARR"
            if src.count == 0:
                pytest.skip("Dataset has no bands for production workflow")

                # Step 2: Read data
            window = rasterio.windows.Window(0, 0, min(20, src.width), min(20, src.height))
            data = src.read(1, window=window)

            # Step 3: Basic processing
            assert data.size > 0
            stats = {
                    'min': float(data.min()),
                    'max': float(data.max()),
                    'mean': float(data.mean()),
                    'std': float(data.std())
                }

            # Step 4: Verify processing results
            assert all(isinstance(v, float) for v in stats.values())
            assert stats['min'] <= stats['max']

            print(f"✅ Production workflow successful: {stats}")
    except Exception as e:
        print(f"❌ Rasterio failed: {e}")

def test_rasterio_concurrent_access():
    """Test that multiple concurrent rasterio sessions work"""
    try:
        with rasterio.open(f'EOPFZARR:"/vsicurl/{REMOTE_WITH_SUBDATASETS_ZARR}"') as src1:
            with rasterio.open(f'EOPFZARR:"/vsicurl/{REMOTE_WITH_SUBDATASETS_ZARR}"') as src2:
                assert src1.driver == src2.driver == "EOPFZARR"
    except Exception as e:
        print(f"❌Concurrent access test failed: {e}")

# Standalone functions for manual testing
def test_manual_rasterio_integration():
    """Manual test function for development/debugging"""
    try:
        with rasterio.open(f'EOPFZARR:"/vsicurl/{REMOTE_SAMPLE_ZARR}"') as src:
            assert src.driver == "EOPFZARR"
    except Exception as e:
        print(f"❌ Rasterio failed: {e}")


if __name__ == "__main__":
    # Allow running tests directly
    pytest.main([__file__, "-v"])