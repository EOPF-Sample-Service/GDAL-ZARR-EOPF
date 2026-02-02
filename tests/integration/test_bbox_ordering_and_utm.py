#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
"""
Integration tests for Issue #135: EOPF bbox ordering and UTM geotransform fixes
Tests the two specific bugs that were fixed:
1. Non-standard EOPF bbox ordering [east,south,west,north]
2. UTM products without proj:bbox (should not have invalid geotransform)
"""

import os
import sys
import pytest
from pathlib import Path

try:
    from osgeo import gdal
    gdal.UseExceptions()
except ImportError:
    gdal = None
    pytest.skip("GDAL not available", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")

# Test URLs from Issue #135
SENTINEL3_SLSTR_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202601-s03slsrbt-eu/18/products/cpm_v262/S3A_SL_1_RBT____20260118T234920_20260118T235220_20260119T021734_0180_135_116_1080_PS1_O_NR_004.zarr"
SENTINEL2_UTM_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202602-s02msil2a-eu/02/products/cpm_v262/S2A_MSIL2A_20260202T094641_N0511_R036_T34UDC_20260202T104719.zarr"


def check_url_accessible(url, timeout=10):
    """Check if a URL is accessible"""
    try:
        test_path = f'EOPFZARR:"/vsicurl/{url}"'
        ds = gdal.Open(test_path)
        accessible = ds is not None
        if ds:
            ds = None
        return accessible
    except Exception:
        return False


def skip_if_url_not_accessible(url, test_name=""):
    """Skip test if URL is not accessible"""
    if not check_url_accessible(url):
        pytest.skip(f"Remote data not accessible for {test_name}: {url[:80]}...")


class TestBBoxOrdering:
    """Tests for EOPF non-standard bbox ordering fix"""
    
    def test_sentinel3_corner_coordinates(self):
        """
        Test that Sentinel-3 SLSTR corner coordinates are in correct order.
        """
        skip_if_url_not_accessible(SENTINEL3_SLSTR_URL, "Sentinel-3 SLSTR corners test")
        
        path = f'EOPFZARR:"/vsicurl/{SENTINEL3_SLSTR_URL}"'
        ds = gdal.Open(path)
        
        assert ds is not None, "Failed to open Sentinel-3 SLSTR dataset"
        
        gt = ds.GetGeoTransform()
        width = ds.RasterXSize
        height = ds.RasterYSize
        
        # Calculate corners
        ulx = gt[0]
        uly = gt[3]
        lrx = gt[0] + width * gt[1] + height * gt[2]
        lry = gt[3] + width * gt[4] + height * gt[5]
        
        # Upper left should be west and north
        # Lower right should be east and south
        assert ulx < lrx, "Upper left X should be west of lower right X"
        assert uly > lry, "Upper left Y should be north of lower right Y"
        
        print(f"✅ Corner coordinates correctly ordered:")
        print(f"   Upper Left: ({ulx:.4f}°, {uly:.4f}°)")
        print(f"   Lower Right: ({lrx:.4f}°, {lry:.4f}°)")
        
        ds = None


class TestUTMWithoutProjBbox:
    """Tests for UTM products without proj:bbox fix"""
    
    def test_sentinel2_utm_no_invalid_geotransform(self):
        """
        Test that Sentinel-2 UTM product without proj:bbox doesn't have invalid geotransform.
        Before fix: Driver would create default geographic bounds and transform to UTM,
        resulting in completely wrong coordinates (11M+ easting instead of ~500K).
        After fix: CRS is set but no geotransform (origin 0,0).
        """
        skip_if_url_not_accessible(SENTINEL2_UTM_URL, "Sentinel-2 UTM test")
        
        path = f'EOPFZARR:"/vsicurl/{SENTINEL2_UTM_URL}"'
        ds = gdal.Open(path)
        
        assert ds is not None, "Failed to open Sentinel-2 dataset"
        
        # Check CRS is correct
        srs = ds.GetSpatialRef()
        assert srs is not None, "Spatial reference should exist"
        epsg_code = srs.GetAuthorityCode(None)
        assert epsg_code == "32625", f"Should be EPSG:32625 (UTM Zone 25N), got {epsg_code}"
        
        # Check geotransform
        gt = ds.GetGeoTransform()
        
        if gt is not None:
            origin_x = gt[0]
            origin_y = gt[3]
            
            # After fix, should have no geotransform (0,0) OR valid UTM coordinates
            # Valid UTM Zone 25N easting is typically 166,000 to 834,000 meters
            # The bug produced 11,712,159 meters (completely invalid)
            
            if origin_x == 0.0 and origin_y == 0.0:
                # No geotransform set - acceptable for products without proj:bbox
                print(f"✅ No geotransform set (origin 0,0) - correct for UTM without proj:bbox")
            else:
                # If geotransform exists, verify it's NOT the buggy values
                assert origin_x < 10_000_000, \
                    f"UTM easting {origin_x} is invalid (bug: treating degrees as meters)"
                
                # Check if coordinates are in valid UTM range
                if 166_000 <= origin_x <= 834_000:
                    print(f"✅ Valid UTM coordinates: ({origin_x:.0f}m E, {origin_y:.0f}m N)")
                else:
                    print(f"⚠ Geotransform present but coordinates unusual: ({origin_x:.0f}m, {origin_y:.0f}m)")
        else:
            print(f"✅ No geotransform - correct for UTM without proj:bbox")
        
        # Check that metadata EPSG is correct
        epsg_meta = ds.GetMetadataItem("EPSG")
        assert epsg_meta == "32625", f"Metadata EPSG should be 32625, got {epsg_meta}"
        
        # Verify no invalid metadata from bug
        utm_east_min = ds.GetMetadataItem("utm_easting_min")
        if utm_east_min:
            utm_east_min = float(utm_east_min)
            assert utm_east_min < 10_000_000, \
                f"utm_easting_min {utm_east_min} is invalid (bug: treating degrees as meters)"
        
        ds = None
    
    def test_sentinel2_crs_without_geotransform(self):
        """
        Test that Sentinel-2 can have CRS set without geotransform.
        This is the expected behavior when proj:bbox is not available.
        """
        skip_if_url_not_accessible(SENTINEL2_UTM_URL, "Sentinel-2 CRS test")
        
        path = f'EOPFZARR:"/vsicurl/{SENTINEL2_UTM_URL}"'
        ds = gdal.Open(path)
        
        assert ds is not None, "Failed to open Sentinel-2 dataset"
        
        # Verify CRS is set
        srs = ds.GetSpatialRef()
        assert srs is not None, "CRS should be set even without geotransform"
        
        # Verify it's UTM Zone 25N
        assert srs.IsProjected(), "Should be projected CRS (UTM)"
        assert "UTM zone 25N" in srs.GetName() or srs.GetUTMZone() == 25, \
            "Should be UTM Zone 25N"
        
        print(f"✅ CRS correctly set: {srs.GetName()}")
        print(f"   EPSG: {srs.GetAuthorityCode(None)}")
        
        ds = None


class TestRegressionChecks:
    """Regression tests to ensure fixes don't break existing functionality"""
    
    def test_geographic_products_still_work(self):
        """Ensure geographic products (EPSG:4326) still work correctly"""
        skip_if_url_not_accessible(SENTINEL3_SLSTR_URL, "Geographic regression test")
        
        path = f'EOPFZARR:"/vsicurl/{SENTINEL3_SLSTR_URL}"'
        ds = gdal.Open(path)
        
        assert ds is not None, "Geographic products should still open"
        assert ds.GetGeoTransform() is not None, "Geographic products should have geotransform"
        
        srs = ds.GetSpatialRef()
        assert srs.GetAuthorityCode(None) == "4326", "Geographic CRS should be preserved"
        
        print(f"✅ Geographic products (EPSG:4326) still work correctly")
        
        ds = None
    
    def test_utm_products_open(self):
        """Ensure UTM products can still be opened"""
        skip_if_url_not_accessible(SENTINEL2_UTM_URL, "UTM regression test")
        
        path = f'EOPFZARR:"/vsicurl/{SENTINEL2_UTM_URL}"'
        ds = gdal.Open(path)
        
        assert ds is not None, "UTM products should still open"
        
        srs = ds.GetSpatialRef()
        assert srs is not None, "UTM products should have CRS"
        assert srs.IsProjected(), "UTM products should have projected CRS"
        
        print(f"✅ UTM products open and have correct projected CRS")
        
        ds = None


if __name__ == "__main__":
    pytest.main([__file__, "-v", "-s"])
