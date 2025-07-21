# EOPF-Zarr GDAL Driver - Docker Image
# Based on Ubuntu 25 with GDAL 3.10 and EOPF Python environment
FROM ubuntu:25.04

LABEL maintainer="EOPF Sample Service"
LABEL description="EOPF-Zarr GDAL Driver with Ubuntu 25, GDAL 3.10, and EOPF Python environment"
LABEL version="1.0.0"

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONPATH=/usr/local/lib/python3.11/site-packages:/opt/conda/lib/python3.11/site-packages
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
    python3.11 \
    python3.11-dev \
    python3-pip \
    # Additional utilities
    vim \
    htop \
    && rm -rf /var/lib/apt/lists/*

# Verify GDAL version (should be 3.10.x on Ubuntu 25)
RUN gdalinfo --version

# Install Miniconda for better Python environment management
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O /tmp/miniconda.sh \
    && bash /tmp/miniconda.sh -b -p /opt/conda \
    && rm /tmp/miniconda.sh \
    && /opt/conda/bin/conda clean -ay

# Add conda to PATH
ENV PATH="/opt/conda/bin:$PATH"

# Create conda environment with EOPF requirements
COPY environment.yml /tmp/environment.yml
RUN conda env create -f /tmp/environment.yml \
    && conda clean -ay

# Activate the eopf-zarr environment by default
ENV CONDA_DEFAULT_ENV=eopf-zarr
ENV PATH="/opt/conda/envs/eopf-zarr/bin:$PATH"

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

# Install JupyterLab and additional packages for JupyterHub compatibility
RUN /opt/conda/envs/eopf-zarr/bin/pip install \
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
