#!/usr/bin/env python3
"""
EOPF-Zarr Docker Test Script
Tests the EOPF-Zarr driver installation and basic functionality
"""

import sys
import os

def test_gdal_installation():
    """Test GDAL installation and version"""
    print("🔍 Testing GDAL installation...")
    try:
        from osgeo import gdal
        print(f"✅ GDAL Version: {gdal.VersionInfo()}")
        print(f"📦 Available GDAL drivers: {gdal.GetDriverCount()}")
        return True
    except ImportError as e:
        print(f"❌ GDAL import failed: {e}")
        return False

def test_eopf_driver():
    """Test EOPF-Zarr driver availability"""
    print("\n🔍 Testing EOPF-Zarr driver...")
    try:
        from osgeo import gdal
        gdal.AllRegister()
        
        # Try to get the EOPF-Zarr driver
        driver = gdal.GetDriverByName('EOPFZARR')
        if driver:
            print(f"✅ EOPF-Zarr driver found: {driver.GetDescription()}")
            print(f"   Driver metadata: {driver.GetMetadata()}")
            return True
        else:
            print("⚠️ EOPF-Zarr driver not found")
            # List all available drivers for debugging
            print("📋 Available drivers:")
            for i in range(gdal.GetDriverCount()):
                drv = gdal.GetDriver(i)
                print(f"   {i}: {drv.GetDescription()}")
            return False
    except Exception as e:
        print(f"❌ Error testing EOPF driver: {e}")
        return False

def test_python_packages():
    """Test required Python packages"""
    print("\n🔍 Testing Python packages...")
    
    required_packages = [
        ('xarray', 'xarray'),
        ('zarr', 'zarr'),
        ('dask', 'dask'),
        ('geopandas', 'geopandas'),
        ('rasterio', 'rasterio'),
        ('numpy', 'numpy'),
        ('pandas', 'pandas')
    ]
    
    all_good = True
    for package_name, import_name in required_packages:
        try:
            __import__(import_name)
            print(f"✅ {package_name}")
        except ImportError:
            print(f"❌ {package_name} not available")
            all_good = False
    
    return all_good

def test_environment():
    """Test environment variables"""
    print("\n🔍 Testing environment...")
    
    env_vars = [
        'GDAL_DRIVER_PATH',
        'GDAL_DATA',
        'PROJ_LIB',
        'PYTHONPATH'
    ]
    
    for var in env_vars:
        value = os.environ.get(var)
        if value:
            print(f"✅ {var}={value}")
        else:
            print(f"⚠️ {var} not set")

def main():
    """Run all tests"""
    print("🚀 EOPF-Zarr Docker Environment Test")
    print("=" * 50)
    
    test_environment()
    
    gdal_ok = test_gdal_installation()
    if not gdal_ok:
        print("\n❌ GDAL test failed - cannot continue")
        sys.exit(1)
    
    eopf_ok = test_eopf_driver()
    packages_ok = test_python_packages()
    
    print("\n" + "=" * 50)
    print("📊 Test Summary:")
    print(f"   GDAL: {'✅' if gdal_ok else '❌'}")
    print(f"   EOPF-Zarr Driver: {'✅' if eopf_ok else '⚠️'}")
    print(f"   Python Packages: {'✅' if packages_ok else '❌'}")
    
    if gdal_ok and packages_ok:
        print("\n🎉 Docker environment is ready!")
        if not eopf_ok:
            print("⚠️ Note: EOPF-Zarr driver not loaded, but GDAL environment is working")
    else:
        print("\n❌ Some tests failed - check the installation")
        sys.exit(1)

if __name__ == "__main__":
    main()
