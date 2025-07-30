#!/usr/bin/env python3
"""
Test runner script for EOPF-Zarr GDAL driver.

This script provides an easy way to run different types of tests with proper setup.
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path


def setup_environment():
    """Set up the test environment."""
    # Add the current directory to Python path for imports
    current_dir = Path(__file__).parent
    sys.path.insert(0, str(current_dir))
    
    # Set GDAL environment variables if needed
    if os.name == 'nt':  # Windows
        # Add potential GDAL paths for Windows
        gdal_paths = [
            r"C:\OSGeo4W64\bin",
            r"C:\Program Files\GDAL\bin",
            os.path.join(os.environ.get('CONDA_PREFIX', ''), 'Library', 'bin')
        ]
        
        for gdal_path in gdal_paths:
            if os.path.exists(gdal_path):
                os.environ['PATH'] = gdal_path + os.pathsep + os.environ.get('PATH', '')
                break


def check_dependencies():
    """Check if required dependencies are available."""
    missing_deps = []
    
    try:
        import pytest
    except ImportError:
        missing_deps.append("pytest")
    
    try:
        from osgeo import gdal
    except ImportError:
        missing_deps.append("GDAL Python bindings")
    
    if missing_deps:
        print("Missing dependencies:")
        for dep in missing_deps:
            print(f"  - {dep}")
        print("\nPlease install missing dependencies before running tests.")
        return False
    
    # Check if EOPF-Zarr driver is available
    try:
        from osgeo import gdal
        driver = gdal.GetDriverByName("EOPFZARR")
        if driver is None:
            print("Warning: EOPF-Zarr driver not found. Driver-specific tests will be skipped.")
    except Exception as e:
        print(f"Warning: Could not check driver availability: {e}")
    
    return True


def generate_test_data():
    """Generate test data if it doesn't exist."""
    test_data_dir = Path(__file__).parent / "tests" / "data"
    generate_script = Path(__file__).parent / "tests" / "generate_test_data.py"
    
    if not test_data_dir.exists() and generate_script.exists():
        print("Generating test data...")
        try:
            subprocess.run([sys.executable, str(generate_script)], check=True)
            print("Test data generated successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Failed to generate test data: {e}")
            return False
        except Exception as e:
            print(f"Error generating test data: {e}")
            return False
    
    return True


def run_unit_tests(verbose=False):
    """Run C++ unit tests via CTest."""
    build_dir = Path(__file__).parent / "build"
    
    if not build_dir.exists():
        print("Build directory not found. Please build the project first.")
        return False
    
    print("Running C++ unit tests...")
    cmd = ["ctest", "--output-on-failure"]
    if verbose:
        cmd.append("--verbose")
    
    try:
        result = subprocess.run(cmd, cwd=build_dir, check=True)
        print("C++ unit tests completed successfully.")
        return True
    except subprocess.CalledProcessError as e:
        print(f"C++ unit tests failed: {e}")
        return False
    except FileNotFoundError:
        print("CTest not found. Please ensure CMake is installed and in PATH.")
        return False


def run_python_tests(test_type="all", verbose=False, markers=None):
    """Run Python integration tests."""
    cmd = [sys.executable, "-m", "pytest"]
    
    if verbose:
        cmd.append("-v")
    
    # Add test type selection
    if test_type == "integration":
        cmd.extend(["tests/integration/"])
    elif test_type == "performance":
        cmd.extend(["-m", "performance"])
    elif test_type == "compatibility":
        cmd.extend(["-m", "compatibility"])
    elif test_type == "quick":
        cmd.extend(["-m", "not slow"])
    else:  # all
        cmd.extend(["tests/"])
    
    # Add custom markers
    if markers:
        cmd.extend(["-m", markers])
    
    print(f"Running Python tests ({test_type})...")
    try:
        result = subprocess.run(cmd, check=True)
        print("Python tests completed successfully.")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Python tests failed: {e}")
        return False


def run_smoke_tests():
    """Run basic smoke tests to verify driver functionality."""
    print("Running smoke tests...")
    
    try:
        from osgeo import gdal
        gdal.UseExceptions()
        
        # Test driver registration
        driver = gdal.GetDriverByName("EOPFZARR")
        if driver is None:
            print("❌ EOPF-Zarr driver not registered")
            return False
        else:
            print("✅ EOPF-Zarr driver registered")
        
        # Test basic driver info
        long_name = driver.GetMetadataItem("DMD_LONGNAME")
        if long_name:
            print(f"✅ Driver long name: {long_name}")
        else:
            print("⚠️ Driver long name not available")
        
        # Test format detection with gdalinfo
        print("✅ Basic smoke tests passed")
        return True
        
    except Exception as e:
        print(f"❌ Smoke tests failed: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description="Test runner for EOPF-Zarr GDAL driver")
    parser.add_argument("--type", choices=["all", "unit", "integration", "performance", "compatibility", "quick", "smoke"],
                       default="all", help="Type of tests to run")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--generate-data", action="store_true", help="Generate test data before running tests")
    parser.add_argument("--markers", "-m", help="Custom pytest markers to apply")
    parser.add_argument("--skip-deps-check", action="store_true", help="Skip dependency checking")
    
    args = parser.parse_args()
    
    # Setup environment
    setup_environment()
    
    # Check dependencies
    if not args.skip_deps_check and not check_dependencies():
        return 1
    
    # Generate test data if requested
    if args.generate_data or args.type in ["integration", "all"]:
        if not generate_test_data():
            return 1
    
    success = True
    
    # Run tests based on type
    if args.type == "smoke":
        success = run_smoke_tests()
    elif args.type == "unit":
        success = run_unit_tests(args.verbose)
    elif args.type in ["integration", "performance", "compatibility", "quick"]:
        success = run_python_tests(args.type, args.verbose, args.markers)
    elif args.type == "all":
        # Run all test types
        print("=== Running Smoke Tests ===")
        success &= run_smoke_tests()
        
        print("\n=== Running C++ Unit Tests ===")
        success &= run_unit_tests(args.verbose)
        
        print("\n=== Running Python Integration Tests ===")  
        success &= run_python_tests("integration", args.verbose, args.markers)
    
    if success:
        print("\n🎉 All tests completed successfully!")
        return 0
    else:
        print("\n❌ Some tests failed.")
        return 1


if __name__ == "__main__":
    exit(main())
