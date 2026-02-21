# Developer Guide

This guide provides instructions for building, testing, and contributing to the GDAL EOPF-Zarr Plugin.

## Build System

The project uses CMake for building. We have added several options to help ensure code quality.

### Basic Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Advanced Build Options

We support several CMake options to enable strict checking and static analysis:

| Option | Description | Default |
|--------|-------------|---------|
| `ENABLE_SANITIZERS` | Enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) | `OFF` |
| `ENABLE_CLANG_TIDY` | Enable Clang-Tidy static analysis during build | `OFF` |
| `ENABLE_WERROR` | Treat compiler warnings as errors | `OFF` |

#### Building with Sanitizers (Linux/macOS)

Sanitizers help detect memory leaks, buffer overflows, and undefined behavior at runtime.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
```

#### Building with Static Analysis

Clang-Tidy checks for code style and potential bugs during compilation.

```bash
cmake -S . -B build -DENABLE_CLANG_TIDY=ON
cmake --build build
```

## Testing

We use CTest for running tests.

### Running Tests

```bash
cd build
ctest --output-on-failure
```

### Running Specific Tests

You can run specific tests using regex:

```bash
# Run only unit tests
ctest -R "unit_"

# Run integration tests
ctest -R "integration_"
```

## Coding Standards

- **Style**: We use `clang-format` to enforce code style. Please run `clang-format -i src/*.cpp src/*.h` before committing.
- **Warnings**: Code should compile without warnings. We recommend developing with `-Wall -Wextra` enabled.
- **Modern C++**: We target C++17. Use modern features like `std::optional`, `std::variant`, and smart pointers where appropriate.

## Build & Install Scripts

We provide platform-specific scripts that build the plugin and copy it to the correct GDAL plugins directory.

### Windows

```powershell
# Build + copy DLL to C:\OSGeo4W\apps\gdal\lib\gdalplugins\
.\build-and-install.ps1
```

### Linux

```bash
# Build + copy .so to system GDAL plugins dir (needs sudo)
./build-and-install.sh

# Build only — load plugin from build/ via GDAL_DRIVER_PATH (no sudo)
./build-and-install.sh --dev

# Clean rebuild + install
./build-and-install.sh --clean
```

In **dev mode** (`--dev`), the plugin stays in `build/` and GDAL is told to load it from there using `GDAL_DRIVER_PATH`. This avoids needing `sudo cp` after every rebuild.

### macOS (Apple Silicon)

macOS requires building against the **Conda GDAL** (3.10) to avoid ABI mismatches with Homebrew GDAL.

```bash
# One-time: activate the conda environment
conda activate eopf-zarr-driver

# Build + copy .dylib to conda plugins dir
./build-and-install-macos.sh

# Build only — load plugin from build/ via GDAL_DRIVER_PATH
./build-and-install-macos.sh --dev

# Clean rebuild + install
./build-and-install-macos.sh --clean
```

#### Quick Dev Setup (macOS)

Add this alias to your `~/.zshrc` for one-command setup:

```bash
alias eopf="cd /path/to/GDAL-ZARR-EOPF && source dev-env.sh"
```

Then from any terminal:

```bash
eopf                          # activates conda env, sets GDAL_DRIVER_PATH, builds if needed
cmake --build build           # rebuild after code changes
ctest --test-dir build        # run tests
gdalinfo --formats | grep EOPF  # verify driver loads
```

> **Note:** Do not install GDAL via Homebrew alongside the Conda environment — having two
> GDAL versions causes ABI mismatches and the plugin will fail to load silently.

## Jupyter Notebook Setup (Linux)

The notebooks require **system Python** (`/usr/bin/python3`) because it has the GDAL Python bindings linked to the same GDAL that loads our `.so` plugin. Conda/venv Python environments typically ship their own GDAL version which won't load the plugin.

### One-Time Setup

```bash
./setup-jupyter.sh
```

This script:
1. Installs missing Python packages (`rasterio`, `rioxarray`) into system Python
2. Creates a Jupyter kernel **"EOPFZARR Dev (System Python + build/)"** that:
   - Uses `/usr/bin/python3` (which has GDAL bindings)
   - Sets `GDAL_DRIVER_PATH` to the `build/` directory in its kernel spec
3. Updates existing GDAL kernels (`python3-gdal`, `python3.10-gdal`) to also point at `build/`

To re-create kernels only (skip pip installs):

```bash
./setup-jupyter.sh --kernel
```

### Available Jupyter Kernels

| Kernel | Display Name | Python | Has GDAL | Auto-loads build/ |
|--------|-------------|--------|----------|-------------------|
| `eopfzarr-dev` | EOPFZARR Dev (System Python + build/) | `/usr/bin/python3` | Yes | Yes |
| `python3-gdal` | Python 3.10 (GDAL) | `/usr/bin/python3` | Yes | Yes |
| `python3.10-gdal` | Python 3.10 (GDAL+EOPFZARR) | `/usr/bin/python3.10` | Yes | Yes |
| `python3` (venv/conda) | Python 3 | `.venv/` or conda | No | No |

Use any of the first three kernels when running notebooks. The venv/conda kernel does **not** have GDAL.

### How Kernels Pick Up the Plugin

Each GDAL-aware kernel has this in its `kernel.json`:

```json
{
  "env": {
    "GDAL_DRIVER_PATH": "<project-root>/build"
  }
}
```

GDAL loads plugins once when the process starts. The kernel process **is** the Python process, so restarting the kernel forces GDAL to re-scan the plugins directory and load the freshly built `.so`.

## Development Workflow — Picking Up Code Changes

### After every C++ change

```bash
# 1. Rebuild (fast — incremental build)
./build-and-install.sh --dev

# 2. In Jupyter: restart the kernel
#    VS Code:   Ctrl+Shift+P → "Notebook: Restart Kernel"
#    Browser:   Kernel → Restart Kernel

# 3. Re-run notebook cells — new plugin is loaded
```

That's it. No `sudo cp`, no environment variable exports, no container restarts.

### Verify the plugin is loaded

In any notebook cell:

```python
from osgeo import gdal
d = gdal.GetDriverByName('EOPFZARR')
print('EOPFZARR loaded:', d is not None)
```

Or from the terminal:

```bash
GDAL_DRIVER_PATH=build gdalinfo --formats | grep EOPFZARR
```

### Quick Reference

| Task | Linux | macOS |
|------|-------|-------|
| Quick dev setup | — | `eopf` (alias) |
| Rebuild (dev, no sudo) | `./build-and-install.sh --dev` | `cmake --build build` |
| Rebuild + install | `./build-and-install.sh` | `./build-and-install-macos.sh` |
| Clean rebuild + install | `./build-and-install.sh --clean` | `./build-and-install-macos.sh --clean` |
| One-time Jupyter setup | `./setup-jupyter.sh` | `eopf` (auto-configures) |
| Run tests | `cd build && ctest --output-on-failure` | `ctest --test-dir build` |
| Check plugin loads | `GDAL_DRIVER_PATH=build gdalinfo --formats \| grep EOPFZARR` | `gdalinfo --formats \| grep EOPF` |

## Contributing

### Code Structure

```
src/
├── eopfzarr_driver.cpp       # GDAL driver registration
├── eopfzarr_dataset.cpp      # Main dataset implementation
├── eopfzarr_dataset.h        # Dataset header
├── eopfzarr_performance.cpp  # Performance optimizations
└── eopf_metadata.cpp         # Metadata handling

include/
├── eopfzarr_performance.h    # Performance framework
└── eopfzarr_config.h.in      # Configuration template
```

### Architecture Notes

- **Dataset Wrapper Pattern**: The plugin wraps GDAL's Zarr driver to add EOPF-specific metadata enhancement, geospatial coordinate detection, and performance optimizations.
- **Performance Framework**: Includes metadata caching with TTL, network optimization, block access pattern tracking, and lazy loading.
- **Const-Correctness**: Use `const_cast` sparingly and document why. Prefer `mutable` members for caching.

### Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes and add/update tests
4. Ensure all tests pass: `ctest --test-dir build --output-on-failure`
5. Commit with clear messages (e.g., `feat: add caching for metadata operations`)
6. Push and create a pull request

### Debugging

```bash
# Verbose CMake output
cmake -S . -B build -DCMAKE_VERBOSE_MAKEFILE=ON

# Debug build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Enable GDAL debug output
export CPL_DEBUG=ON

# Enable plugin-specific debug
export CPL_DEBUG=EOPFZARR

# Enable performance timers
export EOPF_ENABLE_PERFORMANCE_TIMERS=1
```

## CI/CD

Our GitHub Actions pipeline automatically runs:
1. **Linting**: `cppcheck` and `clang-format` checks.
2. **Build & Test**: Builds and runs tests on Linux, Windows, and macOS.
3. **Sanitizers**: Runs tests with ASan/UBSan on Linux.
4. **Static Analysis**: Runs `clang-tidy` on Linux.
