#!/bin/bash
# EOPF-Zarr GDAL Plugin — Development Environment
# Usage: source dev-env.sh
#
# This single command does everything:
#   1. Activates the conda environment (eopf-zarr-driver)
#   2. Points GDAL_DRIVER_PATH at build/ for auto-pickup of latest build
#   3. Builds the plugin if not already built (against the correct GDAL)
#   4. Verifies the driver loads

# Support both bash (BASH_SOURCE) and zsh (via EOPF_PROJECT_ROOT or fallback)
if [[ -n "${BASH_SOURCE[0]:-}" && "${BASH_SOURCE[0]}" != "$0" ]]; then
    PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
elif [[ -n "${EOPF_PROJECT_ROOT:-}" ]]; then
    PROJECT_ROOT="${EOPF_PROJECT_ROOT}"
else
    PROJECT_ROOT="/Users/yuvrajadagale/gdal/development/GDAL-ZARR-EOPF"
fi
BUILD_DIR="${PROJECT_ROOT}/build"
CONDA_ENV="eopf-zarr-driver"
CONDA_PREFIX="/opt/anaconda3/envs/${CONDA_ENV}"

# --- Step 1: Activate conda environment ---
if [ -z "${CONDA_DEFAULT_ENV}" ] || [ "${CONDA_DEFAULT_ENV}" != "${CONDA_ENV}" ]; then
    if command -v conda &>/dev/null; then
        eval "$(conda shell.bash hook 2>/dev/null)"
        conda activate "${CONDA_ENV}" 2>/dev/null
        if [ $? -ne 0 ]; then
            echo "ERROR: Could not activate conda env '${CONDA_ENV}'"
            echo "  Create it with: conda create -n ${CONDA_ENV} python gdal numpy matplotlib rasterio jupyter"
            return 1 2>/dev/null || exit 1
        fi
    fi
fi

# --- Step 2: Set environment variables ---
export GDAL_DRIVER_PATH="${BUILD_DIR}"
export PROJ_LIB="${CONDA_PREFIX}/share/proj"

# --- Step 3: Build if needed ---
if [ ! -f "${BUILD_DIR}/gdal_EOPFZarr.dylib" ]; then
    echo "Plugin not built yet — building against conda GDAL..."
    cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DGDAL_INCLUDE_DIR="${CONDA_PREFIX}/include" \
        -DGDAL_LIBRARY="${CONDA_PREFIX}/lib/libgdal.dylib" \
        && cmake --build "${BUILD_DIR}" --parallel
    if [ $? -ne 0 ]; then
        echo "ERROR: Build failed"
        return 1 2>/dev/null || exit 1
    fi
fi

# --- Step 4: Verify ---
if GDAL_DRIVER_PATH="${BUILD_DIR}" gdalinfo --formats 2>/dev/null | grep -qi EOPFZARR; then
    DRIVER_OK="yes"
else
    DRIVER_OK="no"
fi

echo ""
echo "EOPF-Zarr dev environment active"
echo "  Conda env:        ${CONDA_DEFAULT_ENV}"
echo "  GDAL version:     $(gdal-config --version 2>/dev/null)"
echo "  GDAL_DRIVER_PATH: ${BUILD_DIR}"
echo "  Plugin:           $(ls -lh "${BUILD_DIR}/gdal_EOPFZarr.dylib" 2>/dev/null | awk '{print $5, $6, $7, $8}')"
echo "  Driver loaded:    ${DRIVER_OK}"
echo ""
echo "After code changes:  cmake --build build"
echo "Run notebooks:       jupyter notebook notebooks/"
echo ""
