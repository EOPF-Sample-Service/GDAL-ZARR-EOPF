"""
Integration tests for Issue #166: Geocoding via Ground Control Points.

Tests that GCP-based geocoding (gdal.Warp) produces correctly georeferenced
output for Sentinel-1 GRD and SLC products.

Acceptance criteria:
  - Warping with GCPs produces output in EPSG:4326 (CRS:84 for lon/lat order)
  - Output bounds match the known product geographic extent
  - All standard resampling methods work (near, bilinear, cubic)
  - Warped data is non-trivial (non-empty, correct data type)
  - SLC burst subdatasets can also be geocoded
"""
import os
import sys
import tempfile
import pytest

try:
    from osgeo import gdal, osr

    gdal.UseExceptions()
except ImportError:
    gdal = None
    osr = None
    pytest.skip("GDAL not available", allow_module_level=True)

pytestmark = pytest.mark.require_driver("EOPFZARR")

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from test_urls import S1_GRD_VV_VH_URL, S1_SLC_URL

GRD_URL = "/vsicurl/" + S1_GRD_VV_VH_URL
SLC_URL = "/vsicurl/" + S1_SLC_URL

# -----------------------------------------------------------------------------
# Known geographic extent for the GRD product used in tests:
#   S1C_IW_GRDH_1SDV_20260205T120122 — Guatemala / southern Mexico
#   Verified against stac_discovery.bbox in the Zarr store.
# -----------------------------------------------------------------------------
GRD_EXPECTED_LON_MIN = -94.15
GRD_EXPECTED_LON_MAX = -91.42
GRD_EXPECTED_LAT_MIN = 14.04
GRD_EXPECTED_LAT_MAX = 16.67
# Tolerance: warped output may be slightly smaller than the full GCP extent
GRD_LON_TOL = 0.5   # degrees
GRD_LAT_TOL = 0.5   # degrees


def _check_url_accessible(url):
    """Return True if the EOPF dataset at url can be opened."""
    try:
        ds = gdal.Open(f'EOPFZARR:"{url}"')
        ok = ds is not None
        ds = None
        return ok
    except Exception:
        return False


def _skip_if_inaccessible(url, label=""):
    if not _check_url_accessible(url):
        pytest.skip(f"Remote data not accessible ({label}): {url[:80]}...")


def _open_grd_vv_subdataset():
    """Open the first VV GRD measurement subdataset."""
    gdal.UseExceptions()
    root = gdal.OpenEx(
        f'EOPFZARR:"{GRD_URL}"',
        gdal.OF_RASTER | gdal.OF_READONLY,
        open_options=["GRD_MULTIBAND=NO"],
    )
    assert root is not None, "Failed to open GRD root dataset"
    subdatasets = root.GetMetadata("SUBDATASETS")
    subds_name = None
    for k, v in subdatasets.items():
        if "_NAME" in k and "measurements/grd" in v:
            subds_name = v
            break
    root = None
    assert subds_name is not None, "No measurements/grd subdataset found"
    ds = gdal.Open(subds_name)
    assert ds is not None, f"Failed to open subdataset: {subds_name}"
    return ds


# =============================================================================
# TestWarpBoundsVerification
# =============================================================================

