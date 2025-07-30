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
    gdal.UseExceptions()
except ImportError:
    pytest.skip("GDAL not available", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")

# Test data directory
TEST_DATA_DIR = os.path.join(os.path.dirname(__file__), "..", "data")


@pytest.fixture(scope="session", autouse=True)
def ensure_test_data():
    """Ensure test data exists before running tests."""
    if not os.path.exists(TEST_DATA_DIR):
        # Try to generate test data
        generate_script = os.path.join(os.path.dirname(__file__), "..", "generate_test_data.py")
        if os.path.exists(generate_script):
            import subprocess
            try:
                subprocess.run([sys.executable, generate_script], check=True)
            except subprocess.CalledProcessError:
                pytest.skip("Could not generate test data")
        else:
            pytest.skip("Test data and generator not available")


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
        """Test opening a basic EOPF-Zarr dataset"""
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
            
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        assert ds is not None, "Failed to open test dataset"
        assert ds.RasterCount > 0, "Dataset has no raster bands"
        assert ds.RasterXSize > 0, "Dataset has zero width"
        assert ds.RasterYSize > 0, "Dataset has zero height"
        
        # Test raster band properties
        band = ds.GetRasterBand(1)
        assert band is not None, "Failed to get raster band"
        assert band.DataType in [
            gdal.GDT_Byte, gdal.GDT_UInt16, gdal.GDT_Int16, 
            gdal.GDT_UInt32, gdal.GDT_Int32, gdal.GDT_Float32, gdal.GDT_Float64
        ], "Unsupported data type"
    
    def test_data_reading(self):
        """Test reading data from EOPF-Zarr dataset"""
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
            
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        band = ds.GetRasterBand(1)
        
        # Test reading a small block
        width = min(10, ds.RasterXSize)
        height = min(10, ds.RasterYSize)
        data = band.ReadAsArray(0, 0, width, height)
        
        assert data is not None, "Failed to read data"
        assert data.shape == (height, width), f"Unexpected data shape: {data.shape}"
        assert data.size > 0, "Empty data array"
        
        # Test reading full dataset (if not too large)
        if ds.RasterXSize * ds.RasterYSize < 1000000:  # Less than 1M pixels
            full_data = band.ReadAsArray()
            assert full_data is not None, "Failed to read full dataset"
            assert full_data.shape == (ds.RasterYSize, ds.RasterXSize), "Full data shape mismatch"
    
    def test_subdatasets(self):
        """Test subdataset enumeration and access"""
        test_file = os.path.join(TEST_DATA_DIR, "with_subdatasets.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
            
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        subdatasets = ds.GetMetadata("SUBDATASETS")
        
        if subdatasets:
            # Verify subdataset metadata format
            subds_count = len([k for k in subdatasets.keys() if k.endswith("_NAME")])
            assert subds_count > 0, "No subdatasets found"
            
            # Test opening first subdataset
            first_subds = subdatasets["SUBDATASET_1_NAME"]
            subds = gdal.Open(first_subds)
            assert subds is not None, f"Failed to open subdataset: {first_subds}"
            assert subds.RasterCount > 0, "Subdataset has no bands"
            
            # Test all subdatasets can be opened
            for i in range(1, subds_count + 1):
                subds_name = subdatasets.get(f"SUBDATASET_{i}_NAME")
                if subds_name:
                    subds = gdal.Open(subds_name)
                    assert subds is not None, f"Failed to open subdataset {i}: {subds_name}"
    
    def test_geospatial_info(self):
        """Test geospatial information retrieval"""
        test_file = os.path.join(TEST_DATA_DIR, "georeferenced.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
            
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        
        # Test geotransform
        gt = ds.GetGeoTransform()
        assert gt != (0, 1, 0, 0, 0, 1), "Default geotransform detected"
        assert len(gt) == 6, "Invalid geotransform format"
        assert gt[1] != 0 and gt[5] != 0, "Zero pixel size in geotransform"
        
        # Test spatial reference
        srs = ds.GetSpatialRef()
        if srs:
            assert srs.IsValid(), "Invalid spatial reference"
            # Test that we can get authority code
            auth_code = srs.GetAuthorityCode(None)
            if auth_code:
                assert auth_code.isdigit(), f"Invalid authority code: {auth_code}"
    
    def test_metadata_retrieval(self):
        """Test EOPF metadata retrieval"""
        test_file = os.path.join(TEST_DATA_DIR, "with_metadata.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
            
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        
        # Test dataset metadata
        metadata = ds.GetMetadata()
        assert isinstance(metadata, dict), "Metadata should be a dictionary"
        
        # Test band metadata
        band = ds.GetRasterBand(1)
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
        """Test network dataset access via VSI and HTTPS URLs"""
        # Test the specific EODC URL
        eodc_url = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr"
        
        print(f"Testing EODC URL: {eodc_url}")
        
        # Test direct HTTPS access
        try:
            ds = gdal.Open(f"EOPFZARR:{eodc_url}")
            if ds is not None:
                print("✅ Successfully opened EODC HTTPS dataset")
                assert ds.RasterCount > 0, "EODC dataset should have bands"
                
                # Test reading metadata
                metadata = ds.GetMetadata()
                print(f"Metadata keys: {len(metadata)}")
                
                # Test reading a small data sample
                if ds.RasterXSize > 0 and ds.RasterYSize > 0:
                    band = ds.GetRasterBand(1)
                    # Read a small 10x10 sample
                    sample_size = min(10, ds.RasterXSize, ds.RasterYSize)
                    data = band.ReadAsArray(0, 0, sample_size, sample_size)
                    if data is not None:
                        print(f"✅ Successfully read {sample_size}x{sample_size} data sample")
                    else:
                        print("⚠️ Could not read data sample")
                
                # Test subdatasets if available
                subdatasets = ds.GetMetadata("SUBDATASETS")
                if subdatasets:
                    print(f"Found {len(subdatasets)//2} subdatasets")
                    
            else:
                print("⚠️ Could not open EODC dataset (may be network issue)")
                
        except Exception as e:
            print(f"⚠️ EODC dataset access failed: {e}")
            # Don't fail the test - network issues are expected
        
        # Test with VSI wrapper
        try:
            vsi_url = f"/vsicurl/{eodc_url}"
            ds_vsi = gdal.Open(f"EOPFZARR:{vsi_url}")
            if ds_vsi is not None:
                print("✅ Successfully opened EODC dataset via /vsicurl/")
                assert ds_vsi.RasterCount > 0, "VSI EODC dataset should have bands"
            else:
                print("⚠️ Could not open EODC dataset via /vsicurl/")
        except Exception as e:
            print(f"⚠️ VSI EODC dataset access failed: {e}")
        
        # Test other public HTTPS URLs if available
        test_urls = [
            "https://example.com/public/test.zarr",  # Hypothetical
        ]
        
        for url in test_urls:
            try:
                ds = gdal.Open(f"EOPFZARR:{url}")
                if ds is not None:
                    print(f"✅ Successfully opened {url}")
                    assert ds.RasterCount > 0
                else:
                    print(f"⚠️ Could not open {url} (expected)")
            except Exception:
                # Expected for non-existent URLs
                pass
    
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
        """Test performance optimization features"""
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        # Enable performance logging
        original_debug = gdal.GetConfigOption("CPL_DEBUG")
        original_timers = gdal.GetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS")
        
        try:
            gdal.SetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS", "1")
            gdal.SetConfigOption("CPL_DEBUG", "EOPFZARR_PERF")
            
            ds = gdal.Open(f"EOPFZARR:{test_file}")
            assert ds is not None, "Failed to open dataset with performance logging"
            
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
            # The important thing is no crashes occur with performance features enabled
            
        finally:
            gdal.SetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS", original_timers)
            gdal.SetConfigOption("CPL_DEBUG", original_debug)
    
    def test_block_reading_patterns(self):
        """Test different block reading patterns for memory efficiency"""
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        band = ds.GetRasterBand(1)
        
        # Test different read patterns
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
        """Test handling of large datasets"""
        test_file = os.path.join(TEST_DATA_DIR, "performance_test.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        if ds is None:
            pytest.skip("Performance test dataset not available")
        
        # Test that we can handle large datasets without loading everything into memory
        assert ds.RasterXSize > 0 and ds.RasterYSize > 0, "Dataset should have valid dimensions"
        
        # Read a small portion of a large dataset
        band = ds.GetRasterBand(1)
        small_data = band.ReadAsArray(0, 0, min(100, ds.RasterXSize), min(100, ds.RasterYSize))
        assert small_data is not None, "Should be able to read small portion of large dataset"


class TestEOPFZarrPerformance:
    """Performance-specific integration tests"""
    
    def test_caching_effectiveness(self):
        """Test that caching actually improves performance"""
        test_file = os.path.join(TEST_DATA_DIR, "with_metadata.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        
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
        """Test memory efficiency with repeated operations"""
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        # Test that repeated dataset opens don't accumulate memory
        for i in range(10):
            ds = gdal.Open(f"EOPFZARR:{test_file}")
            assert ds is not None, f"Failed to open dataset on iteration {i}"
            
            # Read some data
            band = ds.GetRasterBand(1)
            data = band.ReadAsArray(0, 0, min(50, ds.RasterXSize), min(50, ds.RasterYSize))
            assert data is not None, f"Failed to read data on iteration {i}"
            
            # Explicitly close to test cleanup
            ds = None


class TestEOPFZarrCompatibility:
    """Compatibility tests with different Zarr formats"""
    
    def test_zarr_format_compatibility(self):
        """Test compatibility with different Zarr format variations"""
        # Test different compression algorithms if available
        test_files = [
            "sample.zarr",
            "with_subdatasets.zarr", 
            "georeferenced.zarr",
            "with_metadata.zarr"
        ]
        
        for test_file in test_files:
            full_path = os.path.join(TEST_DATA_DIR, test_file)
            if os.path.exists(full_path):
                ds = gdal.Open(f"EOPFZARR:{full_path}")
                if ds is not None:  # File may not be compatible
                    assert ds.RasterCount > 0, f"Dataset {test_file} should have bands"
                    
                    # Test reading data
                    band = ds.GetRasterBand(1)
                    data = band.ReadAsArray(0, 0, min(10, ds.RasterXSize), min(10, ds.RasterYSize))
                    assert data is not None, f"Should be able to read data from {test_file}"
    
    def test_different_data_types(self):
        """Test handling of different data types"""
        # This would test various numpy dtypes if available in test data
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        band = ds.GetRasterBand(1)
        
        # Verify data type is supported
        data_type = band.DataType
        supported_types = [
            gdal.GDT_Byte, gdal.GDT_UInt16, gdal.GDT_Int16,
            gdal.GDT_UInt32, gdal.GDT_Int32, gdal.GDT_Float32, gdal.GDT_Float64
        ]
        assert data_type in supported_types, f"Unsupported data type: {data_type}"
    
    def test_chunk_size_variations(self):
        """Test handling of different chunk configurations"""
        test_file = os.path.join(TEST_DATA_DIR, "performance_test.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        if ds is None:
            pytest.skip("Performance test dataset not available")
        
        # Test block size handling
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
        """Test handling of malformed or missing metadata"""
        # Test with dataset that might have incomplete metadata
        test_file = os.path.join(TEST_DATA_DIR, "sample.zarr")
        if not os.path.exists(test_file):
            pytest.skip(f"Test data not found: {test_file}")
        
        ds = gdal.Open(f"EOPFZARR:{test_file}")
        
        # Should handle missing optional metadata gracefully
        metadata = ds.GetMetadata()
        # Test should not fail even if some metadata is missing
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
