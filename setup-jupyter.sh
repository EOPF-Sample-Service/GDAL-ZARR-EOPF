#!/bin/bash
# =============================================================================
# setup-jupyter.sh — One-time setup for running notebooks on Linux
#
# What this does:
#   1. Installs missing Python packages into system Python (rasterio, rioxarray)
#   2. Creates a Jupyter kernel "EOPFZARR-Dev" that:
#      - Uses system Python (/usr/bin/python3) which has GDAL bindings
#      - Sets GDAL_DRIVER_PATH to the build/ directory
#      → After rebuilding, just restart the kernel — no sudo cp needed
#   3. Ensures ipykernel is installed in system Python
#
# Usage:
#   ./setup-jupyter.sh          # full setup
#   ./setup-jupyter.sh --kernel # only (re)create the kernel
#
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SYSTEM_PYTHON="/usr/bin/python3"
KERNEL_NAME="eopfzarr-dev"
KERNEL_DISPLAY="EOPFZARR Dev (System Python + build/)"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# ---------- parse args ------------------------------------------------------
KERNEL_ONLY=false
for arg in "$@"; do
    case "$arg" in
        --kernel) KERNEL_ONLY=true ;;
        -h|--help)
            echo "Usage: $0 [--kernel | --help]"
            exit 0
            ;;
    esac
done

# ---------- verify system python has GDAL -----------------------------------
echo -e "${GREEN}Checking system Python GDAL...${NC}"
if ! $SYSTEM_PYTHON -c "from osgeo import gdal" 2>/dev/null; then
    echo -e "${RED}ERROR: System Python ($SYSTEM_PYTHON) does not have GDAL bindings.${NC}"
    echo "Install with:  sudo apt install python3-gdal"
    exit 1
fi

GDAL_VER=$($SYSTEM_PYTHON -c "from osgeo import gdal; print(gdal.VersionInfo())")
echo -e "  GDAL version: ${YELLOW}$GDAL_VER${NC}"

# ---------- install missing packages (unless --kernel only) ------------------
if [[ "$KERNEL_ONLY" == false ]]; then
    echo ""
    echo -e "${GREEN}Installing missing Python packages for notebooks...${NC}"

    # Packages the notebooks actually import that are missing from system Python
    MISSING_PKGS=()
    for pkg in rasterio rioxarray; do
        if ! $SYSTEM_PYTHON -c "import $pkg" 2>/dev/null; then
            MISSING_PKGS+=("$pkg")
        fi
    done

    if [[ ${#MISSING_PKGS[@]} -gt 0 ]]; then
        echo -e "  Installing: ${YELLOW}${MISSING_PKGS[*]}${NC}"
        sudo $SYSTEM_PYTHON -m pip install "${MISSING_PKGS[@]}"
    else
        echo -e "  All notebook packages already installed."
    fi

    # Ensure ipykernel is installed (needed for Jupyter kernel)
    if ! $SYSTEM_PYTHON -c "import ipykernel" 2>/dev/null; then
        echo -e "  Installing ipykernel..."
        sudo $SYSTEM_PYTHON -m pip install ipykernel
    fi
fi

# ---------- create / update the Jupyter kernel spec --------------------------
echo ""
echo -e "${GREEN}Creating Jupyter kernel: $KERNEL_DISPLAY${NC}"

KERNEL_DIR="${HOME}/.local/share/jupyter/kernels/${KERNEL_NAME}"
mkdir -p "$KERNEL_DIR"

cat > "$KERNEL_DIR/kernel.json" <<EOF
{
  "argv": [
    "$SYSTEM_PYTHON",
    "-m",
    "ipykernel_launcher",
    "-f",
    "{connection_file}"
  ],
  "display_name": "$KERNEL_DISPLAY",
  "language": "python",
  "env": {
    "GDAL_DRIVER_PATH": "$BUILD_DIR"
  },
  "metadata": {
    "debugger": true
  }
}
EOF

echo -e "  Kernel spec written to: ${YELLOW}$KERNEL_DIR/kernel.json${NC}"

# ---------- also update the existing GDAL kernels to use build/ path ---------
for existing_kernel in python3-gdal python3.10-gdal; do
    EXISTING_DIR="${HOME}/.local/share/jupyter/kernels/${existing_kernel}"
    if [[ -f "$EXISTING_DIR/kernel.json" ]]; then
        echo -e "  Updating ${YELLOW}$existing_kernel${NC} kernel to include GDAL_DRIVER_PATH"
        # Use python to safely patch the JSON
        $SYSTEM_PYTHON -c "
import json, sys
path = '$EXISTING_DIR/kernel.json'
with open(path) as f:
    k = json.load(f)
k.setdefault('env', {})
k['env']['GDAL_DRIVER_PATH'] = '$BUILD_DIR'
with open(path, 'w') as f:
    json.dump(k, f, indent=2)
print('  Updated:', path)
"
    fi
done

# ---------- summary ----------------------------------------------------------
echo ""
echo -e "${GREEN}=== Setup Complete ===${NC}"
echo ""
echo "Available GDAL-aware kernels:"
jupyter kernelspec list 2>/dev/null | grep -E "eopfzarr|gdal" || true
echo ""
echo "Development workflow:"
echo "  1. Edit C++ source code"
echo "  2. Run:   ./build-and-install.sh --dev"
echo "  3. In Jupyter: Kernel → Restart Kernel"
echo "  4. Re-run notebook cells — new plugin is loaded automatically"
echo ""
echo -e "Kernel reads the plugin directly from ${YELLOW}$BUILD_DIR/${NC}"
echo "No sudo cp needed during development!"
