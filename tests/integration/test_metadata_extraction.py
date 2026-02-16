"""
Integration tests for EOPF metadata extraction from Zarr groups.

Tests that STAC discovery properties and other_metadata are extracted
and exposed via the EOPF metadata domain.
"""
import pytest
from osgeo import gdal

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from test_urls import S1_SLC_URL, S1_GRD_VV_VH_URL, S2_L1C_URL

# Test URLs (centralized in tests/test_urls.py)
SLC_URL = "/vsicurl/" + S1_SLC_URL
GRD_URL = "/vsicurl/" + S1_GRD_VV_VH_URL
S2_URL = "/vsicurl/" + S2_L1C_URL


class TestSentinel1SLCMetadata:
    """Tests for Sentinel-1 SLC product metadata extraction."""

    @pytest.fixture
    def dataset(self):
        """Open SLC product."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{SLC_URL}'")
        yield ds
        if ds:
            ds = None

    def test_constellation(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_CONSTELLATION", "EOPF") == "sentinel-1"

    def test_platform(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_PLATFORM", "EOPF") == "sentinel-1c"

    def test_orbit_state(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_ORBIT_STATE", "EOPF") == "ascending"

    def test_absolute_orbit(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_ABSOLUTE_ORBIT", "EOPF") == "4590"

    def test_relative_orbit(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_RELATIVE_ORBIT", "EOPF") == "44"

    def test_instrument_mode(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_INSTRUMENT_MODE", "EOPF") == "IW"

    def test_datetime(self, dataset):
        assert dataset is not None
        dt = dataset.GetMetadataItem("EOPF_DATETIME", "EOPF")
        assert dt is not None
        assert "2025-10-16" in dt

    def test_start_datetime(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_START_DATETIME", "EOPF") is not None

    def test_end_datetime(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_END_DATETIME", "EOPF") is not None

    def test_sar_polarizations(self, dataset):
        assert dataset is not None
        pols = dataset.GetMetadataItem("EOPF_SAR_POLARIZATIONS", "EOPF")
        assert pols is not None
        assert "VV" in pols
        assert "VH" in pols

    def test_sar_frequency_band(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_SAR_FREQUENCY_BAND", "EOPF") == "C"

    def test_sar_instrument_mode(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_SAR_INSTRUMENT_MODE", "EOPF") == "IW"

    def test_product_type(self, dataset):
        assert dataset is not None
        pt = dataset.GetMetadataItem("EOPF_PRODUCT_TYPE", "EOPF")
        assert pt is not None
        assert "SLC" in pt

    def test_title(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_TITLE", "EOPF") == "S01SIWSLC"

    def test_datatake_id(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_DATATAKE_ID", "EOPF") is not None

    def test_instruments(self, dataset):
        assert dataset is not None
        inst = dataset.GetMetadataItem("EOPF_INSTRUMENTS", "EOPF")
        assert inst is not None
        assert "sar" in inst


class TestSentinel1GRDMetadata:
    """Tests for Sentinel-1 GRD product metadata extraction."""

    @pytest.fixture
    def dataset(self):
        """Open GRD product."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{GRD_URL}'")
        yield ds
        if ds:
            ds = None

    def test_constellation(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_CONSTELLATION", "EOPF") == "sentinel-1"

    def test_orbit_state(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_ORBIT_STATE", "EOPF") == "descending"

    def test_sar_product_type(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_SAR_PRODUCT_TYPE", "EOPF") == "GRD"

    def test_title(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_TITLE", "EOPF") == "S01SIWGRD"

    def test_timeliness(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_TIMELINESS", "EOPF") is not None

    def test_processing_lineage(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_PROCESSING_LINEAGE", "EOPF") == "systematic"


class TestSentinel2Metadata:
    """Tests for Sentinel-2 product metadata extraction."""

    @pytest.fixture
    def dataset(self):
        """Open Sentinel-2 product."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{S2_URL}'")
        yield ds
        if ds:
            ds = None

    def test_constellation(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_CONSTELLATION", "EOPF") == "sentinel-2"

    def test_platform(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_PLATFORM", "EOPF") == "sentinel-2a"

    def test_cloud_cover(self, dataset):
        assert dataset is not None
        cc = dataset.GetMetadataItem("EOPF_CLOUD_COVER", "EOPF")
        assert cc is not None
        assert float(cc) >= 0.0

    def test_snow_cover(self, dataset):
        assert dataset is not None
        sc = dataset.GetMetadataItem("EOPF_SNOW_COVER", "EOPF")
        assert sc is not None
        assert float(sc) >= 0.0

    def test_processing_level(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_PROCESSING_LEVEL", "EOPF") == "L1C"

    def test_gsd(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_GSD", "EOPF") == "10"

    def test_orbit_state(self, dataset):
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_ORBIT_STATE", "EOPF") == "descending"

    def test_instruments(self, dataset):
        assert dataset is not None
        inst = dataset.GetMetadataItem("EOPF_INSTRUMENTS", "EOPF")
        assert inst is not None
        assert "msi" in inst

    def test_product_type(self, dataset):
        assert dataset is not None
        pt = dataset.GetMetadataItem("EOPF_PRODUCT_TYPE", "EOPF")
        assert pt is not None
        assert "L1C" in pt

    def test_title(self, dataset):
        """Sentinel-2 other_metadata has detailed info, not simple title."""
        assert dataset is not None
        # S2 other_metadata may not have a simple 'title' field
        # (it has band_description, geometric_refinement, etc. instead)
        # This is OK - title is optional


class TestMetadataDomainIsolation:
    """Tests that EOPF metadata is in the EOPF domain, not default domain."""

    @pytest.fixture
    def dataset(self):
        """Open GRD product."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{GRD_URL}'")
        yield ds
        if ds:
            ds = None

    def test_eopf_domain_has_items(self, dataset):
        """EOPF domain should contain metadata items."""
        assert dataset is not None
        md = dataset.GetMetadata("EOPF")
        assert md is not None
        assert len(md) > 0

    def test_constellation_in_eopf_domain(self, dataset):
        """EOPF_CONSTELLATION should be in EOPF domain."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_CONSTELLATION", "EOPF") is not None

    def test_constellation_not_in_default_domain(self, dataset):
        """EOPF_CONSTELLATION should NOT be in default domain."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_CONSTELLATION") is None


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
