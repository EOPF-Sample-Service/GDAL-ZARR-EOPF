FROM ghcr.io/eopf-sample-service/eopf-container-images/eopf-zarr-driver:latest

USER root

# Add rioxarray (missing from base image, tracked in eopf-container-images#46)
RUN mamba install -y -c conda-forge rioxarray && mamba clean --all -f -y

# Rebuild EOPF-Zarr driver from latest main to pick up newest changes
RUN git clone --depth 1 https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git /tmp/eopf-zarr && \
    cd /tmp/eopf-zarr && \
    mkdir build && cd build && \
    export PATH="/opt/conda/bin:$PATH" && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="/opt/conda" \
        -DGDAL_INCLUDE_DIR="/opt/conda/include" \
        -DGDAL_LIBRARY="/opt/conda/lib/libgdal.so" \
        -DCMAKE_INSTALL_RPATH="/opt/conda/lib" \
        -DCMAKE_BUILD_RPATH="/opt/conda/lib" && \
    make -j$(nproc) && \
    cp gdal_EOPFZarr.so /opt/conda/lib/gdalplugins/ && \
    rm -rf /tmp/eopf-zarr

USER $NB_UID