class TestWarpBoundsVerification:
    """
    Core acceptance criterion: warped output must land in the correct
    geographic region.

    The GRD product covers Guatemala / southern Mexico.  After gdal.Warp()
    with GCPs, the output bounding box must overlap that known region.

    This is how we confirm the data is shown in the right place:
    we compare the warped output's bounding box against the product's
    known geographic extent (from stac_discovery.bbox / EOPF metadata).
    """

    @pytest.fixture(scope="class")
    def warped_ds(self, tmp_path_factory):
        _skip_if_inaccessible(GRD_URL, "GRD warp bounds test")
        src = _open_grd_vv_subdataset()
        out = str(tmp_path_factory.mktemp("geocode") / "grd_wgs84.tif")
        warped = gdal.Warp(
            out, src,
            dstSRS="CRS:84",      # CRS:84 = lon/lat order (not lat/lon like EPSG:4326)
            width=256, height=0,   # low-res, preserves aspect ratio
            resampleAlg="near",
            format="GTiff",
        )
        src = None
        assert warped is not None, "gdal.Warp() returned None"
        yield warped
        warped = None

    def test_output_has_raster_data(self, warped_ds):
        """Warped output must have at least one band with valid data."""
        assert warped_ds.RasterCount >= 1
        assert warped_ds.RasterXSize > 0
        assert warped_ds.RasterYSize > 0

    def test_output_is_wgs84(self, warped_ds):
        """Warped output SRS must be geographic WGS84."""
        srs = osr.SpatialReference()
        srs.ImportFromWkt(warped_ds.GetProjectionRef())
        assert srs.IsGeographic(), "Output must be geographic CRS"
        # CRS:84 may report authority "CRS84" at top level rather than EPSG 6326;
        # verify by checking the semi-major axis (WGS84 = 6378137 m) instead.
        semi_major = srs.GetSemiMajor()
        assert abs(semi_major - 6378137.0) < 1.0, (
            f"Expected WGS84 semi-major axis ~6378137, got {semi_major:.0f}"
        )

    def test_output_lon_min_in_expected_range(self, warped_ds):
        """Output western edge must be near the known product lon_min."""
        gt = warped_ds.GetGeoTransform()
        lon_min = gt[0]
        assert lon_min > GRD_EXPECTED_LON_MIN - GRD_LON_TOL, (
            f"lon_min {lon_min:.4f} is too far west of expected {GRD_EXPECTED_LON_MIN}"
        )
        assert lon_min < GRD_EXPECTED_LON_MIN + GRD_LON_TOL, (
            f"lon_min {lon_min:.4f} is too far east of expected {GRD_EXPECTED_LON_MIN}"
        )

    def test_output_lon_max_in_expected_range(self, warped_ds):
        """Output eastern edge must be near the known product lon_max."""
        gt = warped_ds.GetGeoTransform()
        lon_max = gt[0] + warped_ds.RasterXSize * gt[1]
        assert lon_max > GRD_EXPECTED_LON_MAX - GRD_LON_TOL, (
            f"lon_max {lon_max:.4f} is too far west of expected {GRD_EXPECTED_LON_MAX}"
        )
        assert lon_max < GRD_EXPECTED_LON_MAX + GRD_LON_TOL, (
            f"lon_max {lon_max:.4f} is too far east of expected {GRD_EXPECTED_LON_MAX}"
        )

    def test_output_lat_min_in_expected_range(self, warped_ds):
        """Output southern edge must be near the known product lat_min."""
        gt = warped_ds.GetGeoTransform()
        lat_min = gt[3] + warped_ds.RasterYSize * gt[5]
        assert lat_min > GRD_EXPECTED_LAT_MIN - GRD_LAT_TOL, (
            f"lat_min {lat_min:.4f} is too far south of expected {GRD_EXPECTED_LAT_MIN}"
        )
        assert lat_min < GRD_EXPECTED_LAT_MIN + GRD_LAT_TOL, (
            f"lat_min {lat_min:.4f} is too far north of expected {GRD_EXPECTED_LAT_MIN}"
        )

    def test_output_lat_max_in_expected_range(self, warped_ds):
        """Output northern edge must be near the known product lat_max."""
        gt = warped_ds.GetGeoTransform()
        lat_max = gt[3]
        assert lat_max > GRD_EXPECTED_LAT_MAX - GRD_LAT_TOL, (
            f"lat_max {lat_max:.4f} is too far south of expected {GRD_EXPECTED_LAT_MAX}"
        )
        assert lat_max < GRD_EXPECTED_LAT_MAX + GRD_LAT_TOL, (
            f"lat_max {lat_max:.4f} is too far north of expected {GRD_EXPECTED_LAT_MAX}"
        )

    def test_output_bounds_consistent_with_gcp_extent(self, warped_ds):
        """
        The warped output bbox must be consistent with the GCP coordinate extent.

        We re-open the subdataset and compare GCP min/max coordinates against
        the warped output corners — this is the programmatic ground truth check.
        """
        _skip_if_inaccessible(GRD_URL, "GCP extent consistency test")
        src = _open_grd_vv_subdataset()
        gcps = src.GetGCPs()
        src = None

        gcp_lon_min = min(g.GCPX for g in gcps)
        gcp_lon_max = max(g.GCPX for g in gcps)
        gcp_lat_min = min(g.GCPY for g in gcps)
        gcp_lat_max = max(g.GCPY for g in gcps)

        gt = warped_ds.GetGeoTransform()
        w = warped_ds.RasterXSize
        h = warped_ds.RasterYSize
        out_lon_min = gt[0]
        out_lon_max = gt[0] + w * gt[1]
        out_lat_min = gt[3] + h * gt[5]
        out_lat_max = gt[3]

        # Warped output must not extend beyond GCP coverage + tolerance
        assert out_lon_min >= gcp_lon_min - GRD_LON_TOL
        assert out_lon_max <= gcp_lon_max + GRD_LON_TOL
        assert out_lat_min >= gcp_lat_min - GRD_LAT_TOL
        assert out_lat_max <= gcp_lat_max + GRD_LAT_TOL


