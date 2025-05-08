# EOPF‑Zarr plugin

[![Build Status](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/build.yml/badge.svg)](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/actions/workflows/build.yml)

**Overview**  
This repository contains the **EOPF GDAL Plugin**, a community-driven effort to extend [GDAL](https://gdal.org) with support for the Earth Observation Processing Framework (EOPF). By loading this plugin, users can open, process, and visualize EOPF datasets in GDAL-based applications and workflows—such as [QGIS](https://qgis.org), command-line utilities (e.g., `gdalinfo`, `gdal_translate`), and custom scripts.

> **Note:** EOPF data is commonly stored in cloud-native formats (e.g., Zarr) or NetCDF-like containers. This plugin aims to handle those data structures seamlessly, exposing them as standard GDAL raster datasets.

---

## Features

- **Read-Only Support**  
  Currently supports reading of EOPF datasets, including chunked data (Zarr-based archives), multi-band imagery, and associated metadata.

- **Cloud Compatibility**  
  Leverages GDAL’s virtual file I/O to access data hosted on local storage or cloud object stores (future enhancement: S3, /vsicurl/).

- **Metadata Extraction**  
  Parses core EOPF metadata (dimensions, data types, partial georeferencing if present). Additional coverage of advanced attributes (e.g. sensor-specific metadata) will be added incrementally.

- **Integration**  
  Once installed, any GDAL-based application automatically recognizes EOPF datasets. For example, QGIS “Add Raster Layer” can ingest EOPF data with no additional configuration.

---

## Status

| Feature                        | Status       |
|--------------------------------|-------------|
| Zarr-based read                | In progress |
| Multi-band support             | Planned     |
| Metadata and georeferencing    | Planned     |
| Write support                  | Planned     |
| Cloud (/vsis3/) access         | Planned     |
| QGIS usage validation          | Ongoing     |

All functionality is under continuous development. Refer to the [Roadmap](docs/roadmap.md) or GitHub Issues for the full development timeline.

---

## Table of Contents

1. [Requirements](#requirements)  
2. [Building](#building)  
3. [Installing the Plugin](#installing-the-plugin)  
4. [Usage](#usage)  
5. [Testing](#testing)  
6. [Contributing](#contributing)  
7. [License](#license)  
8. [Contact & Credits](#contact--credits)

---

## Requirements

- **GDAL** >= 3.x (development headers required, e.g. `libgdal-dev` on Debian/Ubuntu)  
- **C++17** or later (recommended for improved JSON parsing, concurrency, etc.)  
- **CMake** >= 3.10 (build system)  
- **(Optional) Python** for extended testing or data generation scripts

On Linux, typical packages might include:
```bash
sudo apt-get update && sudo apt-get install -y \
  gdal-bin libgdal-dev \
  build-essential cmake \
  python3 python3-pip
```

---

## Building

1. **Clone the Repository**  
   ```bash
    https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF.git
    cd GDAL-ZARR-EOPF
   ```

2. **Configure with CMake**  
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
  
   ```
   The CMake script attempts to locate GDAL via `find_package(GDAL REQUIRED)`. If GDAL is not found automatically, set `GDAL_DIR` or `GDAL_INCLUDE_DIR/GDAL_LIBRARY` manually.

3. **Compile**  
   ```bash
    cmake --build . -j$(nproc)
   ```
   This produces a shared library named something like `gdal_EOPFZarr.so` (on Linux) or `gdal_EOPFZarr.dll` (Windows).

---

## Installing the Plugin

### Option A: Copy to GDAL’s Plugin Directory

Move or copy the resulting library to GDAL’s plugin path, such as:
- **Linux**: `/usr/lib/gdalplugins/` or `/usr/local/lib/gdalplugins/`
- **Windows**: `C:\Program Files\GDAL\gdalplugins\` (varies depending on your setup)

### Option B: Use `GDAL_DRIVER_PATH`

Set an environment variable so GDAL knows where to find your plugin:
```bash
export GDAL_DRIVER_PATH="/path/to/build"
```
Replace `/path/to/build` with the actual directory containing `gdal_EOPF.so` (or `.dll` on Windows).

---

## Usage

1. **Check Formats**  
   ```bash
   gdalinfo --formats | grep EOPFZARR
   ```
   You should see something like:  
   ```
   EOPFZarr (ro): Earth Observation Processing Framework
   ```

2. **Open an EOPF Dataset**  
   ```bash
   gdalinfo /path/to/EOPF_Zarr_Dataset/
   ```
   or in Python:
   ```python
   from osgeo import gdal
   ds = gdal.Open("/path/to/EOPF_Zarr_Dataset/")
   print(ds.RasterXSize, ds.RasterYSize)
   ```
   If everything is set up correctly, the plugin interprets the EOPF data and displays or processes it like any other GDAL-supported format.

3. **In QGIS**  
   - Ensure the plugin library is discoverable by GDAL (see [Installing the Plugin](#installing-the-plugin)).  
   - Launch QGIS → **Add Raster Layer** → Select the EOPF directory.  
   - Confirm it displays the data with correct band(s), georeferencing, and metadata.

---

## Testing

We provide **two** types of tests:

1. **C++ Tests** (optional)  
   - Located in `tests/`. Built with CMake + CTest.  
   ```bash
   cd build
   ctest --verbose
   ```

2. **Python-based Tests**  
   - Python scripts or `pytest` tests that open EOPF data. For example, `tests/test_eopf.py` might validate chunk reading.  
   ```bash
   pip install -r python/requirements.txt
   pytest tests/
   ```
3. **Sample Data**  
   - Minimal Zarr/EOPF directories are in `tests/sample_data/`.  
   - Future expansions will include real-world reference data for integration testing.

---

## Contributing

1. **Fork & Branch**  
   - Fork this repo, create feature branches, and submit Pull Requests.  
2. **Coding Style**  
   - Follow [GDAL’s guidelines](https://gdal.org/development/rfc/index.html) for driver formatting, error handling, etc.  
3. **Issues & Roadmap**  
   - See GitHub Issues for our backlog and planned features (multi-band metadata, HPC performance, S3 integration).  
   - We welcome bug reports, feature requests, or suggestions.

**Security**: If you discover a security vulnerability in the plugin, please contact the maintainers directly before creating a public issue.

---

## License

This project is released under the **MIT License**, compatible with GDAL’s open-source licensing. Please see the [LICENSE](LICENSE) file for details. By contributing, you agree your contributions will be licensed under the same terms.

---

## Contact & Credits

- **Maintainers**: EURAC, [Issues](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues)  
- **Acknowledgments**:  
  - ESA for EOPF specification, sample data, or project sponsorship  
  - GDAL & QGIS communities for providing the geospatial foundation

---
