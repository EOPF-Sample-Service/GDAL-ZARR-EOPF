# GDAL EOPF-Zarr Plugin

[![Build Status](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/main.yml/badge.svg)](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/main.yml)
[![Build and Publish Docker Image](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/docker-publish.yml/badge.svg)](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/docker-publish.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A GDAL driver plugin for reading EOPF (Earth Observation Processing Framework) Zarr datasets, including Sentinel-1, Sentinel-2, and Sentinel-3 products.

## Features

- **QGIS integration** — works with "Add Raster Layer" using the `EOPFZARR:` prefix
- **Sentinel-1 GRD** — multi-band polarization (VV/VH or HH/HV) via `GRD_MULTIBAND=YES`
- **Sentinel-1 SLC** — burst selection via `BURST=IW1_VV_001`
- **Geocoding via GCPs** — ground control points from `conditions/gcp/` arrays for accurate georeferencing
- **Metadata extraction** — STAC discovery properties exposed in the `EOPF` metadata domain
- **Cloud native** — HTTP/HTTPS and virtual file system (`/vsicurl/`) support
- **Cross-platform** — Windows, macOS, Linux

## Quick Start

### Docker (Recommended)

```bash
docker pull yuvraj1989/eopf-zarr-driver:latest
docker run -p 8888:8888 yuvraj1989/eopf-zarr-driver:latest
```

Access JupyterLab at `http://localhost:8888`. The image includes GDAL 3.10, the EOPFZARR driver, rasterio, rioxarray, and jupyterhub-singleuser.

### Build from Source

Requirements: GDAL 3.10+, CMake 3.16+, C++17 compiler.

```bash
git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
cd GDAL-ZARR-EOPF
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo cp gdal_EOPFZarr.so $(gdal-config --plugindir)/
```

Verify:

```bash
gdalinfo --formats | grep EOPFZARR
```

## Usage

### Command Line

```bash
# List subdatasets
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/product.zarr"'

# Open a specific subdataset
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04'

# Sentinel-1 GRD multi-band (VV+VH)
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr"' --oo GRD_MULTIBAND=YES

# Sentinel-1 SLC burst selection
gdalinfo 'EOPFZARR:"/vsicurl/https://example.com/S1_SLC.zarr"' --oo BURST=IW1_VV_001

# Geocode via GCPs
gdalwarp -geoloc -t_srs CRS:84 'EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr":measurements/ew/hh_grd' output.tif
```

### Python

```python
from osgeo import gdal

# Open dataset and list subdatasets
ds = gdal.Open('EOPFZARR:"/vsicurl/https://example.com/product.zarr"')
subdatasets = ds.GetMetadata("SUBDATASETS")

# Open subdataset directly
sub = gdal.Open('EOPFZARR:"/vsicurl/https://example.com/product.zarr":measurements/reflectance/r10m/b04')
array = sub.ReadAsArray()

# Sentinel-1 GRD multi-band
ds = gdal.OpenEx('EOPFZARR:"/vsicurl/https://example.com/S1_GRD.zarr"',
                 open_options=["GRD_MULTIBAND=YES"])

# Sentinel-1 SLC burst
ds = gdal.OpenEx('EOPFZARR:"/vsicurl/https://example.com/S1_SLC.zarr"',
                 open_options=["BURST=IW1_VV_001"])

# Read EOPF metadata
meta = ds.GetMetadata("EOPF")
print(meta.get("EOPF_PRODUCT_TYPE"))
```

### Open Options

| Option | Values | Description |
|---|---|---|
| `GRD_MULTIBAND` | `YES` / `NO` (default) | Combine VV+VH or HH+HV polarizations into a 2-band dataset |
| `BURST` | e.g. `IW1_VV_001` | Select a specific SLC burst by name |

### Environment Variables

| Variable | Values | Description |
|---|---|---|
| `EOPF_SHOW_ZARR_ERRORS` | `YES` / `NO` (default) | Show Zarr driver error messages (useful for debugging) |

## Notebooks

See [`notebooks/`](notebooks/) for Jupyter notebooks covering all features:

| Notebook | Description |
|---|---|
| `01-Basic-Functionality-Demo.ipynb` | Driver registration, subdatasets, basic reads |
| `03-EOPF-Zarr-Test.ipynb` | Environment validation |
| `04-Explore_sentinel2_EOPFZARR.ipynb` | Sentinel-2 product exploration |
| `07-Sentinel-3-OLCI-Level-1-EFR.ipynb` | Sentinel-3 OLCI data |
| `08-EOPFZARR-with-Rioxarray.ipynb` | rioxarray integration |
| `09-EOPFZARR-with-Rasterio.ipynb` | rasterio integration |
| `11-Sentinel-1-GRD-Demo.ipynb` | Sentinel-1 GRD access and GCPs |
| `12-Sentinel-1-GRD-MultiBand-Demo.ipynb` | GRD multi-band polarization |
| `13-Sentinel-1-SLC-Burst-Selection.ipynb` | SLC burst selection |

## Documentation

- [Developer Guide](DEVELOPER.md) — building, testing, contributing
- [Usage Guide](USAGE.md) — examples and best practices
- [Troubleshooting](TROUBLESHOOTING.md) — common issues and solutions

## License

MIT License — see [LICENSE](LICENSE) for details.