# =============================================================================
# TestResamplingMethods
# =============================================================================

class TestResamplingMethods:
    """Verify that all standard resampling methods produce valid geocoded output."""

    @pytest.mark.parametrize("resample", ["near", "bilinear", "cubic"])
    def test_resampling_method(self, resample, tmp_path):
        _skip_if_inaccessible(GRD_URL, f"resampling {resample}")
        src = _open_grd_vv_subdataset()

        out = str(tmp_path / f"grd_{resample}.tif")
        warped = gdal.Warp(
            out, src,
            dstSRS="CRS:84",
            width=128, height=0,
            resampleAlg=resample,
            format="GTiff",
        )
        src = None
        assert warped is not None, f"Warp with resampleAlg='{resample}' returned None"

        gt = warped.GetGeoTransform()
        lon_min = gt[0]
        lon_max = gt[0] + warped.RasterXSize * gt[1]
        lat_min = gt[3] + warped.RasterYSize * gt[5]
        lat_max = gt[3]

        # Bounds must be in the expected region
        assert lon_min > GRD_EXPECTED_LON_MIN - GRD_LON_TOL, \
            f"{resample}: lon_min={lon_min:.4f} too far from expected"
        assert lon_max < GRD_EXPECTED_LON_MAX + GRD_LON_TOL, \
            f"{resample}: lon_max={lon_max:.4f} too far from expected"
        assert lat_min > GRD_EXPECTED_LAT_MIN - GRD_LAT_TOL, \
            f"{resample}: lat_min={lat_min:.4f} too far from expected"
        assert lat_max < GRD_EXPECTED_LAT_MAX + GRD_LAT_TOL, \
            f"{resample}: lat_max={lat_max:.4f} too far from expected"

        warped = None


# =============================================================================
# TestDataIntegrity
# =============================================================================

class TestDataIntegrity:
    """Verify that warped data retains valid backscatter values."""

    @pytest.fixture(scope="class")
    def warped_band(self, tmp_path_factory):
        _skip_if_inaccessible(GRD_URL, "data integrity test")
        src = _open_grd_vv_subdataset()
        out = str(tmp_path_factory.mktemp("integrity") / "grd_integrity.tif")
        warped = gdal.Warp(
            out, src,
            dstSRS="CRS:84",
            width=256, height=0,
            resampleAlg="near",
            format="GTiff",
        )
        src = None
        assert warped is not None
        yield warped.GetRasterBand(1), warped
        warped = None

    def test_data_type_is_uint16(self, warped_band):
        """GRD backscatter must remain UInt16 after warp."""
        band, _ = warped_band
        assert band.DataType == gdal.GDT_UInt16, (
            f"Expected UInt16, got {gdal.GetDataTypeName(band.DataType)}"
        )

    def test_data_has_non_zero_pixels(self, warped_band):
        """Warped data must contain non-zero (valid backscatter) pixels."""
        band, ds = warped_band
        try:
            data = band.ReadAsArray()
        except Exception:
            pytest.skip("ReadAsArray failed (possible NumPy ABI mismatch in CI)")
        if data is None:
            pytest.skip("ReadAsArray returned None")
        non_zero = (data > 0).sum()
        total = data.size
        non_zero_pct = 100.0 * non_zero / total
        assert non_zero_pct > 50.0, (
            f"Expected >50% non-zero pixels, got {non_zero_pct:.1f}%"
        )

    def test_data_values_in_valid_range(self, warped_band):
        """UInt16 GRD values must be in [0, 65535]."""
        band, ds = warped_band
        try:
            min_val, max_val, _, _ = band.ComputeStatistics(False)
        except Exception:
            pytest.skip("ComputeStatistics failed")
        assert 0.0 <= min_val <= 65535.0
        assert 0.0 <= max_val <= 65535.0
        assert max_val > 0.0, "Max value should be non-zero for valid SAR data"


