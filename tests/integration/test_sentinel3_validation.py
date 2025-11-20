import pytest
from osgeo import gdal
import os

# Enable GDAL exceptions
gdal.UseExceptions()

# Known Sentinel-3 Product URLs
OLCI_L1_EFR_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202508-s03olcefr/19/products/cpm_v256/S3B_OL_1_EFR____20250819T074058_20250819T074358_20250819T092155_0179_110_106_3420_ESA_O_NR_004.zarr"
SLSTR_L1_RBT_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202511-s03slsrbt-eu/17/products/cpm_v262/S3B_SL_1_RBT____20251117T132116_20251117T132416_20251117T153553_0179_113_238_1620_ESA_O_NR_004.zarr"
SLSTR_L2_LST_URL = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202511-s03slslst-eu/20/products/cpm_v262/S3A_SL_2_LST____20251120T124420_20251120T124720_20251120T150344_0179_133_038_1800_PS1_O_NR_004.zarr"

# Placeholders for missing products
# OLCI_L1_ERR_URL = "..."
# SLSTR_L2_WST_URL = "..."
# SLSTR_L2_FRP_URL = "..."
# SYNERGY_L2_SYN_URL = "..."

def get_eopf_path(url):
    return f'EOPFZARR:"/vsicurl/{url}"'

@pytest.mark.parametrize("product_name, url, expected_subdatasets", [
    ("OLCI_L1_EFR", OLCI_L1_EFR_URL, ["measurements/oa01_radiance", "conditions/geometry/latitude"]),
    ("SLSTR_L1_RBT", SLSTR_L1_RBT_URL, ["measurements/anadir/s1_radiance_an", "measurements/inadir/latitude"]),
    ("SLSTR_L2_LST", SLSTR_L2_LST_URL, ["measurements/lst", "measurements/latitude"]),
])
def test_open_sentinel3_product(product_name, url, expected_subdatasets):
    """
    Test opening Sentinel-3 products and verifying subdatasets exist.
    """
    path = get_eopf_path(url)
    print(f"Opening {product_name}: {path}")
    
    try:
        ds = gdal.Open(path)
        assert ds is not None, f"Failed to open {product_name}"
        
        subdatasets = ds.GetMetadata("SUBDATASETS")
        assert len(subdatasets) > 0, f"No subdatasets found for {product_name}"
        
        # Extract subdataset names
        subdataset_names = [value.split(":")[-1] for key, value in subdatasets.items() if key.endswith("_NAME")]
        
        # Verify expected subdatasets are present (partial match to handle full paths)
        for expected in expected_subdatasets:
            found = False
            for name in subdataset_names:
                if expected in name:
                    found = True
                    break
            assert found, f"Expected subdataset '{expected}' not found in {product_name}"
            
        # Basic metadata check
        assert ds.GetDriver().ShortName == "EOPFZARR"
        
    except Exception as e:
        pytest.fail(f"Exception opening {product_name}: {str(e)}")
    finally:
        ds = None

def test_slstr_l1_rbt_structure():
    """
    Specific test for SLSTR L1 RBT structure (views, stripes).
    """
    path = get_eopf_path(SLSTR_L1_RBT_URL)
    ds = gdal.Open(path)
    
    # Check for specific SLSTR L1 RBT bands (nadir/oblique, a/b/c/i/f stripes)
    # S1 to S6 are reflective, S7-S9 thermal, F1-F2 fire
    # Views: an (nadir), ao (oblique), etc.
    
    subdatasets = ds.GetMetadata("SUBDATASETS")
    subdataset_names = [value for key, value in subdatasets.items() if key.endswith("_NAME")]
    
    # Check for S1 nadir radiance
    s1_nadir = any("measurements/anadir/s1_radiance_an" in name for name in subdataset_names)
    assert s1_nadir, "S1 nadir radiance not found"
    
    # Check for geometry
    geo_nadir = any("measurements/inadir/latitude" in name for name in subdataset_names)
    assert geo_nadir, "Nadir geometry latitude not found"

    ds = None

def test_olci_l1_efr_geolocation():
    """
    Test geolocation arrays for OLCI L1 EFR.
    """
    path = get_eopf_path(OLCI_L1_EFR_URL)
    ds = gdal.Open(path)
    
    # Check for geolocation metadata
    metadata = ds.GetMetadata("")
    # Note: EOPF Zarr driver might expose geolocation via GEOLOCATION metadata domain or standard RPCs/GCPs
    # For now, just check if we can access the latitude/longitude arrays as subdatasets
    
    subdatasets = ds.GetMetadata("SUBDATASETS")
    subdataset_names = [value for key, value in subdatasets.items() if key.endswith("_NAME")]
    
    has_lat = any("measurements/latitude" in name for name in subdataset_names)
    has_lon = any("measurements/longitude" in name for name in subdataset_names)
    
    assert has_lat and has_lon, "Geolocation arrays (latitude/longitude) not found in OLCI L1 EFR"
    
    ds = None
