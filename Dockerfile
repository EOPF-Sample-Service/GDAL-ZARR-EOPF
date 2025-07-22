# EOPF-Zarr GDAL Driver - Docker Image
# Based on Ubuntu 25.04 with GDAL and EOPF Python environment
FROM ubuntu:25.04

LABEL maintainer="EOPF Sample Service"
LABEL description="EOPF-Zarr GDAL Driver with Ubuntu 25.04, GDAL, and EOPF Python environment"
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

# Verify GDAL version (should be 3.9.x on Ubuntu 24.04)
RUN gdalinfo --version

# Debug GDAL installation
RUN echo "=== GDAL Debug Information ===" && \
    dpkg -l | grep gdal && \
    echo "GDAL config:" && \
    gdal-config --version && gdal-config --libs && \
    echo "GDAL library files:" && \
    find /usr -name "*gdal*" -type f 2>/dev/null | grep -E "(\.so|\.a)$" | head -10

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

# Create build and driver directories
RUN mkdir -p /opt/eopf-zarr/drivers /opt/eopf-zarr/build

# Clone and build EOPF-Zarr driver from source
WORKDIR /opt/eopf-zarr

# Copy only the essential source files for building
COPY src/ source/src/
COPY include/ source/include/
COPY tests/ source/tests/
COPY CMakeLists.txt source/CMakeLists.txt

# Build EOPF-Zarr driver  
WORKDIR /opt/eopf-zarr/build
RUN cmake ../source \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc) gdal_EOPFZarr \
    && echo "Build completed. Files in build directory:" \
    && ls -la \
    && echo "Copying driver to drivers directory:" \
    && cp gdal_EOPFZarr.so /opt/eopf-zarr/drivers/ \
    && echo "Driver installed successfully:" \
    && ls -la /opt/eopf-zarr/drivers/

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
RUN useradd -m -s /bin/bash jupyter \
    && echo "jupyter:jupyter" | chpasswd

# Create jupyter workspace directory
RUN mkdir -p /home/jupyter/work \
    && chown -R jupyter:jupyter /home/jupyter

# Copy docker entrypoint and test scripts
COPY docker-entrypoint.sh /usr/local/bin/
COPY test-environment.py /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh \
    && chmod +x /usr/local/bin/test-environment.py

# Switch to non-root user
USER jupyter
WORKDIR /home/jupyter

# Create test notebooks directory
RUN mkdir -p /home/jupyter/work/notebooks

# Copy test notebooks (if they exist) 
COPY notebooks/ /home/jupyter/work/notebooks/
RUN chown -R jupyter:jupyter /home/jupyter/work/notebooks/ 2>/dev/null || true

# Expose JupyterLab port
EXPOSE 8888

# Default command
CMD ["/usr/local/bin/docker-entrypoint.sh"]
