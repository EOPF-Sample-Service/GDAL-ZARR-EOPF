#!/usr/bin/env python3
"""
Quick path parsing validation for EOPF-Zarr driver.

This script tests path parsing without trying to open actual datasets,
focusing on the URL parsing logic fixes.
"""

import sys

try:
    from osgeo import gdal
    gdal.UseExceptions()
except ImportError:
    print("❌ GDAL Python bindings not available")
    sys.exit(1)


def test_path_parsing_with_debug(path, description):
    """Test path parsing and show debug output."""
    print(f"\n--- {description} ---")
    print(f"Testing: EOPFZARR:{path}")
    
    # Enable detailed debug logging
    gdal.SetConfigOption("CPL_DEBUG", "EOPFZARR")
    
    try:
        # This will trigger the path parsing logic
        ds = gdal.Open(f"EOPFZARR:{path}")
        
        if ds is not None:
            print("✅ Successfully parsed and attempted to open")
            ds = None
        else:
            print("⚠️ Parsed but failed to open (expected for non-existent paths)")
            
    except Exception as e:
        print(f"❌ Exception during parsing: {e}")
    
    print("-" * 50)


def main():
    """Test critical path parsing scenarios."""
    print("🔍 EOPF-Zarr Path Parsing Validation")
    print("="*60)
    
    # Check driver availability
    driver = gdal.GetDriverByName("EOPFZARR")
    if driver is None:
        print("❌ EOPF-Zarr driver not available")
        return 1
    
    print("✅ EOPF-Zarr driver available")
    print(f"   Long name: {driver.GetMetadataItem('DMD_LONGNAME')}")
    
    # Test the critical cases that were fixed
    test_cases = [
        # HTTPS URLs (should NOT split at protocol colon)
        ("https://example.com/data.zarr", "HTTPS URL - should not split at protocol colon"),
        ("https://objects.eodc.eu/complex:path/data.zarr", "HTTPS URL with colon in path"),
        ("http://example.com/data.zarr", "HTTP URL - should not split at protocol colon"),
        
        # Local paths with subdatasets (SHOULD split at subdataset colon)
        ("/local/path/data.zarr:band1", "Local path with subdataset - should split"),
        ("C:/data/test.zarr:band1", "Windows path with subdataset - should split"),
        
        # VSI paths
        ("/vsicurl/https://example.com/data.zarr", "VSI with HTTPS URL"),
        ("/vsis3/bucket/data.zarr", "VSI S3 path"),
        
        # Edge cases
        ("https://", "Incomplete HTTPS URL"),
        (":", "Single colon"),
        ("", "Empty path"),
        
        # Real EODC URL
        ("https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr", 
         "Real EODC URL with complex path and colons"),
    ]
    
    for path, description in test_cases:
        test_path_parsing_with_debug(path, description)
    
    # Clean up
    gdal.SetConfigOption("CPL_DEBUG", None)
    
    print("\n🎯 Key things to look for in the debug output:")
    print("• HTTPS URLs should show 'Path is URL/Virtual: YES'")
    print("• HTTPS URLs should NOT be split at the protocol colon") 
    print("• Local paths with subdatasets SHOULD be split at the colon")
    print("• The main path should be the complete URL for HTTPS")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
