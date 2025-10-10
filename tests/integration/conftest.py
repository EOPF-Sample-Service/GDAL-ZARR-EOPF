"""
pytest configuration for integration tests.

This conftest.py ensures the EOPFZARR driver is available before running tests.
"""

import os
import sys
import pytest


def pytest_configure(config):
    """
    Configure pytest before test collection.
    
    This ensures GDAL_DRIVER_PATH is set before any GDAL imports happen.
    """
    # If GDAL_DRIVER_PATH is not set, try to set it intelligently
    if "GDAL_DRIVER_PATH" not in os.environ:
        # Try to find the build directory relative to the test directory
        test_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.dirname(os.path.dirname(test_dir))
        
        # Check common build locations
        possible_paths = [
            os.path.join(repo_root, "build", "Release"),  # Windows MSVC
            os.path.join(repo_root, "build"),              # Linux/macOS
        ]
        
        for path in possible_paths:
            # Check if driver file exists
            driver_files = [
                os.path.join(path, "gdal_EOPFZarr.dll"),  # Windows
                os.path.join(path, "libgdal_EOPFZarr.so"),  # Linux
                os.path.join(path, "libgdal_EOPFZarr.dylib"),  # macOS
            ]
            
            if any(os.path.exists(f) for f in driver_files):
                os.environ["GDAL_DRIVER_PATH"] = path
                print(f"Set GDAL_DRIVER_PATH to: {path}", file=sys.stderr)
                break


@pytest.fixture(scope="session", autouse=True)
def check_eopfzarr_driver():
    """
    Session-scoped fixture that checks if EOPFZARR driver is available.
    
    This runs once per test session and provides clear error messages if the driver
    is not found.
    """
    try:
        from osgeo import gdal
        gdal.UseExceptions()
        
        # Force GDAL to register all drivers
        gdal.AllRegister()
        
        driver = gdal.GetDriverByName("EOPFZARR")
        
        if driver is None:
            # List available drivers for debugging
            driver_count = gdal.GetDriverCount()
            available_drivers = [
                gdal.GetDriver(i).GetDescription() 
                for i in range(driver_count)
            ]
            
            error_msg = (
                f"EOPFZARR driver not available!\n"
                f"GDAL_DRIVER_PATH = {os.environ.get('GDAL_DRIVER_PATH', 'NOT SET')}\n"
                f"Found {driver_count} drivers: {', '.join(available_drivers[:10])}..."
            )
            
            pytest.exit(error_msg, returncode=1)
        
        print(f"✓ EOPFZARR driver is available", file=sys.stderr)
        
    except ImportError as e:
        pytest.exit(f"Failed to import GDAL: {e}", returncode=1)


def pytest_runtest_setup(item):
    """
    Hook that runs before each test.
    
    Checks for require_driver marker and skips tests if driver is not available.
    """
    for marker in item.iter_markers(name="require_driver"):
        driver_name = marker.args[0] if marker.args else None
        if driver_name:
            try:
                from osgeo import gdal
                driver = gdal.GetDriverByName(driver_name)
                if driver is None:
                    pytest.skip(f"Driver {driver_name} not available")
            except ImportError:
                pytest.skip("GDAL not available")
