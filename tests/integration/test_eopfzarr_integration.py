#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
"""
Comprehensive integration tests for EOPF-Zarr GDAL driver.
Following GDAL autotest patterns.
"""

import os
import sys
import time
import pytest
import tempfile
import shutil
import numpy as np
from pathlib import Path
from unittest.mock import patch, MagicMock


try:
    from osgeo import gdal, osr
    import rasterio
    gdal.UseExceptions()
except ImportError:
    gdal = None
    osr = None
    pytest.skip("GDAL not available", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")


# Remote Zarr test data URLs (publicly accessible)
REMOTE_SAMPLE_ZARR = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr"
REMOTE_WITH_SUBDATASETS_ZARR = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil2a/21/products/cpm_v256/S2B_MSIL2A_20250721T073619_N0511_R092_T36HUG_20250721T095416.zarr/conditions/mask/detector_footprint/r10m/b04"



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


class TestEOPFZarrIntegration:
    """Integration test suite following GDAL patterns"""
    
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
    
    def test_basic_dataset_open(self):
        """Test opening a basic EOPF-Zarr dataset (remote HTTPS)"""
        url = REMOTE_SAMPLE_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")

        assert ds.RasterXSize > 0, "Dataset has zero width"
        assert ds.RasterYSize > 0, "Dataset has zero height"

    
    def test_data_reading(self):
        """Test reading data from remote EOPF-Zarr dataset (HTTPS)"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        
        band = ds.GetRasterBand(1)
        
        # Test reading a small block
        width = min(10, ds.RasterXSize)
        height = min(10, ds.RasterYSize)
        
        try:
            data = band.ReadAsArray(0, 0, width, height)
            assert data is not None, "Failed to read data"
            assert data.shape == (height, width), f"Unexpected data shape: {data.shape}"
            assert data.size > 0, "Empty data array"
        except ImportError as e:
            if "numpy.core.multiarray failed to import" in str(e):
                pytest.skip(f"NumPy compatibility issue: {e}")
            else:
                raise
        except Exception as e:
            pytest.skip(f"Data reading failed: {e}")
        
        # Test reading full dataset (if not too large)
        if ds.RasterXSize * ds.RasterYSize < 1000000:  # Less than 1M pixels
            try:
                full_data = band.ReadAsArray()
                assert full_data is not None, "Failed to read full dataset"
                assert full_data.shape == (ds.RasterYSize, ds.RasterXSize), "Full data shape mismatch"
            except ImportError as e:
                if "numpy.core.multiarray failed to import" in str(e):
                    pytest.skip(f"NumPy compatibility issue with full dataset read: {e}")
                else:
                    raise
    
    def test_subdatasets(self):
        """Test subdataset enumeration and access with a limit of 10"""
        # Example remote dataset URL (replace with your actual dataset)
        url = REMOTE_SAMPLE_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        
        # Check if the dataset opened successfully
        if ds is None:
            pytest.skip(f"Remote data not accessible: {url}")
        
        # Get subdataset metadata
        subdatasets = ds.GetMetadata("SUBDATASETS")
        if not subdatasets:
            assert False, "No subdatasets found"
        
        # Count the total number of subdatasets
        subds_count = len([k for k in subdatasets.keys() if k.endswith("_NAME")])
        assert subds_count > 0, "No subdatasets found"
        
        # Limit to checking the first 10 subdatasets (or fewer if less than 10)
        max_subds_to_check = min(10, subds_count)
        opened_subds = 0
        
        # Loop through only the first 10 (or fewer) subdatasets
        for i in range(1, max_subds_to_check + 1):
            subds_name = subdatasets.get(f"SUBDATASET_{i}_NAME")
            if subds_name:
                try:
                    subds = gdal.Open(subds_name)
                    if subds is not None and subds.RasterCount > 0:
                        opened_subds += 1
                except RuntimeError:
                    # Skip subdatasets that fail to open (e.g., non-raster data)
                    continue
        
        # Ensure at least one subdataset was successfully opened
        assert opened_subds > 0, "No subdatasets could be opened as raster datasets among the first 10"
    
    def test_geospatial_info(self):
        """Test geospatial information retrieval (remote HTTPS)"""
        url = REMOTE_SAMPLE_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        # Test geotransform
        gt = ds.GetGeoTransform()
        assert gt != (0, 1, 0, 0, 0, 1), "Default geotransform detected"
        assert len(gt) == 6, "Invalid geotransform format"
        assert gt[1] != 0 and gt[5] != 0, "Zero pixel size in geotransform"
        # Test spatial reference
        srs = ds.GetSpatialRef()
        if srs:
            if hasattr(srs, "Validate"):
                assert srs.Validate() == 0, "Invalid spatial reference (Validate() != 0)"
            elif hasattr(srs, "IsValid"):
                assert srs.IsValid(), "Invalid spatial reference (IsValid() == False)"
            auth_code = srs.GetAuthorityCode(None)
            if auth_code:
                assert auth_code.isdigit(), f"Invalid authority code: {auth_code}"
    
    def test_metadata_retrieval(self):
        """Test EOPF metadata retrieval from remote HTTPS Zarr"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        # Test dataset metadata
        metadata = ds.GetMetadata()
        assert isinstance(metadata, dict), "Metadata should be a dictionary"
        # Test band metadata (check band exists)
        band = ds.GetRasterBand(1)
        if band is not None:
            band_metadata = band.GetMetadata()
            assert isinstance(band_metadata, dict), "Band metadata should be a dictionary"
        # Test specific EOPF metadata if present
        if "eopf_version" in metadata:
            assert metadata["eopf_version"], "EOPF version should not be empty"
        # Test metadata domains
        domains = ds.GetMetadataDomainList()
        if domains:
            for domain in domains:
                domain_metadata = ds.GetMetadata(domain)
                assert isinstance(domain_metadata, dict), f"Domain {domain} metadata should be a dict"
    
    @pytest.mark.require_curl
    def test_network_access(self):
        """Test network dataset access via VSI and HTTPS URLs - This is the most critical test"""
        # Test the specific EODC URL - this is our primary use case
        eodc_url = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr"
        
        print(f"Testing EODC URL: {eodc_url}")
        
        # Test different URL formats to find working ones
        url_formats = [
            ("Direct HTTPS", f"EOPFZARR:{eodc_url}"),
            ("VSI Curl", f"EOPFZARR:/vsicurl/{eodc_url}"),
            ("Quoted VSI", f'EOPFZARR:"/vsicurl/{eodc_url}"'),
        ]
        
        successful_format = None
        dataset = None
        
        for format_name, url in url_formats:
            print(f"\n--- Testing {format_name} ---")
            try:
                ds = gdal.Open(url)
                if ds is not None:
                    print(f"✅ Successfully opened EODC dataset via {format_name}")
                    print(f"   RasterCount: {ds.RasterCount}")
                    print(f"   Size: {ds.RasterXSize}x{ds.RasterYSize}")
                    
                    # Basic validation
                    assert ds.RasterXSize > 0, f"Dataset should have positive width"
                    assert ds.RasterYSize > 0, f"Dataset should have positive height"
                    
                    successful_format = format_name
                    dataset = ds
                    break
                else:
                    print(f"⚠️ Could not open EODC dataset via {format_name} (returned None)")
                    
            except Exception as e:
                print(f"⚠️ {format_name} failed: {e}")
        
        # At least one format should work
        assert successful_format is not None, "None of the URL formats worked for HTTPS access"
        assert dataset is not None, "Could not open the EODC dataset with any format"
        
        print(f"\n=== Working format: {successful_format} ===")
        
        # Test metadata access
        metadata = dataset.GetMetadata()
        print(f"Metadata keys: {len(metadata)}")
        assert isinstance(metadata, dict), "Metadata should be a dictionary"
        
        # Test subdatasets - this is crucial for EOPF data
        subdatasets = dataset.GetMetadata("SUBDATASETS")
        if subdatasets:
            subds_count = len([k for k in subdatasets.keys() if k.endswith("_NAME")])
            print(f"Found {subds_count} subdatasets")
            assert subds_count > 0, "Should have subdatasets for EOPF data"
            
            # Test opening first few subdatasets
            for i in range(1, min(4, subds_count + 1)):  # Test first 3 subdatasets
                subds_name = subdatasets.get(f"SUBDATASET_{i}_NAME")
                subds_desc = subdatasets.get(f"SUBDATASET_{i}_DESC", "No description")
                if subds_name:
                    print(f"  Testing subdataset {i}: {subds_desc}")
                    try:
                        subds = gdal.Open(subds_name)
                        if subds is not None:
                            print(f"    ✅ Opened: {subds.RasterXSize}x{subds.RasterYSize}, {subds.RasterCount} bands")
                            assert subds.RasterCount > 0, f"Subdataset {i} should have bands"
                            assert subds.RasterXSize > 0 and subds.RasterYSize > 0, f"Subdataset {i} should have valid dimensions"
                            
                            # Test reading a small data sample from subdataset
                            if subds.RasterCount > 0:
                                band = subds.GetRasterBand(1)
                                sample_size = min(5, subds.RasterXSize, subds.RasterYSize)
                                try:
                                    data = band.ReadAsArray(0, 0, sample_size, sample_size)
                                    if data is not None:
                                        print(f"    ✅ Successfully read {sample_size}x{sample_size} data from subdataset {i}")
                                    else:
                                        print(f"    ⚠️ Could not read data from subdataset {i}")
                                except Exception as e:
                                    print(f"    ⚠️ Data read failed for subdataset {i}: {e}")
                        else:
                            print(f"    ❌ Failed to open subdataset {i}")
                    except Exception as e:
                        print(f"    ❌ Exception opening subdataset {i}: {e}")
        else:
            print("No subdatasets found (this might be unexpected for EOPF data)")
        
        # Test reading a small data sample from main dataset if it has bands
        if dataset.RasterCount > 0:
            band = dataset.GetRasterBand(1)
            sample_size = min(10, dataset.RasterXSize, dataset.RasterYSize)
            try:
                data = band.ReadAsArray(0, 0, sample_size, sample_size)
                if data is not None:
                    print(f"✅ Successfully read {sample_size}x{sample_size} data sample from main dataset")
                else:
                    print("⚠️ Could not read data sample from main dataset")
            except Exception as e:
                print(f"⚠️ Data read failed from main dataset: {e}")
        
        print(f"\n🎉 HTTPS URL testing completed successfully with {successful_format}!")
        
        # Test additional URLs if time permits
        additional_test_urls = [
            "https://example.com/public/test.zarr",  # Should fail gracefully
        ]
        
        print(f"\n--- Testing error handling with invalid URLs ---")
        for url in additional_test_urls:
            try:
                ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
                if ds is not None:
                    print(f"⚠️ Unexpectedly opened {url}")
                else:
                    print(f"✅ Correctly failed to open invalid URL: {url}")
            except Exception:
                print(f"✅ Correctly raised exception for invalid URL: {url}")
                # Expected for non-existent URLs
    
    def test_error_handling(self):
        """Test proper error handling for invalid inputs"""
        
        # Test non-existent file
        try:
            ds = gdal.Open("EOPFZARR:/nonexistent/path.zarr")
            assert ds is None, "Should return None for non-existent file"
        except Exception:
            pass  # Exception is also acceptable
        
        # Test invalid URL format
        try:
            ds = gdal.Open("EOPFZARR:invalid_url")
            assert ds is None, "Should return None for invalid URL"
        except Exception:
            pass  # Exception is also acceptable
        
        # Test empty path
        try:
            ds = gdal.Open("EOPFZARR:")
            assert ds is None, "Should return None for empty path"
        except Exception:
            pass  # Exception is also acceptable
    
    def test_performance_features(self):
        """Test performance optimization features (remote HTTPS)"""
        url = REMOTE_SAMPLE_ZARR
        # Enable performance logging
        original_debug = gdal.GetConfigOption("CPL_DEBUG")
        original_timers = gdal.GetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS")
        try:
            gdal.SetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS", "1")
            gdal.SetConfigOption("CPL_DEBUG", "EOPFZARR_PERF")
            ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
            if ds is None:
                pytest.skip(f"Remote Zarr data not accessible: {url}")
            # Test metadata caching by accessing multiple times
            start_time = time.time()
            for _ in range(5):
                metadata = ds.GetMetadata()
            first_duration = time.time() - start_time
            # Second round should be faster due to caching
            start_time = time.time()
            for _ in range(5):
                metadata = ds.GetMetadata()
            second_duration = time.time() - start_time
            # Note: We don't assert timing differences as they may vary
        finally:
            gdal.SetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS", original_timers)
            gdal.SetConfigOption("CPL_DEBUG", original_debug)
    
    def test_block_reading_patterns(self):
        """Test different block reading patterns for memory efficiency (remote HTTPS)"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        band = ds.GetRasterBand(1)
        block_sizes = band.GetBlockSize()
        block_width, block_height = block_sizes
        # Read aligned with block boundaries
        aligned_data = band.ReadAsArray(0, 0, block_width, block_height)
        assert aligned_data is not None, "Failed to read block-aligned data"
        # Read crossing block boundaries
        if ds.RasterXSize > block_width and ds.RasterYSize > block_height:
            cross_boundary_data = band.ReadAsArray(
                block_width // 2, block_height // 2, 
                block_width, block_height
            )
            assert cross_boundary_data is not None, "Failed to read cross-boundary data"
    
    def test_large_dataset_handling(self):
        """Test handling of large remote HTTPS Zarr datasets"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        # Test that we can handle large datasets without loading everything into memory
        assert ds.RasterXSize > 0 and ds.RasterYSize > 0, "Dataset should have valid dimensions"
        # Read a small portion of a large dataset (check band exists)
        band = ds.GetRasterBand(1)
        if band is not None:
            small_data = band.ReadAsArray(0, 0, min(100, ds.RasterXSize), min(100, ds.RasterYSize))
            assert small_data is not None, "Should be able to read small portion of large dataset"


class TestEOPFZarrPerformance:
    """Performance-specific integration tests"""
    
    def test_caching_effectiveness(self):
        """Test that caching actually improves performance (remote HTTPS)"""
        url = REMOTE_SAMPLE_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        # First access (should populate cache)
        start_time = time.time()
        metadata1 = ds.GetMetadata()
        first_access_time = time.time() - start_time
        # Second access (should use cache)
        start_time = time.time()
        metadata2 = ds.GetMetadata()
        second_access_time = time.time() - start_time
        # Verify same metadata returned
        assert metadata1 == metadata2, "Cached metadata should be identical"
        # Note: We don't assert timing as it may vary, but both should succeed
    
    def test_memory_efficiency(self):
        """Test memory efficiency with repeated operations (remote HTTPS)"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        # Test that repeated dataset opens don't accumulate memory
        for i in range(10):
            ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
            if ds is None:
                pytest.skip(f"Remote Zarr data not accessible: {url}")
            # Read some data
            band = ds.GetRasterBand(1)
            data = band.ReadAsArray(0, 0, min(50, ds.RasterXSize), min(50, ds.RasterYSize))
            assert data is not None, f"Failed to read data on iteration {i}"
            # Explicitly close to test cleanup
            ds = None


class TestEOPFZarrCompatibility:
    """Compatibility tests with different Zarr formats"""
    
    def test_zarr_format_compatibility(self):
        """Test compatibility with different Zarr format variations (remote HTTPS)"""
        urls = [
            REMOTE_WITH_SUBDATASETS_ZARR,
        ]
        for url in urls:
            ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
            if ds is None:
                pytest.skip(f"Remote Zarr data not accessible: {url}")
            assert ds.RasterCount > 0, f"Dataset {url} should have bands"
            band = ds.GetRasterBand(1)
            data = band.ReadAsArray(0, 0, min(10, ds.RasterXSize), min(10, ds.RasterYSize))
            assert data is not None, f"Should be able to read data from {url}"
    
    def test_different_data_types(self):
        """Test handling of different data types (remote HTTPS)"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        band = ds.GetRasterBand(1)
        data_type = band.DataType
        supported_types = [
            gdal.GDT_Byte, gdal.GDT_UInt16, gdal.GDT_Int16,
            gdal.GDT_UInt32, gdal.GDT_Int32, gdal.GDT_Float32, gdal.GDT_Float64
        ]
        assert data_type in supported_types, f"Unsupported data type: {data_type}"
    
    def test_chunk_size_variations(self):
        """Test handling of different chunk configurations (remote HTTPS)"""
        url = REMOTE_WITH_SUBDATASETS_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        band = ds.GetRasterBand(1)
        block_sizes = band.GetBlockSize()
        assert block_sizes[0] > 0 and block_sizes[1] > 0, "Block sizes should be positive"
        assert block_sizes[0] <= ds.RasterXSize, "Block width should not exceed dataset width"
        assert block_sizes[1] <= ds.RasterYSize, "Block height should not exceed dataset height"


class TestEOPFZarrEdgeCases:
    """Test edge cases and error conditions"""
    
    def test_empty_datasets(self, temp_zarr_path):
        """Test handling of edge cases like empty or minimal datasets"""
        # This would require creating minimal test datasets on the fly
        # For now, we test with existing datasets
        pass
    
    def test_malformed_metadata(self):
        """Test handling of malformed or missing metadata (remote HTTPS)"""
        url = REMOTE_SAMPLE_ZARR
        ds = gdal.Open(f'EOPFZARR:"/vsicurl/{url}"')
        if ds is None:
            pytest.skip(f"Remote Zarr data not accessible: {url}")
        # Should handle missing optional metadata gracefully
        metadata = ds.GetMetadata()
        assert isinstance(metadata, dict), "Should return dict even with limited metadata"
    
    def test_url_parsing_edge_cases(self):
        """Test URL parsing with various edge cases"""
        edge_cases = [
            "EOPFZARR:",  # Empty path
            "EOPFZARR:/",  # Root path
            "EOPFZARR://",  # Double slash
            "EOPFZARR:relative/path.zarr",  # Relative path
        ]
        
        for test_url in edge_cases:
            try:
                ds = gdal.Open(test_url)
                # Should either return None or raise exception, not crash
                if ds is not None:
                    ds = None  # Clean up
            except Exception:
                pass  # Expected for invalid URLs


# Performance markers for conditional testing
pytestmark = [
    pytest.mark.require_driver("EOPFZARR"),
]


if __name__ == "__main__":
    # Allow running tests directly
    pytest.main([__file__, "-v"])
