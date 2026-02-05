"""
Integration tests for Sentinel-1 GRD multi-band polarization support.

Tests the GRD_MULTIBAND open option which combines VV/VH or HH/HV
polarization subdatasets into a single multi-band dataset.
"""
import pytest
from osgeo import gdal

# Try to import numpy, mark as None if unavailable
try:
    import numpy as np
except ImportError:
    np = None

# Test URLs for GRD products with dual polarization
GRD_VV_VH_URL = (
    "/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:"
    "202602-s01siwgrh-global/05/products/cpm_v262/"
    "S1C_IW_GRDH_1SDV_20260205T120122_20260205T120158_006220_00C7E4_5D6E.zarr"
)

GRD_HH_HV_URL = (
    "/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:"
    "202602-s01sewgrm-global/05/products/cpm_v262/"
    "S1A_EW_GRDM_1SDH_20260205T132815_20260205T132849_063084_07EADA_F486.zarr"
)


class TestGRDMultiBandVVVH:
    """Tests for VV/VH dual polarization GRD products."""

    @pytest.fixture
    def dataset(self):
        """Open VV/VH GRD product with multi-band mode (default)."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{GRD_VV_VH_URL}'")
        yield ds
        if ds:
            ds = None

    def test_band_count(self, dataset):
        """Multi-band GRD should have 2 bands (VV and VH)."""
        assert dataset is not None
        assert dataset.RasterCount == 2

    def test_band_descriptions(self, dataset):
        """Band descriptions should be VV and VH."""
        assert dataset is not None
        band1 = dataset.GetRasterBand(1)
        band2 = dataset.GetRasterBand(2)
        assert band1.GetDescription() == "VV"
        assert band2.GetDescription() == "VH"

    def test_data_type(self, dataset):
        """GRD bands should be UInt16."""
        assert dataset is not None
        for i in range(1, dataset.RasterCount + 1):
            band = dataset.GetRasterBand(i)
            assert band.DataType == gdal.GDT_UInt16

    def test_metadata_multiband_flag(self, dataset):
        """Metadata should indicate multi-band mode."""
        assert dataset is not None
        assert dataset.GetMetadataItem("EOPF_MULTIBAND") == "YES"
        assert dataset.GetMetadataItem("EOPF_PRODUCT_TYPE") == "GRD"

    def test_metadata_polarizations(self, dataset):
        """Metadata should list polarizations."""
        assert dataset is not None
        pols = dataset.GetMetadataItem("EOPF_POLARIZATIONS")
        assert pols is not None
        assert "VV" in pols
        assert "VH" in pols

    def test_read_data(self, dataset):
        """Should be able to read data from both bands."""
        assert dataset is not None
        for i in range(1, dataset.RasterCount + 1):
            band = dataset.GetRasterBand(i)
            try:
                data = band.ReadAsArray(0, 0, 10, 10)
                assert data is not None
                assert data.shape == (10, 10)
            except ImportError as e:
                if "numpy.core.multiarray failed to import" in str(e):
                    pytest.skip(f"NumPy compatibility issue: {e}")
                else:
                    raise

    def test_dimensions_match(self, dataset):
        """Both polarization bands should have same dimensions."""
        assert dataset is not None
        width = dataset.RasterXSize
        height = dataset.RasterYSize
        assert width > 0
        assert height > 0
        # Both bands come from the same size arrays
        for i in range(1, dataset.RasterCount + 1):
            band = dataset.GetRasterBand(i)
            assert band.XSize == width
            assert band.YSize == height


class TestGRDMultiBandHHHV:
    """Tests for HH/HV dual polarization GRD products."""

    @pytest.fixture
    def dataset(self):
        """Open HH/HV GRD product with multi-band mode (default)."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{GRD_HH_HV_URL}'")
        yield ds
        if ds:
            ds = None

    def test_band_count(self, dataset):
        """Multi-band GRD should have 2 bands (HH and HV)."""
        assert dataset is not None
        assert dataset.RasterCount == 2

    def test_band_descriptions(self, dataset):
        """Band descriptions should be HH and HV."""
        assert dataset is not None
        band1 = dataset.GetRasterBand(1)
        band2 = dataset.GetRasterBand(2)
        assert band1.GetDescription() == "HH"
        assert band2.GetDescription() == "HV"

    def test_metadata_polarizations(self, dataset):
        """Metadata should list HH/HV polarizations."""
        assert dataset is not None
        pols = dataset.GetMetadataItem("EOPF_POLARIZATIONS")
        assert pols is not None
        assert "HH" in pols
        assert "HV" in pols


