# EOPFZARR QGIS Docker Container

A Docker container providing QGIS with EOPFZARR GDAL driver support, accessible via VNC web interface.

Note: this image is for QGIS desktop use only. For JupyterHub or notebook use, see the main [Docker Quick Start](../../README.md#quick-start).

## Quick Start

```bash
# Run with QGIS + VNC
docker run -d --name eopf-qgis -p 6080:6080 \
  ghcr.io/eopf-sample-service/gdal-zarr-eopf:latest /usr/local/bin/start-qgis-demo.sh
```

Access QGIS at `http://localhost:6080/vnc.html`.

## Run with JupyterLab + QGIS

```bash
docker run -d --name eopf-full \
  -p 6080:6080 -p 8888:8888 \
  ghcr.io/eopf-sample-service/gdal-zarr-eopf:latest
```

Access:
- QGIS VNC: `http://localhost:6080/vnc.html`
- JupyterLab: `http://localhost:8888`

## Build Locally

```bash
cd docker-images/eopfzarr-qgis
docker build -t eopfzarr-qgis .
```
