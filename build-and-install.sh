#!/bin/bash
# =============================================================================
# build-and-install.sh — Linux equivalent of build-and-install.ps1
#
# Builds the EOPFZARR GDAL plugin and installs it so that Jupyter notebooks
# (and any GDAL process) pick up the latest code.
#
# Usage:
#   ./build-and-install.sh              # build + copy to system plugins dir
#   ./build-and-install.sh --dev        # build only (use GDAL_DRIVER_PATH=build/)
#   ./build-and-install.sh --clean      # clean rebuild + install
#
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---------- configuration ---------------------------------------------------
BUILD_DIR="build"
PLUGIN_FILE="gdal_EOPFZarr.so"

# Where GDAL looks for plugins — auto-detect from gdal-config, fallback
if command -v gdal-config &>/dev/null; then
    SYSTEM_PLUGIN_DIR="$(gdal-config --plugindir 2>/dev/null || echo "/usr/local/lib/gdalplugins")"
else
    SYSTEM_PLUGIN_DIR="${GDAL_DRIVER_PATH:-/usr/local/lib/gdalplugins}"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ---------- parse args ------------------------------------------------------
MODE="install"  # default: build + copy to system plugins dir
for arg in "$@"; do
    case "$arg" in
        --dev)   MODE="dev" ;;
        --clean) MODE="clean" ;;
        -h|--help)
            echo "Usage: $0 [--dev | --clean | --help]"
            echo ""
            echo "  (default)  Build and copy plugin to system GDAL plugins dir"
            echo "  --dev      Build only — use GDAL_DRIVER_PATH=$SCRIPT_DIR/$BUILD_DIR"
            echo "  --clean    Clean rebuild + install to system plugins dir"
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
    echo -e "${GREEN}Configuring CMake...${NC}"
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

# ---------- build -----------------------------------------------------------
echo -e "${GREEN}Building project...${NC}"
cmake --build "$BUILD_DIR" --config Release -j "$(nproc)"

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
    # Copy to system plugin directory (needs sudo)
    echo -e "${GREEN}Installing plugin to $SYSTEM_PLUGIN_DIR ...${NC}"
    sudo mkdir -p "$SYSTEM_PLUGIN_DIR"
    sudo cp "$BUILD_DIR/$PLUGIN_FILE" "$SYSTEM_PLUGIN_DIR/"
    sudo chmod 755 "$SYSTEM_PLUGIN_DIR/$PLUGIN_FILE"

    echo ""
    echo -e "${GREEN}=== Build & install completed ===${NC}"
    echo -e "Plugin installed to: ${YELLOW}$SYSTEM_PLUGIN_DIR/$PLUGIN_FILE${NC}"
    echo ""
    echo -e "To pick up changes in a running Jupyter notebook:"
    echo -e "  Kernel → Restart Kernel"
fi

# ---------- quick sanity check ----------------------------------------------
echo ""
# Prefer system gdalinfo (/usr/local/bin) over conda's version
GDALINFO_BIN=""
for candidate in /usr/local/bin/gdalinfo /usr/bin/gdalinfo; do
    if [[ -x "$candidate" ]]; then
        GDALINFO_BIN="$candidate"
        break
    fi
done
[[ -z "$GDALINFO_BIN" ]] && GDALINFO_BIN="$(command -v gdalinfo 2>/dev/null || true)"

if [[ -n "$GDALINFO_BIN" ]]; then
    # Run in a clean env to avoid conda library conflicts
    FORMATS=$(env -i PATH="/usr/local/bin:/usr/bin:/bin" \
        GDAL_DRIVER_PATH="$SCRIPT_DIR/$BUILD_DIR" \
        GDAL_DATA="${GDAL_DATA:-}" \
        "$GDALINFO_BIN" --formats 2>/dev/null || true)
    if echo "$FORMATS" | grep -q EOPFZARR; then
        echo -e "${GREEN}✅ EOPFZARR driver is loadable${NC}"
    else
        echo -e "${YELLOW}⚠  EOPFZARR not detected by gdalinfo — the plugin may require the matching system GDAL${NC}"
    fi
fi
