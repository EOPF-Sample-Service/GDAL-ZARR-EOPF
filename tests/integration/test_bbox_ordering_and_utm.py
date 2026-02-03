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
    """Tests for UTM products without proj:bbox — root geotransform derived from geographic bbox"""

    def test_sentinel2_utm_root_geotransform(self):
        """
        Test that Sentinel-2 UTM root dataset has a valid geotransform.
        When proj:bbox is absent, the driver transforms stac_discovery.bbox
        (geographic) to UTM via OGR coordinate transformation.
        The resulting origin must be in a plausible UTM range — NOT the old
        buggy value (11M+ easting) and NOT the fallback pixel-coordinate origin (0,0).
        """
        skip_if_url_not_accessible(SENTINEL2_UTM_URL, "Sentinel-2 UTM test")

        path = f'EOPFZARR:"/vsicurl/{SENTINEL2_UTM_URL}"'
        ds = gdal.Open(path)

        assert ds is not None, "Failed to open Sentinel-2 dataset"

        # Check CRS is correct
        srs = ds.GetSpatialRef()
        assert srs is not None, "Spatial reference should exist"
        epsg_code = srs.GetAuthorityCode(None)
        assert epsg_code == "32634", f"Should be EPSG:32634 (UTM Zone 34N), got {epsg_code}"

        gt = ds.GetGeoTransform()
        assert gt is not None, "Geotransform must be present"

        origin_x = gt[0]
        origin_y = gt[3]
        pixel_w  = gt[1]
        pixel_h  = gt[5]

        # Must not be fallback pixel coordinates
        assert not (origin_x == 0.0 and origin_y == 0.0), \
            "Root geotransform must not be fallback (0,0) — geographic bbox should be transformed"

        # Must not be the old bug (degrees treated as meters → 11M+ easting)
        assert origin_x < 10_000_000, \
            f"UTM easting {origin_x} is invalid (bug: treating degrees as meters)"

        # Pixel sizes must be non-zero and positive width / negative height
        assert pixel_w > 0, f"Pixel width must be positive, got {pixel_w}"
        assert pixel_h < 0, f"Pixel height must be negative (north-up), got {pixel_h}"

        # utm_easting_min metadata must also be sane
        utm_east_min = ds.GetMetadataItem("utm_easting_min")
        assert utm_east_min is not None, "utm_easting_min metadata should be set"
        assert float(utm_east_min) < 10_000_000, \
            f"utm_easting_min {utm_east_min} is invalid"

        print(f"✅ Root UTM geotransform valid: origin=({origin_x:.0f}, {origin_y:.0f}), "
              f"pixel=({pixel_w:.2f}, {pixel_h:.2f})")

        ds = None

    def test_sentinel2_crs_and_projection(self):
        """
        Test that Sentinel-2 root dataset has correct projected CRS (UTM Zone 34N).
        """
        skip_if_url_not_accessible(SENTINEL2_UTM_URL, "Sentinel-2 CRS test")

        path = f'EOPFZARR:"/vsicurl/{SENTINEL2_UTM_URL}"'
        ds = gdal.Open(path)

        assert ds is not None, "Failed to open Sentinel-2 dataset"

        srs = ds.GetSpatialRef()
        assert srs is not None, "CRS should be set"
        assert srs.IsProjected(), "Should be projected CRS (UTM)"
        assert "UTM zone 34N" in srs.GetName() or srs.GetUTMZone() == 34, \
            "Should be UTM Zone 34N"

        print(f"✅ CRS correctly set: {srs.GetName()}")
        print(f"   EPSG: {srs.GetAuthorityCode(None)}")

        ds = None


# L1C product — tile T35SLB, UTM zone 35N.  Has x/y coordinate arrays at 10m and 60m.
SENTINEL2_L1C_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202602-s02msil1c-eu/03/products/cpm_v262/S2A_MSIL1C_20260203T092011_N0511_R050_T35SLB_20260203T111324.zarr"

# Standard Sentinel-2 MGRS tile origin for T35SLB (and all S2 tiles at this grid point)
# x/y coordinate arrays confirm: x[0]=300005 (10m), x[0]=300030 (60m) → UL edge = 300000
# y[0]=4199995 (10m), y[0]=4199970 (60m) → UL edge = 4200000
S2_TILE_ORIGIN_X = 300000.0
S2_TILE_ORIGIN_Y = 4200000.0


