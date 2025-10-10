#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
"""
Simple rioxarray integration tests using the same pattern as rasterio tests.
No fancy fixtures - just the exact pattern that works for rasterio.
"""

import os
import sys
import pytest
import numpy as np

try:
    from osgeo import gdal
    import rasterio
    import rasterio.env
    import rioxarray
    import xarray as xr
    gdal.UseExceptions()
except ImportError as e:
    pytest.skip(f"Required packages not available: {e}", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")

# Test URLs - split into base + subdataset
REMOTE_OLCI_BASE = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202508-s03olcefr/19/products/cpm_v256/S3B_OL_1_EFR____20250819T074058_20250819T074358_20250819T092155_0179_110_106_3420_ESA_O_NR_004.zarr"
REMOTE_OLCI_SUBDATASET = "measurements/oa01_radiance"


def create_rasterio_context():
    """Create rasterio environment context - EXACTLY like rasterio tests"""
    config = {
        'GDAL_DRIVER_PATH': os.environ.get('GDAL_DRIVER_PATH', ''),
    }
    return rasterio.env.Env(**config)


def make_eopfzarr_path(base_url, subdataset=""):
    """Construct proper EOPFZARR path"""
    if subdataset:
        return f'EOPFZARR:"/vsicurl/{base_url}":{subdataset}'
    else:
        return f'EOPFZARR:"/vsicurl/{base_url}"'


def check_url_accessible(base_url, subdataset=""):
    """Check if URL is accessible"""
    try:
        with create_rasterio_context():
            path = make_eopfzarr_path(base_url, subdataset)
            ds = gdal.Open(path)
            accessible = ds is not None
            if ds:
                ds = None
            return accessible
    except Exception:
        return False


def skip_if_url_not_accessible(base_url, subdataset="", test_name=""):
    """Skip test if URL is not accessible"""
    if not check_url_accessible(base_url, subdataset):
        full_url = f"{base_url}/{subdataset}" if subdataset else base_url
        pytest.skip(f"Remote Zarr data not accessible for {test_name}: {full_url}")


class TestRioxarrayBasicOperations:
    """Test basic rioxarray operations"""
    
    def test_rioxarray_open_rasterio_basic(self):
        """Test basic rioxarray.open_rasterio() with EOPFZARR dataset"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray basic open")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        # USE THE CONTEXT - EXACTLY LIKE RASTERIO TESTS
        with create_rasterio_context():
            try:
                da = rioxarray.open_rasterio(path)
                
                assert isinstance(da, xr.DataArray), "Should return xarray DataArray"
                assert 'band' in da.dims or 'y' in da.dims or 'x' in da.dims
                assert da.shape is not None
                assert len(da.shape) > 0
                
                print(f"✅ Successfully opened dataset with rioxarray")
                print(f"   Dimensions: {da.dims}")
                print(f"   Shape: {da.shape}")
                
            except Exception as e:
                pytest.fail(f"Failed to open with rioxarray: {e}")
    
    def test_rioxarray_data_array_dimensions(self):
        """Test that rioxarray DataArray has correct dimensions"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray dimensions")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            assert 'y' in da.dims, "Should have 'y' dimension"
            assert 'x' in da.dims, "Should have 'x' dimension"
            
            print(f"✅ Dimensions are correct: {da.dims}")
    
    def test_rioxarray_coordinates(self):
        """Test that coordinate systems are properly set up"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray coordinates")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            assert 'x' in da.coords
            assert 'y' in da.coords
            
            x_coords = da.coords['x']
            y_coords = da.coords['y']
            
            assert len(x_coords) > 0
            assert len(y_coords) > 0
            
            print(f"✅ Coordinates OK: x={len(x_coords)}, y={len(y_coords)}")
    
    def test_rioxarray_attributes(self):
        """Test that DataArray has expected attributes"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray attributes")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            assert hasattr(da, 'attrs')
            assert hasattr(da, 'dims')
            assert hasattr(da, 'coords')
            assert hasattr(da, 'shape')
            assert hasattr(da, 'dtype')
            
            print(f"✅ All required attributes present")


class TestRioxarryCRSAndSpatialReference:
    """Test CRS and spatial reference handling"""
    
    def test_rioxarray_crs_access(self):
        """Test accessing CRS information"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray CRS")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            assert hasattr(da, 'rio')
            crs = da.rio.crs
            
            print(f"✅ CRS accessible: {crs}")
    
    def test_rioxarray_transform(self):
        """Test accessing affine transform"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray transform")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            transform = da.rio.transform()
            assert transform is not None
            
            print(f"✅ Transform accessible: {transform}")
    
    def test_rioxarray_bounds(self):
        """Test accessing bounds"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray bounds")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            bounds = da.rio.bounds()
            assert bounds is not None
            assert len(bounds) == 4
            
            print(f"✅ Bounds: {bounds}")
    
    def test_rioxarray_resolution(self):
        """Test accessing resolution"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray resolution")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            resolution = da.rio.resolution()
            assert resolution is not None
            assert len(resolution) == 2
            
            print(f"✅ Resolution: {resolution}")


class TestRioxarrayDataAccess:
    """Test data access and reading"""
    
    def test_rioxarray_data_reading(self):
        """Test reading actual data values"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray data reading")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            # Read a small subset
            subset = da.isel(x=slice(0, 10), y=slice(0, 10))
            
            # Trigger computation if dask array
            if hasattr(subset.data, 'compute'):
                data = subset.compute()
            else:
                data = subset
            
            assert data.shape[0] <= 10  # May be smaller if dataset is small
            
            print(f"✅ Successfully read data subset: {data.shape}")
    
    def test_rioxarray_data_types(self):
        """Test data type handling"""
        skip_if_url_not_accessible(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET, "rioxarray data types")
        
        path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
        
        with create_rasterio_context():
            da = rioxarray.open_rasterio(path)
            
            assert da.dtype is not None
            print(f"✅ Data type: {da.dtype}")


def test_manual_rioxarray_integration():
    """Manual test for development/debugging"""
    print("\n" + "="*60)
    print("Manual rioxarray Integration Test")
    print("="*60)
    
    path = make_eopfzarr_path(REMOTE_OLCI_BASE, REMOTE_OLCI_SUBDATASET)
    
    try:
        print(f"\n1. Testing: {REMOTE_OLCI_BASE}")
        print(f"   Subdataset: {REMOTE_OLCI_SUBDATASET}")
        print(f"   Path: {path}")
        
        with create_rasterio_context():
            # Test GDAL first
            print("\n2. Testing GDAL access...")
            ds = gdal.Open(path)
            if ds:
                print(f"   ✅ GDAL: {ds.RasterXSize}x{ds.RasterYSize}, {ds.RasterCount} bands")
                ds = None
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
    pytest.main([__file__, "-v", "-s"])
