# EOPF-Zarr GDAL Driver - Docker Image
# Based on Ubuntu 25 with GDAL 3.10 and EOPF Python environment
FROM ubuntu:25.04

LABEL maintainer="EOPF Sample Service"
LABEL description="EOPF-Zarr GDAL Driver with Ubuntu 25, GDAL 3.10, EOPF Python environment, and JupyterHub compatibility"
LABEL version="2.1.0"

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

# Install compression libraries for Zarr/blosc support
RUN apt-get update && apt-get install -y \
    # Compression libraries that rasterio needs
    libblosc1 \
    libblosc-dev \
    python3-blosc \
    # Additional compression support
    liblz4-1 \
    liblz4-dev \
    libzstd1 \
    libzstd-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages using system pip to ensure compatibility with system GDAL
RUN python3 -m pip install --no-cache-dir --break-system-packages \
    GDAL==$(gdal-config --version) \
    # Install compression codecs FIRST
    blosc \
    numcodecs \
    # Then install geospatial packages
    xarray \
    zarr==2.18.* \
    dask \
    geopandas \
    # IMPORTANT: Install rasterio without binary to force compilation against system GDAL
    --no-binary=rasterio rasterio \
    rioxarray \
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

# Create directories for EOPF-Zarr driver
RUN mkdir -p /opt/eopf-zarr/drivers \
    && mkdir -p /opt/eopf-zarr/build

# Clone EOPF-Zarr driver from main repository
WORKDIR /opt/eopf-zarr
RUN git clone https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git source

# Build EOPF-Zarr driver from main repository source
WORKDIR /opt/eopf-zarr/build
RUN cmake ../source \
        -DCMAKE_BUILD_TYPE=Release \
        -DGDAL_ROOT=/usr \
        -DGDAL_INCLUDE_DIR=/usr/include/gdal \
        -DGDAL_LIBRARY=/usr/lib/x86_64-linux-gnu/libgdal.so \
    && make -j$(nproc) gdal_EOPFZarr \
    && echo "Build completed. Files in build directory:" \
    && ls -la \
    && echo "Copying driver to drivers directory:" \
    && cp gdal_EOPFZarr.so /opt/eopf-zarr/drivers/ \
    && echo "Driver installed successfully:" \
    && ls -la /opt/eopf-zarr/drivers/

# Clean up source code to reduce image size (optional)
RUN rm -rf /opt/eopf-zarr/source/.git \
    && rm -rf /opt/eopf-zarr/source/build \
    && rm -rf /opt/eopf-zarr/source/notebooks \
    && rm -rf /opt/eopf-zarr/source/docs

# Verify driver installation
RUN ls -la /opt/eopf-zarr/drivers/ \
    && echo "GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers" >> /etc/environment

# Install JupyterHub and single-user server components for compatibility
RUN python3 -m pip install --no-cache-dir --break-system-packages \
    jupyterhub \
    notebook \
    jupyterlab \
    ipywidgets \
    jupyter-server \
    jupyter-lsp

# Set up JupyterHub single-user environment variables
ENV NB_USER=jupyter
ENV NB_UID=1000
ENV NB_GID=100
ENV HOME=/home/jupyter
ENV SHELL=/bin/bash

# Create a non-root user compatible with JupyterHub single-user setup
RUN groupadd -f -g ${NB_GID} ${NB_USER} 2>/dev/null || true && \
    if id "${NB_USER}" 2>/dev/null; then \
        echo "User ${NB_USER} exists, modifying..."; \
        usermod -u ${NB_UID} -g ${NB_GID} -d /home/${NB_USER} -s ${SHELL} ${NB_USER}; \
    elif id "${NB_UID}" 2>/dev/null; then \
        echo "UID ${NB_UID} exists, recreating user..."; \
        existing_user=$(id -nu ${NB_UID}); \
        userdel ${existing_user}; \
        useradd -m -s ${SHELL} -u ${NB_UID} -g ${NB_GID} ${NB_USER}; \
    else \
        echo "Creating new user ${NB_USER}..."; \
        useradd -m -s ${SHELL} -u ${NB_UID} -g ${NB_GID} ${NB_USER}; \
    fi && \
    mkdir -p /home/${NB_USER} && \
    chown -R ${NB_USER}:${NB_GID} /home/${NB_USER} && \
    echo "${NB_USER}:${NB_USER}" | chpasswd

# Create jupyter workspace directory with proper permissions
RUN mkdir -p /home/jupyter/work \
    && chown -R ${NB_USER}:${NB_GID} /home/jupyter

# Set up environment for EOPF-Zarr driver for the jupyter user
RUN echo 'export GDAL_DRIVER_PATH=/opt/eopf-zarr/drivers' >> /home/jupyter/.bashrc \
    && echo 'export PYTHONPATH=/usr/local/lib/python3.13/site-packages:$PYTHONPATH' >> /home/jupyter/.bashrc

# Copy docker entrypoint and test scripts (these will be in the Docker repo)
COPY docker-entrypoint.sh /usr/local/bin/
COPY test-environment.py /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh \
    && chmod +x /usr/local/bin/test-environment.py

# Create test notebooks directory and copy example notebooks
RUN mkdir -p /home/jupyter/work/notebooks

# Copy example notebooks from Docker repo (not main repo)
COPY notebooks/ /home/jupyter/work/notebooks/

# Fix ownership for JupyterHub compatibility
RUN chown -R ${NB_USER}:${NB_GID} /home/jupyter/work/

# Add JupyterHub single-user entry points
RUN mkdir -p /usr/local/bin \
    && echo '#!/bin/bash' > /usr/local/bin/start-singleuser.sh \
    && echo 'source /home/jupyter/.bashrc' >> /usr/local/bin/start-singleuser.sh \
    && echo 'exec "$@"' >> /usr/local/bin/start-singleuser.sh \
    && chmod +x /usr/local/bin/start-singleuser.sh

# Switch to non-root user for JupyterHub compatibility
USER ${NB_USER}
WORKDIR ${HOME}

# Expose JupyterLab port
EXPOSE 8888

# Default command
CMD ["/usr/local/bin/docker-entrypoint.sh"]
