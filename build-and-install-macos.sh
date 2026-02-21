#!/bin/bash
# =============================================================================
# build-and-install-macos.sh — macOS (Apple Silicon) build script
#
# Builds the EOPFZARR GDAL plugin against the conda environment's GDAL
# and installs it so that Jupyter notebooks pick up the latest code.
#
# Usage:
#   ./build-and-install-macos.sh              # build + copy to conda plugins dir
#   ./build-and-install-macos.sh --dev        # build only (use GDAL_DRIVER_PATH=build/)
#   ./build-and-install-macos.sh --clean      # clean rebuild + install
#
# Prerequisites:
#   conda activate eopf-zarr-driver
#
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---------- configuration ---------------------------------------------------
BUILD_DIR="build"
PLUGIN_FILE="gdal_EOPFZarr.dylib"

# Conda environment — required on macOS to avoid picking up Homebrew GDAL
CONDA_ENV="eopf-zarr-driver"
CONDA_PREFIX="${CONDA_PREFIX:-/opt/anaconda3/envs/$CONDA_ENV}"

if [[ -z "${CONDA_DEFAULT_ENV:-}" ]]; then
    echo -e "\033[0;31mERROR: No conda environment active.\033[0m"
    echo "  Run: conda activate $CONDA_ENV"
    exit 1
fi

if [[ "${CONDA_DEFAULT_ENV}" != "$CONDA_ENV" ]]; then
    echo -e "\033[1;33mWARNING: Active conda env is '${CONDA_DEFAULT_ENV}', expected '$CONDA_ENV'\033[0m"
fi

# Where GDAL looks for plugins — use conda env's plugin dir
if command -v gdal-config &>/dev/null; then
    SYSTEM_PLUGIN_DIR="$(gdal-config --plugindir 2>/dev/null || echo "${CONDA_PREFIX}/lib/gdalplugins")"
else
    SYSTEM_PLUGIN_DIR="${CONDA_PREFIX}/lib/gdalplugins"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ---------- parse args ------------------------------------------------------
MODE="install"  # default: build + copy to plugins dir
for arg in "$@"; do
    case "$arg" in
        --dev)   MODE="dev" ;;
        --clean) MODE="clean" ;;
        -h|--help)
            echo "Usage: $0 [--dev | --clean | --help]"
            echo ""
            echo "  (default)  Build and copy plugin to conda GDAL plugins dir"
            echo "  --dev      Build only — use GDAL_DRIVER_PATH=$SCRIPT_DIR/$BUILD_DIR"
            echo "  --clean    Clean rebuild + install to plugins dir"
            echo ""
            exit 0
            ;;
        *) echo -e "${RED}Unknown option: $arg${NC}"; exit 1 ;;
    esac
done

# ---------- clean (optional) ------------------------------------------------
if [[ "$MODE" == "clean" ]]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# ---------- configure -------------------------------------------------------
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo -e "${GREEN}Configuring CMake (using conda GDAL at ${CONDA_PREFIX})...${NC}"

    CMAKE_GENERATOR="Unix Makefiles"
    if command -v ninja &>/dev/null; then
        CMAKE_GENERATOR="Ninja"
    fi

    cmake -S . -B "$BUILD_DIR" -G "$CMAKE_GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DGDAL_INCLUDE_DIR="${CONDA_PREFIX}/include" \
        -DGDAL_LIBRARY="${CONDA_PREFIX}/lib/libgdal.dylib"
fi

# ---------- build -----------------------------------------------------------
echo -e "${GREEN}Building project...${NC}"
# macOS uses sysctl instead of nproc
NPROC=$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
cmake --build "$BUILD_DIR" --config Release -j "$NPROC"

# ---------- verify build succeeded ------------------------------------------
if [[ ! -f "$BUILD_DIR/$PLUGIN_FILE" ]]; then
    echo -e "${RED}ERROR: $PLUGIN_FILE not found in $BUILD_DIR/ — build may have failed${NC}"
    exit 1
fi

echo -e "${GREEN}Built: $BUILD_DIR/$PLUGIN_FILE${NC}"

# ---------- install / dev mode ----------------------------------------------
if [[ "$MODE" == "dev" ]]; then
    echo ""
    echo -e "${GREEN}=== Dev mode — plugin stays in $BUILD_DIR/ ===${NC}"
    echo -e "Set this env var so GDAL loads the plugin from the build directory:"
    echo ""
    echo -e "  ${YELLOW}export GDAL_DRIVER_PATH=$SCRIPT_DIR/$BUILD_DIR${NC}"
    echo ""
    echo -e "After rebuilding, just restart the Jupyter kernel to pick up changes."
else
    # Copy to conda plugin directory (no sudo needed for conda dirs)
    echo -e "${GREEN}Installing plugin to $SYSTEM_PLUGIN_DIR ...${NC}"
    mkdir -p "$SYSTEM_PLUGIN_DIR"
    cp "$BUILD_DIR/$PLUGIN_FILE" "$SYSTEM_PLUGIN_DIR/"
    chmod 755 "$SYSTEM_PLUGIN_DIR/$PLUGIN_FILE"

    echo ""
    echo -e "${GREEN}=== Build & install completed ===${NC}"
    echo -e "Plugin installed to: ${YELLOW}$SYSTEM_PLUGIN_DIR/$PLUGIN_FILE${NC}"
    echo ""
    echo -e "To pick up changes in a running Jupyter notebook:"
    echo -e "  Kernel → Restart Kernel"
fi

# ---------- quick sanity check ----------------------------------------------
echo ""
GDALINFO_BIN="$(command -v gdalinfo 2>/dev/null || true)"

if [[ -n "$GDALINFO_BIN" ]]; then
    FORMATS=$(GDAL_DRIVER_PATH="$SCRIPT_DIR/$BUILD_DIR" "$GDALINFO_BIN" --formats 2>/dev/null || true)
    if echo "$FORMATS" | grep -q EOPFZARR; then
        echo -e "${GREEN}EOPFZARR driver is loadable${NC}"
    else
        echo -e "${YELLOW}EOPFZARR not detected by gdalinfo — check that conda GDAL versions match${NC}"
    fi
fi
