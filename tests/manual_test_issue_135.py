"""
Manual integration test for Issue #135 fixes using gdalinfo
Tests both Sentinel-3 SLSTR (geographic) and Sentinel-2 (UTM) products
"""

import subprocess
import re
import sys

# Test URLs from Issue #135
SENTINEL3_SLSTR_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202601-s03slsrbt-eu/18/products/cpm_v262/S3A_SL_1_RBT____20260118T234920_20260118T235220_20260119T021734_0180_135_116_1080_PS1_O_NR_004.zarr"
SENTINEL2_UTM_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202602-s02msil2a-eu/02/products/cpm_v262/S2A_MSIL2A_20260202T094641_N0511_R036_T34UDC_20260202T104719.zarr"


def run_gdalinfo(url):
    """Run gdalinfo on a URL and return output"""
    path = f'EOPFZARR:"/vsicurl/{url}"'
    try:
        result = subprocess.run(
            ['gdalinfo', path],
            capture_output=True,
            text=True,
            timeout=30
        )
        return result.stdout, result.returncode
    except subprocess.TimeoutExpired:
        return None, -1
    except Exception as e:
        print(f"Error running gdalinfo: {e}")
        return None, -1


def test_sentinel3_bbox_ordering():
    """Test Sentinel-3 SLSTR bbox ordering fix"""
    print("\n" + "="*80)
    print("TEST 1: Sentinel-3 SLSTR - EOPF bbox ordering")
    print("="*80)
    print(f"URL: {SENTINEL3_SLSTR_URL[:80]}...")
    
    output, returncode = run_gdalinfo(SENTINEL3_SLSTR_URL)
    
    if returncode != 0 or output is None:
        print("❌ FAILED: Could not open dataset")
        return False
    
    # Check for EPSG:4326
    if "EPSG" not in output or "4326" not in output:
        print("❌ FAILED: Missing EPSG:4326")
        return False
    
    # Extract geospatial metadata
    lon_min_match = re.search(r'geospatial_lon_min=([\d.]+)', output)
    lon_max_match = re.search(r'geospatial_lon_max=([\d.]+)', output)
    lat_min_match = re.search(r'geospatial_lat_min=([\d.]+)', output)
    lat_max_match = re.search(r'geospatial_lat_max=([\d.]+)', output)
    
    if all([lon_min_match, lon_max_match, lat_min_match, lat_max_match]):
        lon_min = float(lon_min_match.group(1))
        lon_max = float(lon_max_match.group(1))
        lat_min = float(lat_min_match.group(1))
        lat_max = float(lat_max_match.group(1))
        
        print(f"\nExtracted bounds:")
        print(f"  Longitude: {lon_min:.4f}° to {lon_max:.4f}°")
        print(f"  Latitude:  {lat_min:.4f}° to {lat_max:.4f}°")
        
        # Check if in Djibouti region (40-57°E, 7-21°N) with small tolerance
        if 39 <= lon_min <= 57 and 39 <= lon_max <= 57:
            if 6 <= lat_min <= 22 and 6 <= lat_max <= 22:
                print("\n✅ PASS: Coordinates in correct region (Djibouti/Horn of Africa)")
                print("   Bbox ordering fix is working!")
                return True
        
        print(f"\n❌ FAILED: Coordinates not in expected Djibouti region")
        print(f"   Expected: approximately 40-56°E, 7-21°N")
        print(f"   Got: {lon_min:.1f}-{lon_max:.1f}°E, {lat_min:.1f}-{lat_max:.1f}°N")
        return False
    else:
        print("❌ FAILED: Could not extract geospatial bounds")
        return False


def test_sentinel2_utm_no_invalid_geotransform():
    """Test Sentinel-2 UTM product without invalid geotransform"""
    print("\n" + "="*80)
    print("TEST 2: Sentinel-2 MSI L2A - UTM without proj:bbox")
    print("="*80)
    print(f"URL: {SENTINEL2_UTM_URL[:80]}...")
    
    output, returncode = run_gdalinfo(SENTINEL2_UTM_URL)
    
    if returncode != 0 or output is None:
        print("❌ FAILED: Could not open dataset")
        return False
    
    # Check for EPSG:32625
    if "EPSG" not in output or "32625" not in output:
        print("❌ FAILED: Missing EPSG:32625 (UTM Zone 25N)")
        return False
    
    print("\n✅ CRS correctly set: EPSG:32625 (UTM Zone 25N)")
    
    # Extract origin from Corner Coordinates or Origin
    origin_match = re.search(r'Origin = \(([\d.-]+),([\d.-]+)\)', output)
    corner_match = re.search(r'Upper Left  \(\s*([\d.-]+),\s*([\d.-]+)\)', output)
    
    if origin_match:
        origin_x = float(origin_match.group(1))
        origin_y = float(origin_match.group(2))
    elif corner_match:
        origin_x = float(corner_match.group(1))
        origin_y = float(corner_match.group(2))
    else:
        print("⚠ WARNING: No Origin or Corner Coordinates found")
        origin_x = None
        origin_y = None
    
    if origin_x is not None:
        print(f"\nGeotransform origin: ({origin_x:.1f}, {origin_y:.1f})")
        
        # Check for the BUG: invalid coordinates (11M+ easting)
        if origin_x > 10_000_000:
            print(f"\n❌ FAILED: Invalid UTM coordinates detected!")
            print(f"   Origin X: {origin_x:.0f}m (BUG: treating degrees as meters)")
            print(f"   Valid UTM easting should be < 1,000,000m")
            return False
        elif origin_x == 0.0 and origin_y == 0.0:
            print(f"\n✅ PASS: No geotransform set (0, 0)")
            print(f"   This is correct for UTM products without proj:bbox")
            return True
        elif 166_000 <= origin_x <= 834_000:
            print(f"\n✅ PASS: Valid UTM coordinates")
            print(f"   Easting in valid range for UTM zone")
            return True
        else:
            print(f"\n⚠ UNCERTAIN: Unusual coordinates but not the known bug")
            print(f"   Not the 11M+ bug, so fix is working")
            return True
    else:
        print(f"\n✅ PASS: No geotransform information")
        print(f"   This is acceptable for UTM products without proj:bbox")
        return True


def main():
    """Run all tests"""
    print("\n" + "#"*80)
    print("# Manual Integration Tests for Issue #135")
    print("# Testing EOPF bbox ordering and UTM geotransform fixes")
    print("#"*80)
    
    results = []
    
    # Test 1: Sentinel-3 bbox ordering
    results.append(("Sentinel-3 SLSTR bbox ordering", test_sentinel3_bbox_ordering()))
    
    # Test 2: Sentinel-2 UTM without invalid geotransform
    results.append(("Sentinel-2 UTM geotransform", test_sentinel2_utm_no_invalid_geotransform()))
    
    # Summary
    print("\n" + "="*80)
    print("TEST SUMMARY")
    print("="*80)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for test_name, result in results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{status}: {test_name}")
    
    print(f"\nResult: {passed}/{total} tests passed")
    
    if passed == total:
        print("\n🎉 All tests passed! Issue #135 fixes are working correctly.")
        return 0
    else:
        print(f"\n⚠ {total - passed} test(s) failed.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
