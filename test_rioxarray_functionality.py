#!/usr/bin/env python
"""Test rioxarray compatibility after subdataset path fix"""

import sys

try:
    from osgeo import gdal
    import rioxarray
    import xarray as xr
except ImportError as e:
    print(f"❌ Missing required package: {e}")
    print("Installing rioxarray...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "rioxarray", "--quiet"])
    import rioxarray
    import xarray as xr

gdal.UseExceptions()

# Test URL
url = 'https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202508-s03olcefr/19/products/cpm_v256/S3B_OL_1_EFR____20250819T074058_20250819T074358_20250819T092155_0179_110_106_3420_ESA_O_NR_004.zarr'

print("=" * 80)
print("Testing RIOXARRAY compatibility after subdataset path fix")
print("=" * 80)

# First get a subdataset path using GDAL
print(f"\n1. Getting subdataset path from GDAL...")
eopf_path = f'EOPFZARR:"/vsicurl/{url}"'
ds = gdal.Open(eopf_path)
subs = ds.GetMetadata('SUBDATASETS')
sub_names = [v for k, v in subs.items() if k.endswith('_NAME')]

# Find a measurement subdataset
measurement_sub = None
for sub in sub_names:
    if '/measurements/' in sub.lower():
        measurement_sub = sub
        break

if not measurement_sub:
    measurement_sub = sub_names[0]

print(f"   Subdataset to test: {measurement_sub}")

try:
    print(f"\n2. Opening subdataset with rioxarray...")
    da = rioxarray.open_rasterio(measurement_sub)
    
    print(f"   ✅ SUCCESS: rioxarray.open_rasterio() worked!")
    print(f"   Type: {type(da)}")
    print(f"   Dimensions: {da.dims}")
    print(f"   Shape: {da.shape}")
    print(f"   Data type: {da.dtype}")
    
    print(f"\n3. Checking rioxarray extensions...")
    print(f"   Has .rio accessor: {hasattr(da, 'rio')}")
    if hasattr(da, 'rio'):
        try:
            crs = da.rio.crs
            print(f"   CRS: {crs}")
        except:
            print(f"   CRS: None")
        
        try:
            bounds = da.rio.bounds()
            print(f"   Bounds: {bounds}")
        except:
            print(f"   Bounds: Not available")
    
    print(f"\n4. Testing xarray operations...")
    # Test slicing
    subset = da.isel(x=slice(0, min(10, da.sizes['x'])), y=slice(0, min(10, da.sizes['y'])))
    print(f"   ✅ Slicing works: {subset.shape}")
    
    # Test computation
    if subset.size > 0:
        mean_val = float(subset.mean())
        print(f"   ✅ Computation works: mean = {mean_val:.4f}")
    
    print("\n" + "=" * 80)
    print("✅ RIOXARRAY COMPATIBILITY TEST PASSED!")
    print("=" * 80)
    print("\n🎉 Issue #101 is RESOLVED - rioxarray now works with EOPFZARR!")
    
except Exception as e:
    print(f"\n❌ ERROR: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
