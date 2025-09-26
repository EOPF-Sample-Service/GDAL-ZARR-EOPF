# EOPFZARR QGIS Docker Container

A Docker container providing QGIS with EOPFZARR GDAL driver support, accessible via VNC web interface and Jupyter Lab. This container uses a pure conda-forge approach to ensure GDAL version compatibility.

## 🌟 Features

- **QGIS 3.44.2** with GDAL 3.11.3 from conda-forge
- **EOPFZARR GDAL driver** for remote cloud-optimized geospatial data access
- **VNC web interface** (noVNC) for remote QGIS access
- **Enhanced clipboard support** for copy/paste between host and container
- **JupyterLab** for Python geospatial analysis
- **1440x900 resolution** for optimal screen utilization
- **Pure conda environment** eliminates package conflicts

## 🚀 Quick Start

### Option A: QGIS + VNC Only (Pre-built Image)

Pull and run the container with QGIS and VNC interface:

```bash
# Pull from Docker Hub and run with QGIS + VNC
docker run -d --name eopf-qgis-vnc -p 6080:6080 \
  yuvraj1989/eopf-qgis-conda:latest /usr/local/bin/start-qgis-demo.sh
```

**Access:**
- **QGIS VNC Interface**: <http://localhost:6080/vnc.html>

### Option A+: QGIS + VNC + JupyterLab (Both Services)

To run both QGIS/VNC and JupyterLab simultaneously, use the custom startup script:

```bash
# Download the custom startup script
curl -O https://raw.githubusercontent.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/main/start-both-services.sh
chmod +x start-both-services.sh

# Run with both services
docker run -d --name eopf-full-demo \
  -p 6080:6080 -p 8888:8888 \
  -v $(pwd)/start-both-services.sh:/usr/local/bin/start-both-services.sh \
  yuvraj1989/eopf-qgis-conda:latest /usr/local/bin/start-both-services.sh
```

**Access:**
- **QGIS VNC Interface**: <http://localhost:6080/vnc.html>
- **JupyterLab**: <http://localhost:8888>

### Option B: Build Locally

If you want to build from source:

```bash
# Clone repository
git clone https://github.com/EOPF-Sample-Service/eopf-container-images.git
cd eopf-container-images/eopfzarr-docker-image/eopf-qgis-pure

# Build with cache for faster rebuilds
docker build --cache-from yuvraj1989/eopf-qgis-conda:latest -t eopf-qgis-conda .

# Run locally built image
docker run -d --name eopf-qgis-vnc -p 6080:6080 -p 8888:8888 \
  eopf-qgis-conda /usr/local/bin/start-qgis-demo.sh
```

### Alternative: JupyterLab Only

Start the container with JupyterLab only:

```bash
# From Docker Hub
docker run -d --name eopf-jupyter -p 8888:8888 yuvraj1989/eopf-qgis-conda:latest

# Or locally built
docker run -d --name eopf-jupyter -p 8888:8888 eopf-qgis-conda
```

**Access:**
- **JupyterLab**: <http://localhost:8888>

### Docker Compose (Alternative)

Create a `docker-compose.yml` file:

```yaml
version: '3.8'
services:
  eopf-qgis-vnc:
    image: yuvraj1989/eopf-qgis-conda:latest  # Use Docker Hub image
    container_name: eopf-qgis-vnc
    ports:
      - "6080:6080"  # VNC web interface
      - "8888:8888"  # JupyterLab
    volumes:
      - ./data:/home/jovyan/data  # Mount local data directory
    command: /usr/local/bin/start-qgis-demo.sh
    restart: unless-stopped
```

Then run:

```bash
# Start with VNC + JupyterLab
docker-compose up -d
```

## 🔧 Advanced Usage

### Manual JupyterLab Startup

If you started with QGIS only (Option A) and want to add JupyterLab:

```bash
# Start JupyterLab in the running container
docker exec -d eopf-qgis-vnc jupyter lab \
  --ip=0.0.0.0 \
  --port=8888 \
  --no-browser \
  --notebook-dir=/home/jovyan/work \
  --ServerApp.token='' \
  --ServerApp.password='' \
  --ServerApp.allow_origin='*' \
  --ServerApp.allow_remote_access=True
```

**Access:**
- **QGIS VNC**: <http://localhost:6080/vnc.html>
- **JupyterLab**: <http://localhost:8888>

### Volume Mounting for Data Persistence

