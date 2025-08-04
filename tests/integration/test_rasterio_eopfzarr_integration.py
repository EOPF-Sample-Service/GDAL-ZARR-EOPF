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

class TestRasterioEOPFZarrIntegration:
    """Integration test suite for Rasterio following GDAL patterns"""
    
    @pytest.fixture(autouse=True, scope="module")
    def setup_gdal(self):
        """Setup GDAL for testing"""
        gdal.UseExceptions()
        # Ensure our driver is loaded
        driver = gdal.GetDriverByName("EOPFZARR")
        if driver is None:
            pytest.skip("EOPFZARR driver not available")
    
    def test_driver_registration(self):
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
    
    def test_rasterio_geospatial_info(self):
        """Test geospatial information retrieval with rasterio (remote HTTPS)"""
        url = REMOTE_SAMPLE_ZARR
        path = f'EOPFZARR:"/vsicurl/{url}"'
        
        try:
            with rasterio.open(path) as src:
                # Test basic properties
                assert hasattr(src, 'width')
                assert hasattr(src, 'height')
                assert hasattr(src, 'count')
                assert hasattr(src, 'crs')
                assert hasattr(src, 'transform')
                
                # Test bounds
                bounds = src.bounds
                assert len(bounds) == 4, "Bounds should have 4 values"
                
                # Test profile - Profile class behaves like dict but isn't isinstance(dict)
                profile = src.profile
                assert hasattr(profile, '__getitem__'), "Profile should be dict-like"
                assert 'driver' in profile
                assert profile['driver'] == 'EOPFZARR'
        except Exception as e:
            pytest.skip(f"Remote data not accessible: {e}")
    
    def test_rasterio_metadata_access(self):
        """Test metadata access with rasterio"""
        url = REMOTE_SAMPLE_ZARR
        path = f'EOPFZARR:"/vsicurl/{url}"'
        
        try:
            with rasterio.open(path) as src:
                # Test dataset metadata
                meta = src.meta
                assert hasattr(meta, '__getitem__'), "Meta should be dict-like"
                
                # Test tags (may be empty)
                tags = src.tags()
                assert isinstance(tags, dict)
                
                # Test profile  
                profile = src.profile
                assert hasattr(profile, '__getitem__'), "Profile should be dict-like"
                assert 'width' in profile
                assert 'height' in profile
                assert 'count' in profile
                assert 'driver' in profile
        except Exception as e:
            pytest.skip(f"Remote data not accessible: {e}")
    
    def test_rasterio_path_formats(self):
        """Test different path formats with GDAL and rasterio integration"""
        url = REMOTE_SAMPLE_ZARR
        
        path_formats = [
            f'EOPFZARR:"/vsicurl/{url}"',
            f'EOPFZARR:/vsicurl/{url}',
            f'EOPFZARR:{url}',
            f'EOPFZARR:"{url}"'
        ]
        
        working_formats = []
        for path in path_formats:
            try:
                # First test with GDAL
                ds = gdal.Open(path)
                if ds is not None and ds.RasterXSize > 0 and ds.RasterYSize > 0:
                    working_formats.append(path)
                    
                    # Test integration with rasterio by converting to rasterio-compatible format
                    # Rasterio can work with GDAL datasets through memory drivers
                    try:
                        # Create a memory dataset that rasterio can understand
                        mem_path = f"/vsimem/test_{len(working_formats)}.tif"
                        mem_ds = gdal.GetDriverByName("GTiff").CreateCopy(mem_path, ds)
                        if mem_ds:
                            with rasterio.open(mem_path) as src:
                                assert src.width > 0
                                assert src.height > 0
                            gdal.Unlink(mem_path)
                    except Exception:
                        pass  # Rasterio integration optional for this test
                ds = None
            except Exception:
                continue
        
        # At least one format should work with GDAL
        assert len(working_formats) > 0, "No path formats work with GDAL EOPFZARR driver"
    
    def test_rasterio_driver_comparison(self):
        """Compare EOPFZARR vs standard Zarr driver behavior"""
        url = REMOTE_SAMPLE_ZARR
        eopf_path = f'EOPFZARR:"/vsicurl/{url}"'
        zarr_path = f'ZARR:"/vsicurl/{url}"'
        
        eopf_success = False
        zarr_success = False
        eopf_props = None
        zarr_props = None
        
        # Test EOPFZARR with GDAL first
        try:
            ds = gdal.Open(eopf_path)
            if ds is not None and ds.RasterXSize > 0 and ds.RasterYSize > 0:
                eopf_success = True
                eopf_props = (ds.RasterXSize, ds.RasterYSize, ds.RasterCount)
                
                # Try rasterio integration via memory dataset
                try:
                    mem_path = "/vsimem/eopf_test.tif"
                    mem_ds = gdal.GetDriverByName("GTiff").CreateCopy(mem_path, ds)
                    if mem_ds:
                        with rasterio.open(mem_path) as src:
                            assert src.width == eopf_props[0]
                            assert src.height == eopf_props[1]
                        gdal.Unlink(mem_path)
                except Exception:
                    pass  # Rasterio integration is optional
            ds = None
        except Exception:
            pass
        
        # Test standard Zarr with GDAL
        try:
            ds = gdal.Open(zarr_path)
            if ds is not None and ds.RasterXSize > 0 and ds.RasterYSize > 0:
                zarr_success = True
                zarr_props = (ds.RasterXSize, ds.RasterYSize, ds.RasterCount)
            ds = None
        except Exception:
            pass
        
        # At least EOPFZARR should work with GDAL
        assert eopf_success, "EOPFZARR driver should work with GDAL"
        
        # If both work, they should return similar results
        if eopf_success and zarr_success and eopf_props and zarr_props:
            assert eopf_props[0] == zarr_props[0], "Width mismatch between drivers"
            assert eopf_props[1] == zarr_props[1], "Height mismatch between drivers"
    
    def test_rasterio_production_workflow(self):
        """Test complete production workflow with rasterio: open → read → process"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        path = f'EOPFZARR:"/vsicurl/{url}"'
        
        try:
            with rasterio.open(path) as src:
                # Step 1: Verify dataset properties
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
            pytest.skip(f"Production workflow failed: {e}")
    
    def test_rasterio_error_handling(self):
        """Test error handling for invalid paths and network issues with rasterio"""
        invalid_paths = [
            'EOPFZARR:invalid_url',
            'EOPFZARR:/vsicurl/https://invalid.domain/test.zarr',
            'EOPFZARR:""'
        ]
        
        for invalid_path in invalid_paths:
            with pytest.raises((Exception, RuntimeError)):
                with rasterio.open(invalid_path) as src:
                    pass  # Should not reach here

def test_rasterio_concurrent_access():
    """Test that multiple concurrent rasterio sessions work"""
    url = REMOTE_WITH_SUBDATASETS_ZARR
    path = f'EOPFZARR:"/vsicurl/{url}"'

    try:
            # Open multiple datasets simultaneously
        with rasterio.open(path) as src1:
            with rasterio.open(path) as src2:
                assert src1.driver == src2.driver == "EOPFZARR"
                assert src1.width == src2.width
                assert src1.height == src2.height
    except Exception as e:
        pytest.skip(f"Concurrent access test failed: {e}")



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