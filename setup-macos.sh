#!/bin/bash

# GDAL EOPF-Zarr Plugin Setup Script for macOS
# Complete build, installation, and verification in one command
# Usage: ./setup-macos.sh [options]
#   --local   Install to ~/.gdal/plugins instead of system
#   --skip-build   Skip building (use pre-built plugin)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PLUGIN_FILE="gdal_EOPFZarr.dylib"
LOCAL_INSTALL=false
SKIP_BUILD=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --local)
            LOCAL_INSTALL=true
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        *)
            echo "Usage: $0 [--local] [--skip-build]"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  GDAL EOPF-Zarr Plugin Setup for macOS                   ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# STEP 1: Check Dependencies
# ============================================================================
echo -e "${BLUE}Step 1: Checking Dependencies${NC}"
echo -e "${YELLOW}────────────────────────────────────────${NC}"

MISSING_DEPS=()

if ! command -v cmake &> /dev/null; then
    MISSING_DEPS+=("cmake")
fi

if ! command -v ninja &> /dev/null; then
    MISSING_DEPS+=("ninja")
fi

if ! command -v gdal-config &> /dev/null; then
    MISSING_DEPS+=("gdal")
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo -e "${RED}✗ Missing dependencies: ${MISSING_DEPS[*]}${NC}"
    echo ""
    echo -e "${YELLOW}Install missing packages with:${NC}"
    echo "  brew install ${MISSING_DEPS[*]}"
    exit 1
else
    echo -e "${GREEN}✓ cmake${NC}"
    echo -e "${GREEN}✓ ninja${NC}"
    GDAL_VERSION=$(gdal-config --version)
    echo -e "${GREEN}✓ gdal ${GDAL_VERSION}${NC}"
fi
echo ""

# ============================================================================
# STEP 2: Build
# ============================================================================
if [ "$SKIP_BUILD" = false ]; then
    echo -e "${BLUE}Step 2: Building Plugin${NC}"
    echo -e "${YELLOW}────────────────────────────────────────${NC}"
    
    # Clean build directory
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
    
    # Configure
    echo "Configuring CMake..."
    cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
    
    # Build
    echo "Building plugin..."
    cmake --build "${BUILD_DIR}" --parallel > /dev/null 2>&1
    
    # Verify plugin exists
    if [ ! -f "${BUILD_DIR}/${PLUGIN_FILE}" ]; then
        echo -e "${RED}✗ Build failed: Plugin not found${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Build complete${NC}"
else
    echo -e "${BLUE}Step 2: Skipping Build${NC}"
    echo -e "${YELLOW}────────────────────────────────────────${NC}"
    if [ ! -f "${BUILD_DIR}/${PLUGIN_FILE}" ]; then
        echo -e "${RED}✗ No pre-built plugin found${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Pre-built plugin verified${NC}"
fi
echo ""

# ============================================================================
# STEP 3: Install
# ============================================================================
echo -e "${BLUE}Step 3: Installing Plugin${NC}"
echo -e "${YELLOW}────────────────────────────────────────${NC}"

# Detect GDAL installation
if ! command -v gdal-config &> /dev/null; then
    echo -e "${RED}✗ GDAL not found${NC}"
    exit 1
fi

GDAL_PREFIX=$(gdal-config --prefix)

# Determine plugin directory
if [ "$LOCAL_INSTALL" = true ]; then
    GDAL_PLUGIN_DIR="${HOME}/.gdal/plugins"
    NEEDS_SUDO=false
    echo "Installing to local directory: ${GDAL_PLUGIN_DIR}"
else
    # System-wide installation - use the default GDAL plugin directory
    GDAL_PLUGIN_DIR="/opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins"
    NEEDS_SUDO=false
    echo "Installing to GDAL plugin directory: ${GDAL_PLUGIN_DIR}"
fi

# Create plugin directory if needed
if [ ! -d "${GDAL_PLUGIN_DIR}" ]; then
    if [ "$NEEDS_SUDO" = true ]; then
        sudo mkdir -p "${GDAL_PLUGIN_DIR}"
    else
        mkdir -p "${GDAL_PLUGIN_DIR}"
    fi
fi

# Handle existing plugin
EXISTING_PLUGIN="${GDAL_PLUGIN_DIR}/${PLUGIN_FILE}"
if [ -f "${EXISTING_PLUGIN}" ]; then
    if [ "$NEEDS_SUDO" = true ]; then
        sudo rm -f "${EXISTING_PLUGIN}"
    else
        rm -f "${EXISTING_PLUGIN}"
    fi
fi

# Copy plugin
if [ "$NEEDS_SUDO" = true ]; then
    sudo cp "${BUILD_DIR}/${PLUGIN_FILE}" "${GDAL_PLUGIN_DIR}/"
    sudo chmod 644 "${GDAL_PLUGIN_DIR}/${PLUGIN_FILE}"
else
    cp "${BUILD_DIR}/${PLUGIN_FILE}" "${GDAL_PLUGIN_DIR}/"
    chmod 644 "${GDAL_PLUGIN_DIR}/${PLUGIN_FILE}"
fi

echo -e "${GREEN}✓ Plugin installed${NC}"
echo ""

# ============================================================================
# STEP 4: Verification
# ============================================================================
echo -e "${BLUE}Step 4: Verification${NC}"
echo -e "${YELLOW}────────────────────────────────────────${NC}"

if GDAL_DRIVER_PATH="${GDAL_PLUGIN_DIR}" gdalinfo --formats | grep -qi "EOPFZARR"; then
    echo -e "${GREEN}✓ Plugin loaded successfully!${NC}"
    echo ""
    GDAL_DRIVER_PATH="${GDAL_PLUGIN_DIR}" gdalinfo --formats | grep -i "EOPFZARR"
else
    echo -e "${RED}✗ Plugin verification failed${NC}"
    if [ "$LOCAL_INSTALL" = true ]; then
        echo ""
        echo -e "${YELLOW}For local installation, add to ~/.zprofile:${NC}"
        echo "  export GDAL_DRIVER_PATH=\$HOME/.gdal/plugins:\$GDAL_DRIVER_PATH"
    else
        echo ""
        echo -e "${YELLOW}To enable auto-loading, add to ~/.zprofile:${NC}"
        echo "  export GDAL_DRIVER_PATH=${GDAL_PLUGIN_DIR}:\$GDAL_DRIVER_PATH"
    fi
    exit 1
fi
echo ""

# ============================================================================
# Complete
# ============================================================================
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✓ Setup Complete!                                       ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Installation Summary:${NC}"
echo "  Plugin:       ${PLUGIN_FILE}"
echo "  Location:     ${GDAL_PLUGIN_DIR}/${PLUGIN_FILE}"
echo "  GDAL:         ${GDAL_VERSION}"
echo "  Architecture: $(file "${GDAL_PLUGIN_DIR}/${PLUGIN_FILE}" | grep -o "arm64\|x86_64")"
echo ""
echo -e "${BLUE}Quick Commands:${NC}"
echo "  Test plugin:        gdalinfo --formats | grep EOPFZARR"
echo "  Rebuild:            ./setup-macos.sh"
echo "  Local install:      ./setup-macos.sh --local"
echo ""