```bash
# Mount local directory for data persistence
docker run -d --name eopf-qgis-data \
  -p 6080:6080 -p 8888:8888 \
  -v /path/to/your/data:/home/jovyan/data \
  yuvraj1989/eopf-qgis-conda:latest /usr/local/bin/start-qgis-demo.sh
```

### Building with PowerShell (Windows)

```powershell
# PowerShell build script
Write-Host "🚀 Building EOPFZARR QGIS Container..." -ForegroundColor Green
docker build --cache-from yuvraj1989/eopf-qgis-conda:latest -t eopf-qgis-conda:latest .
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Build completed successfully!" -ForegroundColor Green
    Write-Host "🌐 Access URLs:" -ForegroundColor Cyan
    Write-Host "   • QGIS VNC: http://localhost:6080/vnc.html"
    Write-Host "   • JupyterLab: http://localhost:8888"
}
```

### Building with Bash (Linux/Mac)

```bash
#!/bin/bash
echo "🚀 Building EOPF-Zarr QGIS Container..."
docker build --cache-from yuvraj1989/eopf-qgis-conda:latest -t eopf-qgis-conda:latest .
if [ $? -eq 0 ]; then
    echo "✅ Build completed successfully!"
    echo "🌐 Access URLs:"
    echo "   • QGIS VNC: http://localhost:6080/vnc.html"
    echo "   • JupyterLab: http://localhost:8888"
fi
```

## 💡 Using the VNC Interface

### Clipboard Functionality

The container includes **enhanced multi-protocol clipboard support** for seamless copy/paste between your host system and QGIS:

1. **Standard Copy/Paste**: Use `Ctrl+C` and `Ctrl+V` - works bidirectionally
2. **Primary Selection**: Try `Shift+Insert` for X11 PRIMARY selection paste
3. **noVNC Clipboard**: Use the clipboard icon in the noVNC sidebar for additional clipboard access
4. **Automatic Synchronization**: The container runs multiple clipboard bridges:
   - `autocutsel` daemons for CLIPBOARD and PRIMARY selections
   - `xclip` bridge for continuous synchronization between selections
   - Full x11vnc clipboard integration

**How to use:**
- Copy text on your host (Windows/Mac/Linux) with `Ctrl+C`
- Click in the VNC window to focus it and open the clipboard
- Paste in clipboard in VNC with `Ctrl+V` or `Shift+Insert`
- Again copy from the clipboard in VNC
- Paste in QGIS with `Ctrl+V`
- Copy from QGIS and paste back to VNC Clipboard then to your host system

### Troubleshooting VNC

If QGIS windows are not visible:

```bash
# Maximize QGIS window
docker exec eopf-qgis-vnc bash -c "DISPLAY=:1 wmctrl -r 'QGIS' -b add,maximized_vert,maximized_horz"

# List all windows
docker exec eopf-qgis-vnc bash -c "DISPLAY=:1 wmctrl -l"
```

## 🧪 Testing EOPFZARR Driver

### Verify Driver Installation

Run this Python code in JupyterLab or QGIS Python console:

```python
from osgeo import gdal

# Check if EOPF-Zarr driver is available
gdal.AllRegister()
driver = gdal.GetDriverByName('EOPFZARR')
if driver:
    print("✅ EOPF-Zarr driver loaded successfully!")
    print(f"Driver description: {driver.GetDescription()}")
else:
    print("❌ EOPF-Zarr driver not found")
```

### Example Usage

```python
import xarray as xr
from osgeo import gdal

# Example EOPF-Zarr URL (replace with actual data URL)
url = "EOPFZARR:'/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac34695aae52a:20250912T094121_N0511_R036_T36WWE_20250912T112258.zarr/measurements/reflectance/r60m/b01'"

# Test with GDAL
dataset = gdal.Open(f"EOPFZARR:{url}")
if dataset:
    print(f"✅ Successfully opened dataset with {dataset.RasterCount} bands")
    print(f"Size: {dataset.RasterXSize}x{dataset.RasterYSize}")
else:
    print("❌ Failed to open dataset")

# Test with xarray
try:
    ds = xr.open_dataset(url, engine='zarr')
    print(f"✅ xarray opened dataset with variables: {list(ds.variables)}")
except Exception as e:
    print(f"❌ xarray failed: {e}")
```

### Container Test Script

Create and run this test script inside the container:

