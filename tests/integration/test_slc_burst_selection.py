"""
Integration tests for Sentinel-1 SLC burst selection via BURST open option.

Tests the BURST open option which allows selecting a specific burst from
Sentinel-1 SLC products using a friendly name like IW1_VV_001.
"""
import pytest
from osgeo import gdal

# Try to import numpy, mark as None if unavailable
try:
    import numpy as np
except ImportError:
    np = None

# Test URL for SLC product with dual polarization (IW mode, VV/VH)
SLC_URL = (
    "/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:"
    "notebook-data/tutorial_data/cpm_v262/"
    "S1C_IW_SLC__1SDV_20251016T165627_20251016T165654_004590_00913B_30C4.zarr"
)

# GRD URL for testing BURST option on non-SLC product
GRD_URL = (
    "/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:"
    "202602-s01siwgrh-global/05/products/cpm_v262/"
    "S1C_IW_GRDH_1SDV_20260205T120122_20260205T120158_006220_00C7E4_5D6E.zarr"
)


class TestSLCBurstSelection:
    """Tests for selecting individual SLC bursts via the BURST open option."""

    @pytest.fixture
    def dataset(self):
        """Open SLC product with BURST=IW1_VV_001."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{SLC_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["BURST=IW1_VV_001"],
        )
        yield ds
        if ds:
            ds = None

    def test_burst_selection_opens(self, dataset):
        """BURST=IW1_VV_001 should open successfully."""
        assert dataset is not None

    def test_single_band(self, dataset):
        """A burst selection should produce a single-band dataset."""
        assert dataset is not None
        assert dataset.RasterCount == 1

    def test_burst_dimensions(self, dataset):
        """Burst should have valid raster dimensions."""
        assert dataset is not None
        assert dataset.RasterXSize > 0
        assert dataset.RasterYSize > 0

    def test_burst_data_type(self, dataset):
        """SLC burst should be CFloat32 (complex)."""
        assert dataset is not None
        band = dataset.GetRasterBand(1)
        assert band.DataType == gdal.GDT_CFloat32

    def test_burst_metadata_product_type(self, dataset):
        """Metadata should indicate SLC product type."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_PRODUCT_TYPE") == "SLC"

    def test_burst_metadata_name(self, dataset):
        """Metadata should contain the burst friendly name."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_BURST_NAME") == "IW1_VV_001"

    def test_burst_metadata_subswath(self, dataset):
        """Metadata should contain the burst subswath."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_BURST_SUBSWATH") == "IW1"

    def test_burst_metadata_polarization(self, dataset):
        """Metadata should contain the burst polarization."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_BURST_POLARIZATION") == "VV"

    def test_burst_metadata_index(self, dataset):
        """Metadata should contain the burst index."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_BURST_INDEX") == "1"

    def test_read_data(self, dataset):
        """Should be able to read data from the burst."""
        assert dataset is not None
        band = dataset.GetRasterBand(1)
        try:
            data = band.ReadAsArray(0, 0, 10, 10)
            assert data is not None
            assert data.shape == (10, 10)
        except ImportError as e:
            if "numpy.core.multiarray failed to import" in str(e):
                pytest.skip(f"NumPy compatibility issue: {e}")
            else:
                raise


class TestSLCBurstCaseInsensitive:
    """Tests that burst name matching is case-insensitive."""

    @pytest.fixture
    def dataset(self):
        """Open SLC product with lowercase burst name."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{SLC_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["BURST=iw1_vv_001"],
        )
        yield ds
        if ds:
            ds = None

    def test_case_insensitive_opens(self, dataset):
        """Lowercase burst name should open successfully."""
        assert dataset is not None

    def test_case_insensitive_metadata(self, dataset):
        """Metadata should use canonical uppercase naming."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_BURST_NAME") == "IW1_VV_001"


class TestSLCDifferentSubswaths:
    """Tests that bursts from different subswaths are accessible."""

    @pytest.fixture(params=["IW1_VV_001", "IW2_VV_001", "IW3_VV_001"])
    def dataset(self, request):
        """Open SLC burst from different subswaths."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{SLC_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=[f"BURST={request.param}"],
        )
        yield ds, request.param
        if ds:
            ds = None

    def test_subswath_opens(self, dataset):
        """Each subswath burst should open successfully."""
        ds, burst_name = dataset
        assert ds is not None, f"Failed to open burst {burst_name}"

    def test_subswath_metadata(self, dataset):
        """Each burst should have correct subswath in metadata."""
        ds, burst_name = dataset
        assert ds is not None
        expected_subswath = burst_name.split("_")[0]  # IW1, IW2, IW3
        assert ds.GetMetadataItem("EOPF_BURST_SUBSWATH") == expected_subswath


class TestSLCDifferentPolarizations:
    """Tests that bursts from different polarizations are accessible."""

    @pytest.fixture(params=["IW1_VV_001", "IW1_VH_001"])
    def dataset(self, request):
        """Open SLC burst from different polarizations."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{SLC_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=[f"BURST={request.param}"],
        )
        yield ds, request.param
        if ds:
            ds = None

    def test_polarization_opens(self, dataset):
        """Each polarization burst should open successfully."""
        ds, burst_name = dataset
        assert ds is not None, f"Failed to open burst {burst_name}"

    def test_polarization_metadata(self, dataset):
        """Each burst should have correct polarization in metadata."""
        ds, burst_name = dataset
        assert ds is not None
        expected_pol = burst_name.split("_")[1]  # VV or VH
        assert ds.GetMetadataItem("EOPF_BURST_POLARIZATION") == expected_pol


class TestSLCInvalidBurst:
    """Tests for error handling with invalid burst names."""

    def test_invalid_burst_fails(self):
        """An invalid burst name should fail to open."""
        gdal.UseExceptions()
        with pytest.raises(Exception):
            gdal.OpenEx(
                f"EOPFZARR:'{SLC_URL}'",
                gdal.OF_RASTER | gdal.OF_READONLY,
                open_options=["BURST=INVALID_BURST_NAME"],
            )


class TestSLCNoBurstOption:
    """Tests that without BURST option, normal subdataset listing works."""

    @pytest.fixture
    def dataset(self):
        """Open SLC product without BURST option."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{SLC_URL}'")
        yield ds
        if ds:
            ds = None

    def test_no_burst_has_subdatasets(self, dataset):
        """Without BURST option, subdatasets should be listed."""
        assert dataset is not None
        subdatasets = dataset.GetMetadata("SUBDATASETS")
        assert subdatasets is not None
        subdataset_names = [
            v for k, v in subdatasets.items() if "_NAME" in k
        ]
        # SLC product should have many subdatasets (54 bursts worth)
        slc_subdatasets = [s for s in subdataset_names if "measurements/slc" in s]
        assert len(slc_subdatasets) >= 10


class TestSLCBurstOnGRD:
    """Tests that BURST option on a GRD product is ignored gracefully."""

    @pytest.fixture
    def dataset(self):
        """Open GRD product with BURST option (should be ignored)."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{GRD_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["BURST=IW1_VV_001", "GRD_MULTIBAND=NO"],
        )
        yield ds
        if ds:
            ds = None

    def test_grd_with_burst_opens(self, dataset):
        """GRD product should still open when BURST is specified."""
        assert dataset is not None

    def test_grd_no_burst_metadata(self, dataset):
        """GRD product should not have SLC burst metadata."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_PRODUCT_TYPE") != "SLC"
        assert dataset.GetMetadataItem("EOPF_BURST_NAME") is None


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
