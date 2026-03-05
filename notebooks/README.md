# Notebooks

Jupyter notebooks demonstrating EOPFZARR driver capabilities with real Sentinel data.

## Available Notebooks

| Notebook | Description |
|---|---|
| `01-Basic-Functionality-Demo.ipynb` | Driver registration, subdatasets, basic reads |
| `02-Remote-Data-Access-Demo.ipynb` | Remote data access via `/vsicurl/` |
| `03-EOPF-Zarr-Test.ipynb` | Environment validation and driver testing |
| `04-Explore_sentinel2_EOPFZARR.ipynb` | Sentinel-2 MSI L1C product exploration |
| `06-Data-Visualization.ipynb` | Data visualization techniques |
| `07-Sentinel-3-OLCI-Level-1-EFR.ipynb` | Sentinel-3 OLCI Level-1 EFR data |
| `08-EOPFZARR-with-Rioxarray.ipynb` | rioxarray integration |
| `09-EOPFZARR-with-Rasterio.ipynb` | rasterio integration |
| `10-Sentinel-3-Multi-Product-Demo.ipynb` | Multiple Sentinel-3 product types |
| `11-Sentinel-1-GRD-Demo.ipynb` | Sentinel-1 GRD access and GCPs |
| `12-Sentinel-1-GRD-MultiBand-Demo.ipynb` | GRD multi-band polarization (VV+VH) |
| `13-Sentinel-1-SLC-Burst-Selection.ipynb` | SLC burst selection |

## Running the Notebooks

### Docker (Recommended)

```bash
docker pull yuvraj1989/eopf-zarr-driver:latest
docker run -p 8888:8888 yuvraj1989/eopf-zarr-driver:latest
```

Access JupyterLab at `http://localhost:8888`.

### Local Installation

See [DEVELOPER.md](../DEVELOPER.md) for build instructions and [TROUBLESHOOTING.md](../TROUBLESHOOTING.md) for common issues.