```python
# test_container.py
import sys
from osgeo import gdal
import numpy as np

def test_all():
    """Run comprehensive container tests"""
    print("🧪 EOPF-Zarr Container Test Suite")
    print("=" * 40)
    
    # Test GDAL version
    version = gdal.VersionInfo()
    print(f"✅ GDAL version: {gdal.VersionInfo('RELEASE_NAME')} ({version})")
    
    # Test EOPF-Zarr driver
    gdal.AllRegister()
    driver = gdal.GetDriverByName('EOPFZarr')
    if driver:
        print("✅ EOPF-Zarr GDAL driver found!")
    else:
        print("❌ EOPF-Zarr GDAL driver not found!")
        return False
    
    # Test Python packages
    packages = ['numpy', 'xarray', 'zarr', 'fsspec', 'dask', 'matplotlib']
    for package in packages:
        try:
            __import__(package)
            print(f"✅ {package} imported successfully")
        except ImportError:
            print(f"❌ Failed to import {package}")
            return False
    
    print("\n� All tests passed! Container is ready for use.")
    return True

if __name__ == "__main__":
    test_all()
```

## �🏗️ Technical Details

### Architecture

- **Base Image**: condaforge/mambaforge:24.9.2-0
- **QGIS Version**: 3.44.2 (conda-forge)
- **GDAL Version**: 3.11.3 (conda-forge)
- **Python Version**: 3.12
- **VNC Server**: x11vnc with noVNC web interface
- **Window Manager**: Fluxbox
- **Display**: Xvfb virtual framebuffer (1440x900)

### Key Environment Variables

```bash
GDAL_DRIVER_PATH=/opt/conda/lib/gdalplugins
LD_LIBRARY_PATH=/opt/conda/lib:/opt/conda/lib64
PATH=/opt/conda/bin:$PATH
QT_X11_NO_MITSHM=1
QT_XCB_GL_INTEGRATION=none
LIBGL_ALWAYS_INDIRECT=1
```

### Installed Components

**System Packages (apt):**

- Build tools: git, cmake, build-essential, pkg-config
- VNC/GUI: xvfb, x11vnc, fluxbox, novnc, websockify
- Clipboard: wmctrl, xclip, xsel, autocutsel
- X11 support: libgl1-mesa-glx, libglib2.0-0, etc.

**Conda Packages:**

- qgis>=3.44, numpy, xarray, zarr, fsspec, dask
- matplotlib, jupyterlab, notebook

## 🛠️ Troubleshooting

### Common Issues

1. **QGIS not starting**: Check X11 environment and try software rendering
2. **Clipboard not working**: Enhanced clipboard with multiple protocols is active by default. If issues persist, try the noVNC clipboard button or `Shift+Insert`
3. **VNC black screen**: Ensure x11vnc and Xvfb are running, maximize windows
4. **Port conflicts**: Use different port mappings

### Debug Commands

```bash
# Check running processes
docker exec eopf-qgis-vnc ps aux

# Check GDAL driver
docker exec eopf-qgis-vnc python -c "from osgeo import gdal; gdal.AllRegister(); print([gdal.GetDriver(i).GetDescription() for i in range(gdal.GetDriverCount()) if 'zarr' in gdal.GetDriver(i).GetDescription().lower()])"

# Container logs
docker logs eopf-qgis-vnc
```

## 📋 System Requirements

- **Docker**: Version 20.10+
- **Memory**: Minimum 4GB RAM recommended
- **Storage**: 8GB+ free space for image
- **Network**: Internet access for conda packages and git clone

## 📝 License

This project follows the same licensing as the underlying components:

- QGIS: GPL v2+
- GDAL: MIT/X11
- EOPF-Zarr Driver: Check original repository

## 🔗 Related Projects

- [EOPF-Zarr GDAL Driver](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF)
- [QGIS Official](https://qgis.org/)
- [conda-forge](https://conda-forge.org/)

---

**Happy geospatial analysis with EOPF-Zarr and QGIS!** 🌍🛰️

## 📦 Optional Files

If you need additional automation, you can create these optional files:

### docker-compose.yml

```yaml
version: '3.8'
services:
  eopf-qgis-vnc:
    image: yuvraj1989/eopf-qgis-conda:latest
    container_name: eopf-qgis-vnc
    ports:
      - "6080:6080"
      - "8888:8888"
    volumes:
      - ./data:/home/jovyan/data
    command: /usr/local/bin/start-qgis-demo.sh
    restart: unless-stopped
```

### .dockerignore

```gitignore
.git/
*.md
!README.md
__pycache__/
.vscode/
.idea/
```