class TestGRDMultiBandDisabled:
    """Tests for GRD products with multi-band mode disabled."""

    @pytest.fixture
    def dataset(self):
        """Open GRD product with multi-band mode disabled."""
        gdal.UseExceptions()
        ds = gdal.OpenEx(
            f"EOPFZARR:'{GRD_VV_VH_URL}'",
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["GRD_MULTIBAND=NO"],
        )
        yield ds
        if ds:
            ds = None

    def test_subdatasets_available(self, dataset):
        """With multi-band disabled, subdatasets should be available."""
        assert dataset is not None
        subdatasets = dataset.GetMetadata("SUBDATASETS")
        assert subdatasets is not None
        # Should have subdatasets for measurements/grd
        subdataset_names = [
            v for k, v in subdatasets.items() if "_NAME" in k
        ]
        grd_subdatasets = [s for s in subdataset_names if "measurements/grd" in s]
        assert len(grd_subdatasets) >= 2  # At least VV and VH

    def test_no_multiband_metadata(self, dataset):
        """Without multi-band mode, EOPF_MULTIBAND should not be YES."""
        assert dataset is not None
        multiband = dataset.GetMetadataItem("EOPF_MULTIBAND")
        # Should be None or not "YES"
        assert multiband != "YES"


class TestGRDDataIntegrity:
    """Tests for data integrity in multi-band GRD products."""

    @pytest.fixture
    def dataset(self):
        """Open VV/VH GRD product."""
        gdal.UseExceptions()
        ds = gdal.Open(f"EOPFZARR:'{GRD_VV_VH_URL}'")
        yield ds
        if ds:
            ds = None

    def test_no_all_zeros(self, dataset):
        """Data should not be all zeros (indicates read failure)."""
        assert dataset is not None
        # Read a sample from each band
        for i in range(1, dataset.RasterCount + 1):
            band = dataset.GetRasterBand(i)
            # Read from center of image to avoid potential edge NoData
            xoff = dataset.RasterXSize // 2
            yoff = dataset.RasterYSize // 2
            try:
                data = band.ReadAsArray(xoff, yoff, 100, 100)
                # At least some values should be non-zero
                assert np.any(data != 0), f"Band {i} has all zeros"
            except ImportError as e:
                if "numpy.core.multiarray failed to import" in str(e):
                    pytest.skip(f"NumPy compatibility issue: {e}")
                else:
                    raise

    def test_valid_data_range(self, dataset):
        """UInt16 data should be in valid range."""
        assert dataset is not None
        for i in range(1, dataset.RasterCount + 1):
            band = dataset.GetRasterBand(i)
            try:
                data = band.ReadAsArray(0, 0, 100, 100)
                assert data.min() >= 0
                assert data.max() <= 65535
            except ImportError as e:
                if "numpy.core.multiarray failed to import" in str(e):
                    pytest.skip(f"NumPy compatibility issue: {e}")
                else:
                    raise

    def test_vv_vh_are_different(self, dataset):
        """VV and VH bands should contain different data."""
        assert dataset is not None
        band1 = dataset.GetRasterBand(1)
        band2 = dataset.GetRasterBand(2)

        try:
            data1 = band1.ReadAsArray(0, 0, 100, 100)
            data2 = band2.ReadAsArray(0, 0, 100, 100)

            # The bands should not be identical
            assert not np.array_equal(data1, data2), "VV and VH bands are identical"
        except ImportError as e:
            if "numpy.core.multiarray failed to import" in str(e):
                pytest.skip(f"NumPy compatibility issue: {e}")
            else:
                raise


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
