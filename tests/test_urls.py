"""
Centralized test dataset URLs.

All remote URLs used in integration tests are defined here.
When EODC rotates buckets or URLs change, update THIS FILE ONLY.

Bucket types:
  - notebook-data: Stable bucket, does not rotate with CPM releases.
  - 202602-...:    Release bucket, rotates each month/release.

When possible, prefer the stable notebook-data bucket.
"""

# =============================================================================
# EODC base
# =============================================================================
_EODC = "https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a"

# =============================================================================
# Sentinel-1 SLC  (stable notebook-data bucket)
# =============================================================================
S1_SLC_URL = (
    f"{_EODC}:notebook-data/tutorial_data/cpm_v262/"
    "S1C_IW_SLC__1SDV_20251016T165627_20251016T165654_004590_00913B_30C4.zarr"
)

# =============================================================================
# Sentinel-1 GRD  (release bucket — update when rotated)
# =============================================================================
S1_GRD_VV_VH_URL = (
    f"{_EODC}:202602-s01siwgrh-global/05/products/cpm_v262/"
    "S1C_IW_GRDH_1SDV_20260205T120122_20260205T120158_006220_00C7E4_5D6E.zarr"
)

S1_GRD_HH_HV_URL = (
    f"{_EODC}:202602-s01sewgrm-global/05/products/cpm_v262/"
    "S1A_EW_GRDM_1SDH_20260205T132815_20260205T132849_063084_07EADA_F486.zarr"
)

# =============================================================================
# Sentinel-2  (release bucket — update when rotated)
# =============================================================================
S2_L2A_URL = (
    f"{_EODC}:202602-s02msil2a-eu/02/products/cpm_v262/"
    "S2A_MSIL2A_20260202T094641_N0511_R036_T34UDC_20260202T104719.zarr"
)

S2_L1C_URL = (
    f"{_EODC}:202602-s02msil1c-eu/03/products/cpm_v262/"
    "S2A_MSIL1C_20260203T092011_N0511_R050_T35SLB_20260203T111324.zarr"
)

# =============================================================================
# Sentinel-3  (release bucket — update when rotated)
# =============================================================================
S3_OLCI_L1_EFR_URL = (
    f"{_EODC}:202602-s03olcefr-eu/02/products/cpm_v262/"
    "S3B_OL_1_EFR____20260202T115259_20260202T115559_"
    "20260202T135532_0179_116_180_2160_ESA_O_NR_004.zarr"
)

S3_SLSTR_L1_RBT_URL = (
    f"{_EODC}:202601-s03slsrbt-eu/18/products/cpm_v262/"
    "S3A_SL_1_RBT____20260118T234920_20260118T235220_"
    "20260119T021734_0180_135_116_1080_PS1_O_NR_004.zarr"
)

S3_SLSTR_L1_RBT_URL_B = (
    f"{_EODC}:202601-s03slsrbt-eu/18/products/cpm_v262/"
    "S3B_SL_1_RBT____20260118T231041_20260118T231341_"
    "20260119T014624_0180_115_358_1080_ESA_O_NR_004.zarr"
)

S3_SLSTR_L2_LST_URL = (
    f"{_EODC}:202602-s03slslst-eu/02/products/cpm_v262/"
    "S3A_SL_2_LST____20260202T123441_20260202T123741_"
    "20260202T144630_0179_135_323_2340_PS1_O_NR_004.zarr"
)

# =============================================================================
# Derived URLs used in tests (subdataset paths)
# =============================================================================
S2_L2A_SUBDATASET_URL = S2_L2A_URL + "/measurements/reflectance/r60m/b01"
S2_L2A_MASK_SUBDATASET_URL = S2_L2A_URL + "/conditions/mask/detector_footprint/r10m/b04"

# =============================================================================
# Complete list for health-check validation
# =============================================================================
ALL_URLS = {
    "S1_SLC": S1_SLC_URL,
    "S1_GRD_VV_VH": S1_GRD_VV_VH_URL,
    "S1_GRD_HH_HV": S1_GRD_HH_HV_URL,
    "S2_L2A": S2_L2A_URL,
    "S2_L1C": S2_L1C_URL,
    "S3_OLCI_L1_EFR": S3_OLCI_L1_EFR_URL,
    "S3_SLSTR_L1_RBT": S3_SLSTR_L1_RBT_URL,
    "S3_SLSTR_L1_RBT_B": S3_SLSTR_L1_RBT_URL_B,
    "S3_SLSTR_L2_LST": S3_SLSTR_L2_LST_URL,
}
