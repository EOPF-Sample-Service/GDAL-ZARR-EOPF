FROM ghcr.io/eopf-sample-service/eopf-container-images/eopf-zarr-driver:latest

USER root

# Add rioxarray (missing from base image, tracked in eopf-container-images#46).
# Pin sqlite/libsqlite so the re-solve cannot downgrade them: an older `sqlite`
# package overwrites libsqlite3.so.0 and breaks libgdal with
# "undefined symbol: sqlite3_total_changes64" (needs sqlite >= 3.37).
RUN mamba install -y -c conda-forge rioxarray 'sqlite>=3.45' 'libsqlite>=3.45' && \
    mamba clean --all -f -y

# Fail the build if the install left GDAL broken (catches sqlite/ABI mismatches)
RUN gdalinfo --version && \
    python -c "from osgeo import gdal; print('GDAL python OK:', gdal.__version__)"

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
    make -j$(nproc) gdal_EOPFZarr && \
    cp gdal_EOPFZarr.so /opt/conda/lib/gdalplugins/ && \
    rm -rf /tmp/eopf-zarr

# Fail the build if the EOPFZARR driver does not register
RUN python -c "from osgeo import gdal; gdal.AllRegister(); d = gdal.GetDriverByName('EOPFZARR'); assert d, 'EOPFZARR driver failed to load'; print('EOPFZARR driver OK:', d.GetDescription())"

USER $NB_UID
