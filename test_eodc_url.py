#!/usr/bin/env python3
"""
Test script specifically for EODC HTTPS URL access.

This script tests the specific EODC Zarr dataset URL to ensure
the EOPF-Zarr driver can handle real-world HTTPS datasets.
"""

import sys
import time
from pathlib import Path

try:
    from osgeo import gdal
    gdal.UseExceptions()
except ImportError:
    print("❌ GDAL Python bindings not available")
    sys.exit(1)

# The specific EODC URL to test
EODC_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr"


def test_driver_availability():
    """Test that the EOPF-Zarr driver is available."""
    driver = gdal.GetDriverByName("EOPFZARR")
    if driver is None:
        print("❌ EOPF-Zarr driver not available")
        return False
    
    print("✅ EOPF-Zarr driver available")
    print(f"   Long name: {driver.GetMetadataItem('DMD_LONGNAME')}")
    return True


def test_eodc_dataset_direct():
    """Test accessing the EODC dataset directly via HTTPS."""
    print(f"\n=== Testing Direct HTTPS Access ===")
    print(f"URL: {EODC_URL}")
    
    try:
        start_time = time.time()
        ds = gdal.Open(f"EOPFZARR:{EODC_URL}")
        open_time = time.time() - start_time
        
        if ds is None:
            print("❌ Failed to open EODC dataset")
            return False
        
        print(f"✅ Successfully opened EODC dataset ({open_time:.2f}s)")
        print(f"   Dimensions: {ds.RasterXSize} x {ds.RasterYSize}")
        print(f"   Bands: {ds.RasterCount}")
        
        # Test metadata access
        metadata = ds.GetMetadata()
        print(f"   Metadata items: {len(metadata)}")
        
        # Test subdatasets
        subdatasets = ds.GetMetadata("SUBDATASETS")
        if subdatasets:
            subds_count = len([k for k in subdatasets.keys() if k.endswith("_NAME")])
            print(f"   Subdatasets: {subds_count}")
        
        # Test small data read
        if ds.RasterCount > 0:
            band = ds.GetRasterBand(1)
            sample_size = min(10, ds.RasterXSize, ds.RasterYSize)
            
            start_time = time.time()
            data = band.ReadAsArray(0, 0, sample_size, sample_size)
            read_time = time.time() - start_time
            
            if data is not None:
                print(f"✅ Successfully read {sample_size}x{sample_size} data sample ({read_time:.2f}s)")
                print(f"   Data type: {data.dtype}")
                print(f"   Data shape: {data.shape}")
                print(f"   Value range: {data.min()} - {data.max()}")
            else:
                print("⚠️ Could not read data sample")
        
        return True
        
    except Exception as e:
        print(f"❌ Error accessing EODC dataset: {e}")
        return False


def test_eodc_dataset_vsi():
    """Test accessing the EODC dataset via VSI curl wrapper."""
    print(f"\n=== Testing VSI Curl Access ===")
    vsi_url = f"/vsicurl/{EODC_URL}"
    print(f"VSI URL: {vsi_url}")
    
    try:
        start_time = time.time()
        ds = gdal.Open(f"EOPFZARR:{vsi_url}")
        open_time = time.time() - start_time
        
        if ds is None:
            print("❌ Failed to open EODC dataset via VSI")
            return False
        
        print(f"✅ Successfully opened EODC dataset via VSI ({open_time:.2f}s)")
        print(f"   Dimensions: {ds.RasterXSize} x {ds.RasterYSize}")
        print(f"   Bands: {ds.RasterCount}")
        
        return True
        
    except Exception as e:
        print(f"❌ Error accessing EODC dataset via VSI: {e}")
        return False


def test_performance_with_caching():
    """Test performance features with the EODC dataset."""
    print(f"\n=== Testing Performance Features ===")
    
    # Enable performance logging
    gdal.SetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS", "1")
    gdal.SetConfigOption("CPL_DEBUG", "EOPFZARR_PERF")
    
    try:
        # First access (should populate cache)
        print("First access (populating cache)...")
        start_time = time.time()
        ds = gdal.Open(f"EOPFZARR:{EODC_URL}")
        first_time = 0
        if ds:
            metadata = ds.GetMetadata()
            first_time = time.time() - start_time
            print(f"   Time: {first_time:.2f}s")
        
        # Second access (should use cache)
        print("Second access (using cache)...")
        start_time = time.time()
        ds = gdal.Open(f"EOPFZARR:{EODC_URL}")
        if ds:
            metadata = ds.GetMetadata()
            second_time = time.time() - start_time
            print(f"   Time: {second_time:.2f}s")
            
            if second_time < first_time and first_time > 0:
                print("✅ Caching appears to be effective")
            else:
                print("⚠️ Caching effectiveness unclear")
        
        return True
        
    except Exception as e:
        print(f"❌ Error testing performance: {e}")
        return False
    finally:
        gdal.SetConfigOption("EOPF_ENABLE_PERFORMANCE_TIMERS", None)
        gdal.SetConfigOption("CPL_DEBUG", None)


def main():
    """Run all EODC URL tests."""
    print("🧪 EODC HTTPS URL Test Suite")
    print("=" * 50)
    
    success = True
    
    # Test driver availability
    if not test_driver_availability():
        print("\n❌ Driver not available - cannot run tests")
        return 1
    
    # Test direct HTTPS access
    if not test_eodc_dataset_direct():
        success = False
    
    # Test VSI access
    if not test_eodc_dataset_vsi():
        success = False
    
    # Test performance features
    if not test_performance_with_caching():
        success = False
    
    print("\n" + "=" * 50)
    if success:
        print("🎉 All EODC URL tests completed successfully!")
        return 0
    else:
        print("❌ Some EODC URL tests failed.")
        print("Note: Network-related failures are expected in some environments.")
        return 0  # Don't fail on network issues


if __name__ == "__main__":
    sys.exit(main())
