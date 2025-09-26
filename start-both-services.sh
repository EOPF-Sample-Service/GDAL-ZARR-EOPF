#!/bin/bash
# Custom startup script to run both QGIS/VNC and JupyterLab
set -e

echo "🚀 Starting EOPF-Zarr QGIS + JupyterLab Environment..."

# Set environment variables
export DISPLAY=:1
export QT_QPA_PLATFORM=offscreen

# Start Xvfb (virtual framebuffer)
echo "📺 Starting X server..."
Xvfb :1 -screen 0 1440x900x24 -ac +extension GLX +render -noreset &
sleep 2

# Start window manager
echo "🖼️ Starting window manager..."
fluxbox &
sleep 1

# Start VNC server
echo "🔗 Starting VNC server..."
x11vnc -display :1 -nopw -listen localhost -xkb -forever -shared -noxdamage -ncache 10 -bg
sleep 1

# Start clipboard synchronization
echo "📋 Starting enhanced clipboard..."
autocutsel -display :1 -fork
autocutsel -display :1 -selection CLIPBOARD -fork
# Bidirectional clipboard sync
(
    while true; do
        xclip -selection clipboard -o 2>/dev/null | xclip -selection primary -i 2>/dev/null || true
        xclip -selection primary -o 2>/dev/null | xclip -selection clipboard -i 2>/dev/null || true
        sleep 1
    done
) &

# Start websockify for noVNC
echo "🌐 Starting WebSocket proxy..."
websockify --web=/usr/share/novnc/ 6080 localhost:5900 &
sleep 1

# Start QGIS in background
echo "🗺️ Starting QGIS..."
qgis --nologo &
QGIS_PID=$!

# Start JupyterLab
echo "📊 Starting JupyterLab..."
cd /home/jovyan
jupyter lab \
    --ip=0.0.0.0 \
    --port=8888 \
    --no-browser \
    --notebook-dir=work \
    --ServerApp.token='' \
    --ServerApp.password='' \
    --ServerApp.allow_origin='*' \
    --ServerApp.allow_remote_access=True &
JUPYTER_PID=$!

echo "✅ All services started!"
echo "🌐 Access URLs:"
echo "   • QGIS VNC: http://localhost:6080/vnc.html"
echo "   • JupyterLab: http://localhost:8888"
echo "📋 Enhanced clipboard: Ctrl+C/Ctrl+V works bidirectionally"
echo ""
echo "Press Ctrl+C to stop all services"

# Wait for services
wait $QGIS_PID $JUPYTER_PID