# =============================================================================
# TestInMemoryWarp
# =============================================================================

class TestInMemoryWarp:
    """Verify that warping to /vsimem/ (in-memory) works (no disk I/O)."""

    def test_warp_to_vsimem(self):
        _skip_if_inaccessible(GRD_URL, "in-memory warp test")
        src = _open_grd_vv_subdataset()
        out = "/vsimem/test_geocoded.tif"
        warped = gdal.Warp(
            out, src,
            dstSRS="CRS:84",
            width=64, height=0,
            resampleAlg="near",
            format="GTiff",
        )
        src = None
        assert warped is not None, "In-memory warp failed"
        assert warped.RasterXSize == 64
        assert warped.RasterYSize > 0
        warped = None
        gdal.Unlink(out)


# =============================================================================
# TestSLCGeocodingViaBurst
# =============================================================================

class TestSLCGeocodingViaBurst:
    """SLC burst subdatasets can also be geocoded using their GCPs."""

    @pytest.fixture(scope="class")
    def slc_burst_ds(self):
        _skip_if_inaccessible(SLC_URL, "SLC burst geocoding test")
        ds = gdal.OpenEx(
            f'EOPFZARR:"{SLC_URL}"',
            gdal.OF_RASTER | gdal.OF_READONLY,
            open_options=["BURST=IW1_VV_001"],
        )
        assert ds is not None, "Failed to open SLC burst IW1_VV_001"
        yield ds
        ds = None

    def test_slc_burst_has_gcps(self, slc_burst_ds):
        """SLC burst subdataset must have GCPs before warp."""
        assert slc_burst_ds.GetGCPCount() > 0, "SLC burst should have GCPs"

    def test_slc_burst_warp_succeeds(self, slc_burst_ds, tmp_path):
        """gdal.Warp() on SLC burst with GCPs must succeed."""
        out = str(tmp_path / "slc_burst_wgs84.tif")
        warped = gdal.Warp(
            out, slc_burst_ds,
            dstSRS="CRS:84",
            width=128, height=0,
            resampleAlg="near",
            format="GTiff",
        )
        assert warped is not None, "Warp on SLC burst returned None"
        assert warped.RasterXSize > 0
        assert warped.RasterYSize > 0
        # SLC burst should be in a plausible geographic region
        gt = warped.GetGeoTransform()
        lon_origin = gt[0]
        lat_origin = gt[3]
        assert -180.0 <= lon_origin <= 180.0
        assert -90.0 <= lat_origin <= 90.0
        warped = None

    def test_slc_burst_output_is_geographic(self, slc_burst_ds, tmp_path):
        """Warped SLC burst output must have a geographic CRS."""
        out = str(tmp_path / "slc_burst_crs.tif")
        warped = gdal.Warp(
            out, slc_burst_ds,
            dstSRS="CRS:84",
            width=64, height=0,
            resampleAlg="near",
            format="GTiff",
        )
        assert warped is not None
        srs = osr.SpatialReference()
        srs.ImportFromWkt(warped.GetProjectionRef())
        assert srs.IsGeographic(), "Warped SLC output must be geographic CRS"
        warped = None


if __name__ == "__main__":
    pytest.main([__file__, "-v", "-s"])
