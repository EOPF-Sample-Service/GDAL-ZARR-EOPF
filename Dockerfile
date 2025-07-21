# EOPF-Zarr GDAL Driver - Docker Image
# Based on Ubuntu 25 with GDAL 3.10 and EOPF Python environment
FROM ubuntu:25.04

LABEL maintainer="EOPF Sample Service"
LABEL description="EOPF-Zarr GDAL Driver with Ubuntu 25, GDAL 3.10, and EOPF Python environment"
LABEL version="1.0.0"

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONPATH=/usr/local/lib/python3.13/site-packages
ENV GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers
ENV GDAL_DATA=/usr/share/gdal
ENV PROJ_LIB=/usr/share/proj

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    curl \
    # GDAL and geospatial libraries
    gdal-bin \
    libgdal-dev \
    libproj-dev \
    libgeos-dev \
    # Python development
    python3 \
    python3-dev \
    python3-pip \
    # Additional utilities
    vim \
    htop \
    && rm -rf /var/lib/apt/lists/*

# Verify GDAL version (should be 3.10.x on Ubuntu 25)
RUN gdalinfo --version

# Install Python packages using system pip to ensure compatibility with system GDAL
RUN python3 -m pip install --no-cache-dir --break-system-packages \
    GDAL==$(gdal-config --version) \
    xarray \
    zarr==2.18.* \
    dask \
    geopandas \
    rasterio \
    fiona \
    shapely \
    pyproj \
    netcdf4 \
    h5py \
    scipy \
    matplotlib \
    cartopy \
    ipykernel \
    ipywidgets \
    jupyterlab \
    notebook

# Update PYTHONPATH to use system Python site-packages
ENV PYTHONPATH=/usr/local/lib/python3.13/site-packages

# Create directories for EOPF-Zarr driver
RUN mkdir -p /opt/eopf-zarr/drivers \
    && mkdir -p /opt/eopf-zarr/src

# Copy EOPF-Zarr source code
WORKDIR /opt/eopf-zarr
COPY . .

# Build EOPF-Zarr driver
RUN mkdir -p build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DGDAL_ROOT=/usr \
        -DGDAL_INCLUDE_DIR=/usr/include/gdal \
        -DGDAL_LIBRARY=/usr/lib/x86_64-linux-gnu/libgdal.so \
    && make -j$(nproc) \
    && cp gdal_EOPFZarr.so /opt/eopf-zarr/drivers/

# Verify driver installation
RUN ls -la /opt/eopf-zarr/drivers/ \
    && echo "GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers" >> /etc/environment

# Install additional JupyterHub packages for compatibility
RUN python3 -m pip install --no-cache-dir --break-system-packages \
    jupyterhub \
    notebook \
    jupyterlab \
    ipywidgets

# Create a non-root user for JupyterHub
RUN useradd -m -s /bin/bash eopfuser \
    && chown -R eopfuser:eopfuser /opt/eopf-zarr

# Switch to non-root user
USER eopfuser
WORKDIR /home/eopfuser

# Create test notebooks directory
RUN mkdir -p /home/eopfuser/notebooks

# Copy test notebooks
COPY --chown=eopfuser:eopfuser notebooks/ /home/eopfuser/notebooks/

# Expose JupyterLab port
EXPOSE 8888

# Set up entry point script
COPY --chown=eopfuser:eopfuser docker-entrypoint.sh /usr/local/bin/
USER root
RUN chmod +x /usr/local/bin/docker-entrypoint.sh
USER eopfuser

# Default command
CMD ["/usr/local/bin/docker-entrypoint.sh"]
