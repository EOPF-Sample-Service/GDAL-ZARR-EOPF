#!/usr/bin/env python3
"""
Comprehensive path combination test for EOPF-Zarr driver.

This script tests various path formats, URL schemes, and edge cases
to ensure the driver handles all combinations correctly.
"""

import sys
import os
from pathlib import Path

try:
    from osgeo import gdal
    gdal.UseExceptions()
except ImportError:
    print("❌ GDAL Python bindings not available")
    sys.exit(1)


class PathTester:
    """Comprehensive path testing for EOPF-Zarr driver."""
    
    def __init__(self):
        self.test_results = []
        self.driver = None
        
    def setup(self):
        """Initialize the test environment."""
        # Check driver availability
        self.driver = gdal.GetDriverByName("EOPFZARR")
        if self.driver is None:
            print("❌ EOPF-Zarr driver not available")
            return False
        
        print("✅ EOPF-Zarr driver available")
        print(f"   Long name: {self.driver.GetMetadataItem('DMD_LONGNAME')}")
        
        # Enable debug logging to see path parsing
        gdal.SetConfigOption("CPL_DEBUG", "EOPFZARR")
        
        return True
    
    def test_path(self, path, expected_behavior, description):
        """Test a single path and record results."""
        print(f"\n--- Testing: {description} ---")
        print(f"Path: {path}")
        
        try:
            ds = gdal.Open(f"EOPFZARR:{path}")
            
            if ds is not None:
                result = "✅ OPENED"
                details = f"Dims: {ds.RasterXSize}x{ds.RasterYSize}, Bands: {ds.RasterCount}"
                ds = None  # Close dataset
            else:
                result = "❌ FAILED_TO_OPEN"
                details = "Dataset returned None"
                
        except Exception as e:
            result = "❌ EXCEPTION"
            details = str(e)
        
        self.test_results.append({
            'path': path,
            'description': description,
            'result': result,
            'details': details,
            'expected': expected_behavior
        })
        
        print(f"Result: {result}")
        print(f"Details: {details}")
        
        return result.startswith("✅")
    
    def test_local_paths(self):
        """Test various local path formats."""
        print("\n" + "="*60)
        print("🗂️  TESTING LOCAL PATHS")
        print("="*60)
        
        # Windows-style paths
        if os.name == 'nt':
            self.test_path(
                r"C:\data\test.zarr",
                "FAIL",
                "Windows absolute path with backslashes"
            )
            
            self.test_path(
                "C:/data/test.zarr",
                "FAIL", 
                "Windows absolute path with forward slashes"
            )
            
            self.test_path(
                r".\relative\test.zarr",
                "FAIL",
                "Windows relative path"
            )
        
        # Unix-style paths
        self.test_path(
            "/home/user/data/test.zarr",
            "FAIL",
            "Unix absolute path"
        )
        
        self.test_path(
            "./relative/test.zarr", 
            "FAIL",
            "Unix relative path"
        )
        
        self.test_path(
            "relative/test.zarr",
            "FAIL", 
            "Simple relative path"
        )
        
        # Path with spaces
        self.test_path(
            "/path with spaces/test.zarr",
            "FAIL",
            "Path with spaces"
        )
        
        # Path with special characters
        self.test_path(
            "/path/with-special_chars.123/test.zarr",
            "FAIL",
            "Path with special characters"
        )
    
    def test_url_schemes(self):
        """Test various URL schemes."""
        print("\n" + "="*60)
        print("🌐 TESTING URL SCHEMES")
        print("="*60)
        
        # HTTP URLs
        self.test_path(
            "http://example.com/data.zarr",
            "FAIL",
            "Simple HTTP URL"
        )
        
        self.test_path(
            "http://example.com:8080/data.zarr",
            "FAIL",
            "HTTP URL with port"
        )
        
        # HTTPS URLs
        self.test_path(
            "https://example.com/data.zarr", 
            "FAIL",
            "Simple HTTPS URL"
        )
        
        self.test_path(
            "https://example.com:443/data.zarr",
            "FAIL",
            "HTTPS URL with port"
        )
        
        # EODC URL (the real one we're testing)
        self.test_path(
            "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr",
            "FAIL_NETWORK",
            "Real EODC HTTPS URL with complex path"
        )
        
        # Complex URLs with query parameters
        self.test_path(
            "https://example.com/data.zarr?param=value&other=123",
            "FAIL",
            "URL with query parameters"
        )
        
        # URLs with authentication
        self.test_path(
            "https://user:pass@example.com/data.zarr",
            "FAIL",
            "URL with authentication"
        )
    
    def test_vsi_paths(self):
        """Test VSI (Virtual File System) paths."""
        print("\n" + "="*60)
        print("📁 TESTING VSI PATHS")
        print("="*60)
        
        # VSI Curl
        self.test_path(
            "/vsicurl/http://example.com/data.zarr",
            "FAIL",
            "VSI Curl with HTTP"
        )
        
        self.test_path(
            "/vsicurl/https://example.com/data.zarr",
            "FAIL",
            "VSI Curl with HTTPS"
        )
        
        # VSI S3
        self.test_path(
            "/vsis3/bucket/path/data.zarr",
            "FAIL",
            "VSI S3 path"
        )
        
        self.test_path(
            "/vsis3/bucket-name-with-dashes/data.zarr", 
            "FAIL",
            "VSI S3 with dashed bucket name"
        )
        
        # VSI Azure
        self.test_path(
            "/vsiaz/container/data.zarr",
            "FAIL",
            "VSI Azure path"
        )
        
        self.test_path(
            "/vsiazure/container/data.zarr",
            "FAIL", 
            "VSI Azure (long form)"
        )
        
        # VSI Google Cloud
        self.test_path(
            "/vsigs/bucket/data.zarr",
            "FAIL",
            "VSI Google Cloud Storage"
        )
        
        # Complex VSI paths
        self.test_path(
            "/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202506-s02msil1c/25/products/cpm_v256/S2C_MSIL1C_20250625T095051_N0511_R079_T33TWE_20250625T132854.zarr",
            "FAIL_NETWORK",
            "VSI Curl with real EODC URL"
        )
    
    def test_subdataset_combinations(self):
        """Test subdataset path combinations."""
        print("\n" + "="*60)
        print("📊 TESTING SUBDATASET COMBINATIONS")
        print("="*60)
        
        # Local path with subdataset
        self.test_path(
            "/path/to/data.zarr:band1",
            "FAIL",
            "Local path with subdataset"
        )
        
        # URL with subdataset
        self.test_path(
            "https://example.com/data.zarr:band1",
            "FAIL",
            "HTTPS URL with subdataset (should NOT split at colon)"
        )
        
        # VSI path with subdataset  
        self.test_path(
            "/vsicurl/https://example.com/data.zarr:band1",
            "FAIL",
            "VSI URL with subdataset"
        )
        
        # Quoted paths with subdatasets
        self.test_path(
            '"/path/to/data.zarr":band1',
            "FAIL",
            "Quoted local path with subdataset"
        )
        
        self.test_path(
            '"https://example.com/data.zarr":band1',
            "FAIL",
            "Quoted URL with subdataset"
        )
        
        # Windows drive letter edge cases
        if os.name == 'nt':
            self.test_path(
                "C:/data/test.zarr:band1",
                "FAIL",
                "Windows path with subdataset (colon confusion)"
            )
            
            self.test_path(
                '"C:/data/test.zarr":band1',
                "FAIL",
                "Quoted Windows path with subdataset"
            )
    
    def test_edge_cases(self):
        """Test edge cases and malformed paths."""
        print("\n" + "="*60)
        print("⚠️  TESTING EDGE CASES")
        print("="*60)
        
        # Empty and minimal paths
        self.test_path(
            "",
            "FAIL",
            "Empty path"
        )
        
        self.test_path(
            ":",
            "FAIL", 
            "Single colon"
        )
        
        self.test_path(
            "::",
            "FAIL",
            "Double colon"
        )
        
        # Malformed URLs
        self.test_path(
            "https://",
            "FAIL",
            "Incomplete HTTPS URL"
        )
        
        self.test_path(
            "https:///invalid",
            "FAIL",
            "Malformed HTTPS URL"
        )
        
        # Multiple colons
        self.test_path(
            "https://example.com:8080/data:with:colons.zarr",
            "FAIL",
            "URL with multiple colons"
        )
        
        # Very long paths
        long_path = "https://example.com/" + "very_long_directory_name/" * 20 + "data.zarr"
        self.test_path(
            long_path,
            "FAIL",
            "Very long URL path"
        )
        
        # Unicode characters
        self.test_path(
            "https://example.com/данные/файл.zarr",
            "FAIL",
            "URL with Unicode characters"
        )
        
        # Special characters that might break parsing
        self.test_path(
            'https://example.com/path%20with%20encoded%20spaces/data.zarr',
            "FAIL",
            "URL with percent-encoded characters"
        )
    
    def test_path_parsing_logic(self):
        """Test the specific path parsing logic fixes."""
        print("\n" + "="*60) 
        print("🔍 TESTING PATH PARSING LOGIC")
        print("="*60)
        
        # These should NOT be split at the colon after https
        test_cases = [
            ("https://example.com/data.zarr", "HTTPS URL should not split at protocol colon"),
            ("http://example.com/data.zarr", "HTTP URL should not split at protocol colon"),
            ("ftp://example.com/data.zarr", "FTP URL should not split at protocol colon"),
            ("s3://bucket/data.zarr", "S3 URL should not split at protocol colon"),
        ]
        
        for path, description in test_cases:
            self.test_path(path, "FAIL", description)
        
        # Windows drive letters should not confuse the parser
        if os.name == 'nt':
            self.test_path(
                "C:/data/test.zarr",
                "FAIL",
                "Windows drive letter should not confuse parser"
            )
        
        # These SHOULD be split at the subdataset colon  
        subdataset_cases = [
            ("/local/path/data.zarr:band1", "Local path WITH subdataset"),
            ('"/quoted/path/data.zarr":band1', "Quoted path WITH subdataset"),
        ]
        
        for path, description in subdataset_cases:
            self.test_path(path, "FAIL", description)
    
    def run_all_tests(self):
        """Run all test categories."""
        if not self.setup():
            return False
        
        print("🧪 Comprehensive EOPF-Zarr Path Testing Suite")
        print("="*60)
        
        self.test_local_paths()
        self.test_url_schemes()
        self.test_vsi_paths()
        self.test_subdataset_combinations()
        self.test_edge_cases()
        self.test_path_parsing_logic()
        
        self.print_summary()
        
        return True
    
    def print_summary(self):
        """Print a summary of all test results."""
        print("\n" + "="*60)
        print("📊 TEST SUMMARY")
        print("="*60)
        
        total_tests = len(self.test_results)
        opened_count = sum(1 for r in self.test_results if r['result'].startswith('✅'))
        failed_count = sum(1 for r in self.test_results if r['result'].startswith('❌'))
        
        print(f"Total tests: {total_tests}")
        print(f"Successfully opened: {opened_count}")
        print(f"Failed to open: {failed_count}")
        
        # Group results by category
        categories = {}
        for result in self.test_results:
            key = result['result']
            if key not in categories:
                categories[key] = []
            categories[key].append(result)
        
        for category, results in categories.items():
            print(f"\n{category} ({len(results)} tests):")
            for result in results[:5]:  # Show first 5 of each category
                print(f"  • {result['description']}")
            if len(results) > 5:
                print(f"  ... and {len(results) - 5} more")
        
        print("\n" + "="*60)
        print("🎯 KEY FINDINGS:")
        print("="*60)
        
        # Check for specific patterns we care about
        https_tests = [r for r in self.test_results if 'https://' in r['path']]
        if https_tests:
            https_success = [r for r in https_tests if not 'split' in r['details'].lower()]
            print(f"✅ HTTPS URL parsing: {len(https_success)}/{len(https_tests)} handled correctly")
        
        vsi_tests = [r for r in self.test_results if r['path'].startswith('/vsi')]
        if vsi_tests:
            print(f"📁 VSI paths tested: {len(vsi_tests)}")
        
        subdataset_tests = [r for r in self.test_results if ':' in r['path'] and not r['path'].startswith('http')]
        if subdataset_tests:
            print(f"📊 Subdataset combinations tested: {len(subdataset_tests)}")
    
    def cleanup(self):
        """Clean up after testing."""
        gdal.SetConfigOption("CPL_DEBUG", None)


def main():
    """Run the comprehensive path testing suite."""
    tester = PathTester()
    
    try:
        success = tester.run_all_tests()
        return 0 if success else 1
    finally:
        tester.cleanup()


if __name__ == "__main__":
    sys.exit(main())