class TestSubdatasetGeoTransform:
    """Tests that subdataset geotransform is derived from sibling x/y coordinate arrays."""

    def _open_subdataset(self, subpath):
        path = f'EOPFZARR:"/vsicurl/{SENTINEL2_L1C_URL}":{subpath}'
        ds = gdal.Open(path)
        if ds is None:
            pytest.skip(f"Could not open subdataset: {subpath}")
        return ds

    def test_b02_10m_origin_and_pixel_size(self):
        """b02 at 10 m: origin must equal tile origin, pixel size must be (10, -10)."""
        skip_if_url_not_accessible(SENTINEL2_L1C_URL, "L1C b02 10m test")
        ds = self._open_subdataset("measurements/reflectance/r10m/b02")

        gt = ds.GetGeoTransform()
        assert gt is not None

        assert abs(gt[0] - S2_TILE_ORIGIN_X) < 1.0, \
            f"10m origin X should be {S2_TILE_ORIGIN_X}, got {gt[0]}"
        assert abs(gt[3] - S2_TILE_ORIGIN_Y) < 1.0, \
            f"10m origin Y should be {S2_TILE_ORIGIN_Y}, got {gt[3]}"
        assert abs(gt[1] - 10.0) < 0.01, \
            f"10m pixel width should be 10, got {gt[1]}"
        assert abs(gt[5] - (-10.0)) < 0.01, \
            f"10m pixel height should be -10, got {gt[5]}"

        assert ds.RasterXSize == 10980, f"10m band width should be 10980, got {ds.RasterXSize}"
        assert ds.RasterYSize == 10980, f"10m band height should be 10980, got {ds.RasterYSize}"

        print(f"✅ b02 10m: origin=({gt[0]}, {gt[3]}), pixel=({gt[1]}, {gt[5]}), size={ds.RasterXSize}x{ds.RasterYSize}")
        ds = None

    def test_b09_60m_origin_and_pixel_size(self):
        """b09 at 60 m: origin must equal tile origin, pixel size must be (60, -60)."""
        skip_if_url_not_accessible(SENTINEL2_L1C_URL, "L1C b09 60m test")
        ds = self._open_subdataset("measurements/reflectance/r60m/b09")

        gt = ds.GetGeoTransform()
        assert gt is not None

        assert abs(gt[0] - S2_TILE_ORIGIN_X) < 1.0, \
            f"60m origin X should be {S2_TILE_ORIGIN_X}, got {gt[0]}"
        assert abs(gt[3] - S2_TILE_ORIGIN_Y) < 1.0, \
            f"60m origin Y should be {S2_TILE_ORIGIN_Y}, got {gt[3]}"
        assert abs(gt[1] - 60.0) < 0.01, \
            f"60m pixel width should be 60, got {gt[1]}"
        assert abs(gt[5] - (-60.0)) < 0.01, \
            f"60m pixel height should be -60, got {gt[5]}"

        assert ds.RasterXSize == 1830, f"60m band width should be 1830, got {ds.RasterXSize}"
        assert ds.RasterYSize == 1830, f"60m band height should be 1830, got {ds.RasterYSize}"

        print(f"✅ b09 60m: origin=({gt[0]}, {gt[3]}), pixel=({gt[1]}, {gt[5]}), size={ds.RasterXSize}x{ds.RasterYSize}")
        ds = None

    def test_10m_and_60m_share_same_origin(self):
        """10 m and 60 m bands of the same tile must share the same UL origin."""
        skip_if_url_not_accessible(SENTINEL2_L1C_URL, "L1C origin consistency test")
        ds10 = self._open_subdataset("measurements/reflectance/r10m/b02")
        ds60 = self._open_subdataset("measurements/reflectance/r60m/b09")

        gt10 = ds10.GetGeoTransform()
        gt60 = ds60.GetGeoTransform()

        assert abs(gt10[0] - gt60[0]) < 1.0, \
            f"Origin X mismatch: 10m={gt10[0]}, 60m={gt60[0]}"
        assert abs(gt10[3] - gt60[3]) < 1.0, \
            f"Origin Y mismatch: 10m={gt10[3]}, 60m={gt60[3]}"

        print(f"✅ Shared origin: ({gt10[0]}, {gt10[3]})")
        ds10 = None
        ds60 = None

    def test_subdataset_corners_match_tile_extent(self):
        """60 m band lower-right corner must equal origin + 1830 * pixel_size."""
        skip_if_url_not_accessible(SENTINEL2_L1C_URL, "L1C 60m extent test")
        ds = self._open_subdataset("measurements/reflectance/r60m/b09")

        gt = ds.GetGeoTransform()
        expected_lr_x = gt[0] + ds.RasterXSize * gt[1]   # 300000 + 1830*60 = 409800
        expected_lr_y = gt[3] + ds.RasterYSize * gt[5]   # 4200000 + 1830*(-60) = 4090200

        assert abs(expected_lr_x - 409800.0) < 1.0, \
            f"60m LR X should be 409800, got {expected_lr_x}"
        assert abs(expected_lr_y - 4090200.0) < 1.0, \
            f"60m LR Y should be 4090200, got {expected_lr_y}"

        print(f"✅ 60m tile extent: UL=({gt[0]}, {gt[3]}), LR=({expected_lr_x}, {expected_lr_y})")
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
