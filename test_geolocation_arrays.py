#!/usr/bin/env python
"""
Test script for verifying GDAL Geolocation Arrays support in EOPFZARR driver.

This script tests Issue #137: Use native geolocation arrays instead of bbox approximation.
"""

import os
from osgeo import gdal

# Enable debug output
gdal.SetConfigOption('CPL_DEBUG', 'ON')

# Test URL - Sentinel-3 SLSTR product with lat/lon arrays
test_url = "https://s3.waw3-1.cloudferro.com/swift/v1/AUTH_4c3cdb2dc8164a0488c4d1fc29eda0bb/Sentinel3/SLSTR/SL_2_LST___/2024/03/03/S3B_SL_2_LST____20240303T051830_20240303T052130_20240304T115256_0179_090_262_1080_PS2_O_NT_004.SEN3/L2_measurements.zarr"

print("=" * 80)
print("Testing EOPFZARR Driver - Geolocation Arrays Support")
print("=" * 80)
print()

# Open a measurement subdataset that should have lat/lon coordinates
subdataset_path = f'EOPFZARR:"{test_url}":/measurements/inadir/s7_bt_in'
print(f"Opening: {subdataset_path}")
print()

ds = gdal.Open(subdataset_path)
if not ds:
    print("❌ Failed to open dataset")
    exit(1)

print("✅ Dataset opened successfully")
print()

# Check for GEOLOCATION metadata domain
print("-" * 80)
print("GEOLOCATION Metadata:")
print("-" * 80)

geoloc_metadata = ds.GetMetadata('GEOLOCATION')
if geoloc_metadata:
    for key, value in geoloc_metadata.items():
        # Truncate long values
        if len(value) > 100:
            value = value[:100] + "..."
        print(f"  {key}: {value}")
    print()
    print("✅ GEOLOCATION metadata domain found")
else:
    print("❌ No GEOLOCATION metadata found")
    print("   This means the driver is not detecting lat/lon arrays")

print()

# Check GeoTransform (should still exist for backwards compatibility)
print("-" * 80)
print("GeoTransform (for backwards compatibility):")
print("-" * 80)
gt = ds.GetGeoTransform()
if gt and gt != (0, 1, 0, 0, 0, 1):
    print(f"  Origin: ({gt[0]}, {gt[3]})")
    print(f"  Pixel Size: ({gt[1]}, {gt[5]})")
    print(f"  ✅ GeoTransform present")
else:
    print("  ⚠️  No GeoTransform or default identity transform")

print()

# Check subdatasets (should include lat/lon arrays)
print("-" * 80)
print("Checking for lat/lon subdatasets:")
print("-" * 80)

# Open root dataset to check subdatasets
root_ds = gdal.Open(f'EOPFZARR:"{test_url}":/')
if root_ds:
    subdatasets = root_ds.GetMetadata('SUBDATASETS')
    lat_found = False
    lon_found = False
    
    for key, value in subdatasets.items():
        if 'latitude' in value.lower():
            print(f"  {key}: {value}")
            lat_found = True
        elif 'longitude' in value.lower():
            print(f"  {key}: {value}")
            lon_found = True
    
    if lat_found and lon_found:
        print()
        print("✅ Latitude and longitude subdatasets found")
    else:
        print()
        if not lat_found:
            print("❌ Latitude subdataset not found")
        if not lon_found:
            print("❌ Longitude subdataset not found")
    
    root_ds = None

print()

# Dataset info
print("-" * 80)
print("Dataset Info:")
print("-" * 80)
print(f"  Size: {ds.RasterXSize} x {ds.RasterYSize}")
print(f"  Bands: {ds.RasterCount}")
print(f"  Driver: {ds.GetDriver().ShortName}")

ds = None

print()
print("=" * 80)
print("Test Complete")
print("=" * 80)
print()
print("Expected Results:")
print("  ✅ GEOLOCATION metadata domain with X_DATASET, Y_DATASET, etc.")
print("  ✅ X_DATASET and Y_DATASET pointing to lat/lon subdatasets")
print("  ✅ GeoTransform still present for backwards compatibility")
print("  ✅ Latitude and longitude arrays listed in subdatasets")
print()
print("Usage with gdalwarp:")
print("  gdalwarp -geoloc <input> <output>")
print("  This will use the precise lat/lon arrays instead of bbox approximation")
print()
