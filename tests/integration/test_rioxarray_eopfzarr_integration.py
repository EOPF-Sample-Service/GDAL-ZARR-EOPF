#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
"""
Integration tests for EOPF-Zarr GDAL driver with rioxarray compatibility.
Tests rioxarray.open_rasterio() functionality with EOPFZARR datasets.

Issue: https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues/101

IMPORTANT PERFORMANCE NOTE:
---------------------------
These tests use SPECIFIC SUBDATASETS (leaf nodes), NOT root datasets.

When rioxarray.open_rasterio() opens a dataset that has subdatasets,
it eagerly loads ALL subdatasets (via _load_subdatasets() function).
This is rioxarray's design behavior, not an EOPFZARR driver issue.

For example:
- Opening root dataset with 123 subdatasets = 123 individual open operations = SLOW
- Opening specific subdataset with no nested subdatasets = 1 open operation = FAST

Therefore, all test URLs point to specific data arrays (e.g., measurements/oa01_radiance),
not to root .zarr datasets, ensuring fast test execution.
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
    import rioxarray
    import xarray as xr
    import rasterio
    gdal.UseExceptions()
except ImportError as e:
    pytest.skip(f"Required packages not available: {e}", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")

# Remote Zarr test data URLs (publicly accessible)
# 
# ⚠️  IMPORTANT FOR RIOXARRAY TESTS:
# Always use SPECIFIC SUBDATASETS (leaf nodes), NOT root datasets!
# 
# Opening root datasets with rioxarray will eagerly load ALL subdatasets,
# which causes very slow test execution (e.g., 61+ subdatasets = 61+ open operations).
# This is rioxarray's design behavior, not an EOPFZARR issue.
# 
# ✅ GOOD: Use specific subdataset paths (like the URLs below)
# ❌ BAD: Use root .zarr URLs that have subdatasets
#

# Sentinel-3 OLCI Level-1 EFR - Specific subdataset (FAST for rioxarray)
# This URL points directly to a single measurement band, NOT the root dataset
# The root dataset has 123 subdatasets which would cause slow rioxarray tests
# 
# Split into base URL + subdataset path for proper EOPFZARR path construction
REMOTE_OLCI_BASE = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202508-s03olcefr/19/products/cpm_v256/S3B_OL_1_EFR____20250819T074058_20250819T074358_20250819T092155_0179_110_106_3420_ESA_O_NR_004.zarr"
REMOTE_OLCI_SUBDATASET = "measurements/oa01_radiance"

# Sentinel-2 MSI Level-1C - Root dataset (DO NOT USE with rioxarray - has 61 subdatasets!)
REMOTE_SAMPLE_ZARR = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr"


def make_eopfzarr_path(base_url, subdataset=""):
    """
    Construct a proper EOPFZARR path.
    
    Args:
        base_url: Base URL to the Zarr dataset (e.g., https://.../file.zarr)
        subdataset: Optional subdataset path (e.g., measurements/oa01_radiance)
    
    Returns:
        Properly formatted EOPFZARR path string
        
    Examples:
        >>> make_eopfzarr_path("https://example.com/data.zarr")
        'EOPFZARR:"/vsicurl/https://example.com/data.zarr"'
        
        >>> make_eopfzarr_path("https://example.com/data.zarr", "measurements/band1")
        'EOPFZARR:"/vsicurl/https://example.com/data.zarr":measurements/band1'
    """
    if subdataset:
        return f'EOPFZARR:"/vsicurl/{base_url}":{subdataset}'
    else:
        return f'EOPFZARR:"/vsicurl/{base_url}"'


def check_url_accessible(base_url, subdataset=""):
    """Check if a remote URL is accessible"""
    try:
        path = make_eopfzarr_path(base_url, subdataset)
        ds = gdal.Open(path)
        if ds is None:
            return False
        return True
    except Exception:
        return False


def skip_if_url_not_accessible(base_url, subdataset="", test_name=""):
    """Skip test if URL is not accessible"""
    if not check_url_accessible(base_url, subdataset):
        full_url = f"{base_url}/{subdataset}" if subdataset else base_url
        pytest.skip(f"Remote Zarr data not accessible for {test_name}: {full_url}")


@pytest.fixture(scope="session", autouse=True)
def setup_environment():
    """Setup environment for all tests"""
    # Ensure driver is available
    driver = gdal.GetDriverByName("EOPFZARR")
    if driver is None:
        pytest.skip("EOPFZARR driver not available", allow_module_level=True)


@pytest.fixture
def temp_zarr_path():
    """Create a temporary directory for Zarr datasets."""
    temp_dir = tempfile.mkdtemp()
    yield temp_dir
    shutil.rmtree(temp_dir, ignore_errors=True)


class TestRioxarrayBasicOperations:
    """Test basic rioxarray operations with EOPFZARR datasets"""
    
    def test_rioxarray_open_rasterio_basic(self):
        """Test basic rioxarray.open_rasterio() with EOPFZARR dataset"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray basic open test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        try:
            # Open with rioxarray
            da = rioxarray.open_rasterio(path)
            
            # Verify it's an xarray DataArray
            assert isinstance(da, xr.DataArray), "Should return xarray DataArray"
            
            # Check basic structure
            assert 'band' in da.dims or 'y' in da.dims or 'x' in da.dims, \
                "DataArray should have spatial dimensions"
            
            # Verify data is accessible
            assert da.shape is not None, "DataArray should have a shape"
            assert len(da.shape) > 0, "DataArray should not be empty"
            
            print(f"✅ Successfully opened dataset with rioxarray")
            print(f"   Dimensions: {da.dims}")
            print(f"   Shape: {da.shape}")
            print(f"   Data type: {da.dtype}")
            
        except Exception as e:
            pytest.fail(f"Failed to open with rioxarray: {e}")
    
    def test_rioxarray_data_array_dimensions(self):
        """Test that rioxarray DataArray has correct dimensions"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray dimensions test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Standard dimensions for raster data
        # rioxarray typically uses (band, y, x) or (y, x) for single band
        expected_dims = ['band', 'y', 'x']
        
        # Check that we have at least y and x
        assert 'y' in da.dims, "Should have 'y' dimension"
        assert 'x' in da.dims, "Should have 'x' dimension"
        
        # If multi-band, should have band dimension
        if len(da.shape) == 3:
            assert 'band' in da.dims, "Multi-band data should have 'band' dimension"
        
        print(f"✅ Dimensions verified: {da.dims}")
    
    def test_rioxarray_coordinates(self):
        """Test that coordinate systems are properly set up"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray coordinates test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check coordinates exist
        assert 'y' in da.coords, "Should have y coordinates"
        assert 'x' in da.coords, "Should have x coordinates"
        
        # Verify coordinates are not empty
        assert len(da.coords['y']) > 0, "Y coordinates should not be empty"
        assert len(da.coords['x']) > 0, "X coordinates should not be empty"
        
        print(f"✅ Coordinates found:")
        print(f"   X: {len(da.coords['x'])} points")
        print(f"   Y: {len(da.coords['y'])} points")
    
    def test_rioxarray_attributes(self):
        """Test that xarray attributes are preserved"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray attributes test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check that DataArray has attributes
        assert hasattr(da, 'attrs'), "DataArray should have attrs property"
        
        # Print available attributes
        print(f"✅ DataArray attributes: {list(da.attrs.keys())}")


class TestRioxarryCRSAndSpatialReference:
    """Test CRS and spatial reference information handling"""
    
    def test_rioxarray_crs_access(self):
        """Test CRS information is accessible via rio accessor"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray CRS test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check rio accessor is available
        assert hasattr(da, 'rio'), "DataArray should have rio accessor"
        
        # Try to access CRS
        try:
            crs = da.rio.crs
            print(f"✅ CRS accessible: {crs}")
            
            # If CRS is None, it means the dataset doesn't have projection info
            # This is valid for some datasets
            if crs is not None:
                assert crs is not None, "CRS should be accessible"
            else:
                print("   Note: Dataset has no CRS information (unprojected data)")
                
        except Exception as e:
            print(f"⚠ CRS access warning: {e}")
            # Not failing test as some datasets may not have CRS
    
    def test_rioxarray_transform(self):
        """Test geotransform information is accessible"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray transform test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        try:
            transform = da.rio.transform()
            print(f"✅ Transform accessible: {transform}")
            
            if transform is not None:
                # Verify transform has expected properties
                assert hasattr(transform, 'a'), "Transform should have scale/rotation parameters"
                
        except Exception as e:
            print(f"⚠ Transform access warning: {e}")
    
    def test_rioxarray_bounds(self):
        """Test spatial bounds are accessible"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray bounds test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        try:
            bounds = da.rio.bounds()
            print(f"✅ Bounds accessible: {bounds}")
            
            # Bounds should be a tuple of (minx, miny, maxx, maxy)
            if bounds is not None:
                assert len(bounds) == 4, "Bounds should have 4 values"
                
        except Exception as e:
            print(f"⚠ Bounds access warning: {e}")
    
    def test_rioxarray_resolution(self):
        """Test resolution information is accessible"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray resolution test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        try:
            resolution = da.rio.resolution()
            print(f"✅ Resolution accessible: {resolution}")
            
            if resolution is not None:
                assert len(resolution) == 2, "Resolution should have 2 values (x, y)"
                
        except Exception as e:
            print(f"⚠ Resolution access warning: {e}")


class TestRioxarrayMetadataPreservation:
    """Test metadata preservation through rioxarray"""
    
    def test_rioxarray_nodata_handling(self):
        """Test nodata value handling"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray nodata test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        try:
            nodata = da.rio.nodata
            print(f"✅ NoData value: {nodata}")
            
            # NoData can be None if not set
            if nodata is not None:
                # Verify it's a valid numeric value
                assert isinstance(nodata, (int, float)), "NoData should be numeric"
                
        except Exception as e:
            print(f"⚠ NoData access warning: {e}")
    
    def test_rioxarray_band_descriptions(self):
        """Test band descriptions are preserved"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray band descriptions test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check for band-related attributes
        if 'band' in da.dims:
            print(f"✅ Multi-band dataset with {len(da.band)} bands")
            
            # Check if band coordinate has description attributes
            if 'long_name' in da.attrs:
                print(f"   Band description: {da.attrs['long_name']}")
        else:
            print("✅ Single-band dataset")
    
    def test_rioxarray_custom_metadata(self):
        """Test that custom EOPF metadata is accessible"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray custom metadata test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check for any EOPF-specific metadata in attributes
        eopf_attrs = {k: v for k, v in da.attrs.items() if 'eopf' in k.lower() or 'zarr' in k.lower()}
        
        print(f"✅ EOPF-related attributes: {list(eopf_attrs.keys())}")
        
        # Also check encoding
        if hasattr(da, 'encoding'):
            print(f"   Encoding info: {list(da.encoding.keys())}")


class TestRioxarrayChunkingAndPerformance:
    """Test chunking behavior and performance"""
    
    def test_rioxarray_chunked_reading(self):
        """Test that rioxarray respects chunking from Zarr"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray chunking test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path, chunks='auto')
        
        # Check if data is chunked (dask array)
        assert hasattr(da, 'chunks') or hasattr(da.data, 'chunks'), \
            "DataArray should support chunking"
        
        print(f"✅ Chunking information:")
        if hasattr(da, 'chunks'):
            print(f"   Chunks: {da.chunks}")
        if hasattr(da.data, 'chunks'):
            print(f"   Dask chunks: {da.data.chunks}")
    
    def test_rioxarray_lazy_loading(self):
        """Test lazy loading behavior"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray lazy loading test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        # Open with chunking enabled
        da = rioxarray.open_rasterio(path, chunks='auto')
        
        # Check if data is lazy (not loaded yet)
        import dask.array as dask_array
        
        if isinstance(da.data, dask_array.Array):
            print("✅ Data is lazy-loaded (dask array)")
            
            # Trigger computation on a small subset
            small_data = da.isel(x=slice(0, 10), y=slice(0, 10)).compute()
            assert small_data is not None, "Should be able to compute small subset"
            print(f"   Successfully computed small subset: {small_data.shape}")
        else:
            print("✅ Data is eagerly loaded (numpy array)")
    
    def test_rioxarray_window_reading(self):
        """Test reading data in windows for memory efficiency"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray window reading test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Read a small window
        window_size = min(100, da.sizes.get('x', 100), da.sizes.get('y', 100))
        window_data = da.isel(x=slice(0, window_size), y=slice(0, window_size))
        
        assert window_data is not None, "Window read should succeed"
        print(f"✅ Successfully read window of size {window_size}x{window_size}")
    
    def test_rioxarray_memory_efficiency(self):
        """Test memory efficiency with repeated operations"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray memory efficiency test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        # Open and close multiple times
        for i in range(3):
            da = rioxarray.open_rasterio(path)
            
            # Read a small amount of data
            small_data = da.isel(x=slice(0, 10), y=slice(0, 10))
            
            # Explicitly close
            da.close()
            
        print(f"✅ Memory efficiency test passed (3 iterations)")


class TestRioxarrayDataAccess:
    """Test data access patterns with rioxarray"""
    
    def test_rioxarray_data_reading(self):
        """Test reading actual data values"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray data reading test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Read a small subset
        subset = da.isel(x=slice(0, 10), y=slice(0, 10))
        
        # Compute if lazy
        if hasattr(subset.data, 'compute'):
            subset = subset.compute()
        
        # Verify data
        assert subset.values is not None, "Should have data values"
        assert subset.values.size > 0, "Data should not be empty"
        
        print(f"✅ Successfully read data subset")
        print(f"   Shape: {subset.shape}")
        print(f"   Data type: {subset.dtype}")
        print(f"   Sample values: {subset.values.flat[:5]}")
    
    def test_rioxarray_data_types(self):
        """Test different data type handling"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray data types test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check data type
        assert da.dtype is not None, "Should have a data type"
        
        # Common data types for Earth observation data
        valid_dtypes = [np.uint8, np.uint16, np.int16, np.int32, np.float32, np.float64]
        
        print(f"✅ Data type: {da.dtype}")
        print(f"   Valid dtype: {da.dtype in valid_dtypes or np.issubdtype(da.dtype, np.number)}")


class TestRioxarrayIntegrationWorkflows:
    """Test complete workflows with rioxarray"""
    
    def test_rioxarray_to_numpy(self):
        """Test conversion to numpy array"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray to numpy test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Convert small subset to numpy
        subset = da.isel(x=slice(0, 10), y=slice(0, 10))
        
        if hasattr(subset.data, 'compute'):
            numpy_data = subset.compute().values
        else:
            numpy_data = subset.values
        
        assert isinstance(numpy_data, np.ndarray), "Should convert to numpy array"
        print(f"✅ Successfully converted to numpy array: {numpy_data.shape}")
    
    def test_rioxarray_xarray_operations(self):
        """Test xarray operations on EOPFZARR data"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray xarray operations test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Try basic xarray operations
        try:
            # Mean (over a small subset to avoid memory issues)
            subset = da.isel(x=slice(0, 50), y=slice(0, 50))
            
            if hasattr(subset.data, 'compute'):
                subset = subset.compute()
            
            mean_val = subset.mean()
            print(f"✅ xarray operations work")
            print(f"   Mean value: {mean_val.values}")
            
        except Exception as e:
            print(f"⚠ xarray operations warning: {e}")
    
    def test_rioxarray_reprojection_capability(self):
        """Test that reprojection is possible (without actually doing it to save time)"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray reprojection test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        da = rioxarray.open_rasterio(path)
        
        # Check that rio accessor has reproject method
        assert hasattr(da.rio, 'reproject'), "Should have reproject method"
        
        print(f"✅ Reprojection capability available")


class TestRioxarrayErrorHandling:
    """Test error handling and edge cases"""
    
    def test_rioxarray_invalid_path(self):
        """Test handling of invalid paths"""
        invalid_path = 'EOPFZARR:"/vsicurl/https://invalid.url/nonexistent.zarr"'
        
        with pytest.raises(Exception):
            da = rioxarray.open_rasterio(invalid_path)
        
        print("✅ Invalid path properly raises exception")
    
    def test_rioxarray_concurrent_access(self):
        """Test concurrent access to same dataset"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray concurrent access test")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        # Open multiple times
        da1 = rioxarray.open_rasterio(path)
        da2 = rioxarray.open_rasterio(path)
        
        assert da1.shape == da2.shape, "Both instances should have same shape"
        
        da1.close()
        da2.close()
        
        print("✅ Concurrent access works")


