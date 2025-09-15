# EOPFZARR QGIS Docker Container - Pure Conda Build
# This Dockerfile creates a QGIS environment with EOPF-Zarr GDAL driver support
# Built using conda-forge packages to ensure GDAL version compatibility

FROM condaforge/mambaforge:24.9.2-0

LABEL description="Pure Conda EOPF-Zarr GDAL Driver with QGIS Demo"
LABEL version="conda-qgis-demo"

USER root

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

# Install minimal system dependencies (no QGIS from apt!)
RUN apt-get update && apt-get install -y \
    # Build tools for EOPF driver
    git \
    cmake \
    build-essential \
    pkg-config \
    # VNC and GUI support
    xvfb \
    x11vnc \
    tightvncserver \
    fluxbox \
    novnc \
    websockify \
    wget \
    # Window management and clipboard support
    wmctrl \
    xclip \
    xsel \
    autocutsel \
    # Additional X11 and graphics stability
    libgl1-mesa-glx \
    libglib2.0-0 \
    libxext6 \
    libxrender1 \
    libxtst6 \
    libxi6 \
    libxrandr2 \
    libxss1 \
    libgconf-2-4 \
    && rm -rf /var/lib/apt/lists/*

# Install everything via conda (GDAL + QGIS + Python packages + C++ compiler)
RUN mamba install -y -c conda-forge \
    'qgis>=3.44' \
    gxx_linux-64 \
    gcc_linux-64 \
    cmake \
    make \
    numpy \
    xarray \
    zarr \
    fsspec \
    dask \
    matplotlib \
    jupyterlab \
    notebook \
    && mamba clean --all --yes

# Create user for running services
RUN useradd -m -s /bin/bash jovyan && \
    mkdir -p /home/jovyan && \
    chown -R jovyan:jovyan /home/jovyan

# Set GDAL environment to conda locations
ENV GDAL_DRIVER_PATH=/opt/conda/lib/gdalplugins
ENV LD_LIBRARY_PATH="/opt/conda/lib:/opt/conda/lib64"
ENV PATH="/opt/conda/bin:$PATH"

# Verify GDAL version
RUN gdal-config --version && \
    echo "Checking GDAL version compatibility..." && \
    python -c "from osgeo import gdal; v=gdal.VersionInfo(); print(f'GDAL version: {v}'); assert int(v) >= 3100000, f'GDAL {v} too old, need >= 3.10.0'"

# Build EOPF-Zarr driver against conda GDAL using conda C++ compiler
RUN git clone --depth 1 https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git /tmp/eopf-zarr && \
    cd /tmp/eopf-zarr && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/conda \
        -DCMAKE_C_COMPILER=/opt/conda/bin/x86_64-conda-linux-gnu-gcc \
        -DCMAKE_CXX_COMPILER=/opt/conda/bin/x86_64-conda-linux-gnu-g++ \
        -DGDAL_INCLUDE_DIR=/opt/conda/include \
        -DGDAL_LIBRARY=/opt/conda/lib/libgdal.so && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/eopf-zarr

# Create start scripts for QGIS VNC demo
RUN cat > /tmp/start-qgis-demo.sh << 'EOF'
#!/bin/bash
echo "Starting QGIS Demo Environment..."

# Set up environment
export DISPLAY=:1
export GDAL_DRIVER_PATH=/opt/conda/lib/gdalplugins
export XDG_RUNTIME_DIR=/tmp/runtime-jovyan
export QT_X11_NO_MITSHM=1
export QT_XCB_GL_INTEGRATION=none
export LIBGL_ALWAYS_INDIRECT=1

# Create runtime directory with proper permissions
mkdir -p $XDG_RUNTIME_DIR
chmod 0700 $XDG_RUNTIME_DIR

# Start virtual display with larger resolution for better experience
Xvfb :1 -screen 0 1440x900x24 -ac +extension GLX +render -noreset &
sleep 5

echo "✅ X server started"

# Start window manager
fluxbox &
sleep 3

echo "✅ Window manager started"

# Start VNC server with clipboard support
x11vnc -display :1 \
       -nopw -listen localhost \
       -xkb -forever -shared \
       -noxdamage -ncache 10 \
       -bg &

# Give x11vnc time to start
sleep 3

echo "✅ VNC server started"

# Sync X11 CLIPBOARD with Qt/GTK apps using autocutsel
autocutsel -display :1 -fork &
autocutsel -display :1 -selection CLIPBOARD -fork &

sleep 1

# Start additional clipboard bridging with xclip monitoring
nohup bash -c 'while true; do xclip -selection clipboard -o 2>/dev/null | xclip -selection primary -i 2>/dev/null; xclip -selection primary -o 2>/dev/null | xclip -selection clipboard -i 2>/dev/null; sleep 1; done' &
sleep 1

echo "✅ Enhanced clipboard synchronization started"

# Start noVNC for web access
websockify --web=/usr/share/novnc/ 6080 localhost:5900 &
sleep 2

echo "✅ VNC ready at: http://localhost:6080/vnc.html"
echo "🎯 Starting QGIS with EOPF-Zarr driver..."

# Wait a bit more for X11 to be fully ready
sleep 2

# Start QGIS maximized with careful environment setup
DISPLAY=:1 QT_X11_NO_MITSHM=1 qgis --nologo &
QGIS_PID=$!
sleep 5

if ps -p $QGIS_PID > /dev/null; then
    echo "✅ QGIS started successfully (PID: $QGIS_PID)"
    # Try to maximize QGIS window after a short delay
    sleep 3
    DISPLAY=:1 wmctrl -r "QGIS" -b add,maximized_vert,maximized_horz 2>/dev/null || echo "Note: Window maximization optional"
else
    echo "❌ QGIS failed to start, trying fallback..."
    # Fallback: try with additional safety flags
    DISPLAY=:1 QT_X11_NO_MITSHM=1 QT_XCB_GL_INTEGRATION=none qgis --nologo --noplugins &
fi

echo "🎉 Demo environment ready!"
echo "📝 Access instructions:"
echo "   • Jupyter: http://localhost:8888"
echo "   • QGIS VNC: http://localhost:6080/vnc.html"
echo "   • Screen resolution: 1440x900"
echo "   • Clipboard: Use Ctrl+C/Ctrl+V in VNC interface"

# Keep container running
tail -f /dev/null
EOF

# Create clean start script without Windows line endings
RUN tr -d '\r' < /tmp/start-qgis-demo.sh > /usr/local/bin/start-qgis-demo.sh && \
    chmod +x /usr/local/bin/start-qgis-demo.sh && \
    rm /tmp/start-qgis-demo.sh

# Create Jupyter-only startup script
RUN cat > /tmp/start-jupyter.sh << 'EOF'
#!/bin/bash
echo "Starting JupyterLab..."
jupyter-lab --ip=0.0.0.0 --port=8888 --no-browser --allow-root --NotebookApp.token='' --NotebookApp.password=''
EOF

# Create clean Jupyter script
RUN tr -d '\r' < /tmp/start-jupyter.sh > /usr/local/bin/start-jupyter.sh && \
    chmod +x /usr/local/bin/start-jupyter.sh && \
    rm /tmp/start-jupyter.sh

# Set proper ownership
RUN chown -R jovyan:jovyan /home/jovyan

# Set working directory
WORKDIR /home/jovyan

# Verify EOPF-Zarr driver installation
RUN python -c "from osgeo import gdal; gdal.AllRegister(); driver = gdal.GetDriverByName('EOPFZarr'); print('✅ EOPF-Zarr driver loaded successfully!' if driver else '❌ EOPF-Zarr driver not found')"

# Expose ports
EXPOSE 8888 6080

# Default user
USER jovyan

# Default command - start Jupyter (users can override to start VNC demo)
CMD ["/usr/local/bin/start-jupyter.sh"]