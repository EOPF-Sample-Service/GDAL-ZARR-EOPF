#!/usr/bin/env python3
"""
Basic test script for Sentinel-1 GRD data access using EOPFZARR driver.
This validates the key functionality demonstrated in the notebook.
"""

import sys

print("=" * 80)
print("Sentinel-1 GRD EOPFZARR Driver Test")
print("=" * 80)

# Test 1: Import GDAL
print("\n1. Testing GDAL import...")
try:
    from osgeo import gdal
    gdal.UseExceptions()
    print(f"   ✅ GDAL version: {gdal.__version__}")
except ImportError as e:
    print(f"   ❌ Failed to import GDAL: {e}")
    print("\n   To install GDAL Python bindings:")
    print("   - System package: sudo apt-get install python3-gdal")
    print("   - Or use the Docker environment as documented")
    sys.exit(1)

# Test 2: Check driver registration
print("\n2. Checking EOPFZARR driver registration...")
driver = gdal.GetDriverByName('EOPFZARR')
if driver:
    print(f"   ✅ EOPFZARR driver found: {driver.GetDescription()}")
else:
    print("   ❌ EOPFZARR driver not found!")
    print("   Make sure the plugin is installed in GDAL plugins directory")
    sys.exit(1)

# Test 3: Open Sentinel-1 dataset
print("\n3. Opening Sentinel-1 dataset...")
base_url = (
    "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202601-s01sewgrm-global/"
    "29/products/cpm_v262/S1C_EW_GRDM_1SSH_20260129T124421_20260129T124526_"
    "006118_00C473_F440.zarr"
)
zarr_path = f'EOPFZARR:"/vsicurl/{base_url}"'

try:
    root_ds = gdal.Open(zarr_path)
    if root_ds:
        print(f"   ✅ Root dataset opened")
        print(f"      Size: {root_ds.RasterXSize} x {root_ds.RasterYSize}")
    else:
        print("   ❌ Failed to open dataset")
        sys.exit(1)
except Exception as e:
    print(f"   ❌ Error opening dataset: {e}")
    sys.exit(1)

# Test 4: List subdatasets
print("\n4. Discovering subdatasets...")
subdatasets = root_ds.GetMetadata("SUBDATASETS")
sub_count = len([k for k in subdatasets.keys() if k.endswith('_NAME')])
print(f"   ✅ Found {sub_count} subdatasets")

# Test 5: Find GRD measurement
print("\n5. Locating GRD measurement array...")
grd_path = None
for key, value in subdatasets.items():
    if key.endswith('_NAME') and 'measurements/grd' in value:
        grd_path = value
        print(f"   ✅ Found: {grd_path.split(':/')[1]}")
        break

if not grd_path:
    print("   ❌ GRD measurement not found")
    sys.exit(1)

# Test 6: Open GRD subdataset
print("\n6. Opening GRD measurement subdataset...")
try:
    grd_ds = gdal.Open(grd_path)
    if grd_ds:
        print(f"   ✅ GRD dataset opened")
        print(f"      Dimensions: {grd_ds.RasterXSize} x {grd_ds.RasterYSize} pixels")
        print(f"      Data type: {gdal.GetDataTypeName(grd_ds.GetRasterBand(1).DataType)}")
    else:
        print("   ❌ Failed to open GRD dataset")
        sys.exit(1)
except Exception as e:
    print(f"   ❌ Error: {e}")
    sys.exit(1)

# Test 7: Check for geolocation metadata
print("\n7. Checking geolocation metadata...")
domains = grd_ds.GetMetadataDomainList()
if 'GEOLOCATION' in (domains or []):
    geoloc_md = grd_ds.GetMetadata('GEOLOCATION')
    print("   ✅ GEOLOCATION metadata found:")
    for key, value in list(geoloc_md.items())[:3]:
        print(f"      {key}: {value[:60]}...")
else:
    print("   ⚠️  No GEOLOCATION metadata (expected for GCP-based data)")

# Test 8: Check for GCPs
print("\n8. Checking for Ground Control Points...")
gcp_count = grd_ds.GetGCPCount()
if gcp_count > 0:
    print(f"   ✅ Found {gcp_count} GCPs attached to dataset")
    gcps = grd_ds.GetGCPs()
    print("   First GCP:")
    gcp = gcps[0]
    print(f"      Pixel: ({gcp.GCPPixel:.1f}, {gcp.GCPLine:.1f})")
    print(f"      Geo: ({gcp.GCPX:.4f}, {gcp.GCPY:.4f})")
else:
    print(f"   ⚠️  No GCPs attached (found {gcp_count})")
    print("   Note: GCPs are in separate arrays at conditions/gcp/")

# Test 9: Find GCP arrays
print("\n9. Locating GCP arrays...")
gcp_lat_path = None
gcp_lon_path = None

for key, value in subdatasets.items():
    if key.endswith('_NAME'):
        if 'conditions/gcp/latitude' in value:
            gcp_lat_path = value
        elif 'conditions/gcp/longitude' in value:
            gcp_lon_path = value

if gcp_lat_path and gcp_lon_path:
    print("   ✅ Found GCP arrays:")

    lat_ds = gdal.Open(gcp_lat_path)
    if lat_ds:
        print(f"      Latitude: {lat_ds.RasterXSize} x {lat_ds.RasterYSize}")
        lat_ds = None

    lon_ds = gdal.Open(gcp_lon_path)
    if lon_ds:
        print(f"      Longitude: {lon_ds.RasterXSize} x {lon_ds.RasterYSize}")
        lon_ds = None
else:
    print("   ❌ GCP arrays not found")
    sys.exit(1)

# Test 10: Read a small data sample
print("\n10. Testing data read (small sample)...")
try:
    # Read a 100x100 pixel subset
    band = grd_ds.GetRasterBand(1)
    data = band.ReadAsArray(0, 0, 100, 100)

    if data is not None:
        print(f"   ✅ Data read successful")
        print(f"      Sample shape: {data.shape}")
        print(f"      Value range: [{data.min()}, {data.max()}]")
    else:
        print("   ❌ Failed to read data")
        sys.exit(1)
except Exception as e:
    print(f"   ❌ Error reading data: {e}")
    sys.exit(1)

# Cleanup
grd_ds = None
root_ds = None

print("\n" + "=" * 80)
print("✅ All tests passed!")
print("=" * 80)
print("\nConclusions:")
print("  • Sentinel-1 dataset opens successfully")
print("  • GRD measurement data is accessible")
print("  • GCP arrays are available as subdatasets")
print("  • Data can be read from the SAR array")
print("\nLimitations:")
print("  ⚠️  GCPs are not automatically attached to measurement dataset")
print("  ⚠️  No GEOLOCATION metadata domain (sparse GCP grid)")
print("  ⚠️  Users must manually access and interpolate GCP arrays")
print("\nNext steps:")
print("  → Run the full notebook: notebooks/11-Sentinel-1-GRD-Demo.ipynb")
print("  → Consider extending driver to attach GCPs automatically")
print("=" * 80)