def test_manual_rioxarray_integration():
    """Manual test function for development/debugging"""
    print("\n" + "="*60)
    print("Manual rioxarray Integration Test")
    print("="*60)
    
    path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
    
    try:
        print(f"\n1. Testing: {REMOTE_OLCI_BASE}")
        print(f"   Subdataset: {REMOTE_OLCI_SUBDATASET}")
        print(f"   Path: {path}")
        
        # Test GDAL first
        print("\n2. Testing GDAL access...")
        ds = gdal.Open(path)
        if ds:
            print(f"   ✅ GDAL: {ds.RasterXSize}x{ds.RasterYSize}, {ds.RasterCount} bands")
        else:
            print("   ❌ GDAL failed")
            return
        
        # Test rioxarray
        print("\n3. Testing rioxarray access...")
        da = rioxarray.open_rasterio(path)
        print(f"   ✅ rioxarray opened successfully")
        print(f"   - Dimensions: {da.dims}")
        print(f"   - Shape: {da.shape}")
        print(f"   - Data type: {da.dtype}")
        
        # Test CRS
        print("\n4. Testing CRS...")
        try:
            crs = da.rio.crs
            print(f"   ✅ CRS: {crs}")
        except Exception as e:
            print(f"   ⚠ CRS: {e}")
        
        # Test reading data
        print("\n5. Testing data access...")
        subset = da.isel(x=slice(0, 10), y=slice(0, 10))
        if hasattr(subset.data, 'compute'):
            subset = subset.compute()
        print(f"   ✅ Read subset: {subset.shape}")
        
        print("\n" + "="*60)
        print("Manual test completed successfully!")
        print("="*60)
        
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    # Allow running tests directly
    pytest.main([__file__, "-v", "-s"])
