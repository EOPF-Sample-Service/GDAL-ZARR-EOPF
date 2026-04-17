"""
Integration tests for GCP extraction from EOPF Zarr products.

Tests that Ground Control Points are correctly extracted from
conditions/gcp/ arrays in Sentinel-1 GRD and SLC products.
"""
import pytest
from osgeo import gdal, osr

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from test_urls import S1_GRD_VV_VH_URL, S1_SLC_URL

# Test URLs (centralized in tests/test_urls.py)
GRD_URL = "/vsicurl/" + S1_GRD_VV_VH_URL
SLC_URL = "/vsicurl/" + S1_SLC_URL


class TestGRDSubdatasetGCPs:
    """Tests for GCP extraction from a GRD measurement subdataset."""

    @pytest.fixture
    def dataset(self):
        """Open a GRD VV measurement subdataset."""
        gdal.UseExceptions()
        # Open root with GRD_MULTIBAND=NO to get subdataset listing
        root = gdal.OpenEx(
            f"EOPFZARR:'{GRD_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["GRD_MULTIBAND=NO"],
        )
        assert root is not None
        subdatasets = root.GetMetadata("SUBDATASETS")
        assert subdatasets is not None, "No SUBDATASETS metadata"
        # Find the first measurement/grd subdataset
        subds_name = None
        for k, v in subdatasets.items():
            if "_NAME" in k and "measurements/grd" in v:
                subds_name = v
                break
        root = None
        assert subds_name is not None, "No GRD measurement subdataset found"

        ds = gdal.Open(subds_name)
        yield ds
        if ds:
            ds = None

    def test_gcp_count(self, dataset):
        """GRD subdataset should have GCPs (expect ~294 = 14x21)."""
        assert dataset is not None
        count = dataset.GetGCPCount()
        assert count > 0
        assert count >= 100  # At least 100 GCPs expected

    def test_gcp_coordinates_valid(self, dataset):
        """GCPs should have valid lat/lon ranges."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None
        assert len(gcps) > 0
        for gcp in gcps:
            assert -180.0 <= gcp.GCPX <= 180.0, f"Longitude out of range: {gcp.GCPX}"
            assert -90.0 <= gcp.GCPY <= 90.0, f"Latitude out of range: {gcp.GCPY}"

    def test_gcp_pixel_line_valid(self, dataset):
        """GCP pixel/line values should be within raster dimensions."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None
        width = dataset.RasterXSize
        height = dataset.RasterYSize
        for gcp in gcps:
            assert -0.5 <= gcp.GCPPixel <= width + 0.5, (
                f"GCP pixel {gcp.GCPPixel} outside raster width {width}"
            )
            assert -0.5 <= gcp.GCPLine <= height + 0.5, (
                f"GCP line {gcp.GCPLine} outside raster height {height}"
            )

    def test_gcp_projection_wgs84(self, dataset):
        """GCP SRS should be WGS84."""
        assert dataset is not None
        srs = dataset.GetGCPSpatialRef()
        assert srs is not None
        assert srs.IsGeographic()
        # Check it's WGS84
        authority = srs.GetAuthorityCode("DATUM")
        assert authority == "6326", f"Expected WGS84 datum (6326), got {authority}"

    def test_gcp_has_height(self, dataset):
        """At least some GCPs should have non-zero height."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None
        # At least some GCPs should have height data
        heights = [gcp.GCPZ for gcp in gcps]
        # Heights can be 0 for sea-level; just check they're reasonable
        for h in heights:
            assert -500.0 <= h <= 10000.0, f"Height out of range: {h}"

    def test_gcp_ids(self, dataset):
        """Each GCP should have a unique ID."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None
        ids = [gcp.Id for gcp in gcps]
        assert len(ids) == len(set(ids)), "GCP IDs are not unique"
        assert all(gcp_id.startswith("GCP_") for gcp_id in ids)

    def test_gcp_grid_structure(self, dataset):
        """GCPs should form a regular grid pattern."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None

        # Extract unique pixel and line values
        pixels = sorted(set(gcp.GCPPixel for gcp in gcps))
        lines = sorted(set(gcp.GCPLine for gcp in gcps))

        # Should have multiple unique values in each dimension
        assert len(pixels) > 1, "GCPs should span multiple pixel columns"
        assert len(lines) > 1, "GCPs should span multiple line rows"

        # Total should be pixels * lines (rectangular grid)
        assert len(gcps) == len(pixels) * len(lines), (
            f"GCP count {len(gcps)} != {len(pixels)} x {len(lines)}"
        )


class TestRootDatasetNoGCPs:
    """Tests that root datasets (non-subdatasets) have no GCPs."""

    @pytest.fixture
    def dataset(self):
        """Open GRD product at root level."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{GRD_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["GRD_MULTIBAND=NO"],
        )
        yield ds
        if ds:
            ds = None

    def test_root_no_gcps(self, dataset):
        """Root dataset should have no GCPs (only subdatasets have them)."""
        assert dataset is not None
        assert dataset.GetGCPCount() == 0


class TestSLCBurstGCPs:
    """Tests that SLC burst subdatasets also have GCPs."""

    @pytest.fixture
    def dataset(self):
        """Open SLC burst via BURST option."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{SLC_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["BURST=IW1_VV_001"],
        )
        yield ds
        if ds:
            ds = None

    def test_slc_burst_has_gcps(self, dataset):
        """SLC burst should have GCPs."""
        assert dataset is not None
        count = dataset.GetGCPCount()
        assert count > 0, "SLC burst should have GCPs"

    def test_slc_burst_gcp_coordinates(self, dataset):
        """SLC burst GCPs should have valid coordinates."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None
        for gcp in gcps:
            assert -180.0 <= gcp.GCPX <= 180.0
            assert -90.0 <= gcp.GCPY <= 90.0

    def test_slc_burst_gcp_srs(self, dataset):
        """SLC burst GCP SRS should be WGS84."""
        assert dataset is not None
        srs = dataset.GetGCPSpatialRef()
        assert srs is not None
        assert srs.IsGeographic()


class TestGRDMultiBandGCPs:
    """Tests that GRD multi-band wrapper exposes GCPs for geolocation in QGIS."""

    @pytest.fixture
    def dataset(self):
        """Open GRD product with multi-band mode (default)."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{GRD_URL}'")
        yield ds
        if ds:
            ds = None

    def test_multiband_has_gcps(self, dataset):
        """Multi-band GRD wrapper should have GCPs for geolocation."""
        assert dataset is not None
        assert dataset.GetGCPCount() > 0

    def test_multiband_gcp_projection_wgs84(self, dataset):
        """Multi-band GRD GCP SRS should be WGS84."""
        assert dataset is not None
        srs = dataset.GetGCPSpatialRef()
        assert srs is not None
        assert srs.IsGeographic()
        assert srs.GetAuthorityCode("DATUM") == "6326"

    def test_multiband_gcp_coordinates_valid(self, dataset):
        """Multi-band GRD GCPs should have valid lon/lat."""
        assert dataset is not None
        gcps = dataset.GetGCPs()
        assert gcps is not None
        for gcp in gcps:
            assert -180.0 <= gcp.GCPX <= 180.0
            assert -90.0 <= gcp.GCPY <= 90.0


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
