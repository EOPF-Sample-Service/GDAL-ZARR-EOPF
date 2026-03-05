# Developer Guide

## Requirements

- GDAL 3.10+ with development headers
- CMake 3.16+
- C++17 compatible compiler

## Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo cp gdal_EOPFZarr.so $(gdal-config --plugindir)/
```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install libgdal-dev cmake build-essential

mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### macOS

```bash
brew install gdal cmake

mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Windows

```cmd
# Using vcpkg
vcpkg install gdal[core]

mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release
```

### Build Options

| Option | Description | Default |
|---|---|---|
| `ENABLE_SANITIZERS` | AddressSanitizer + UndefinedBehaviorSanitizer | `OFF` |
| `ENABLE_CLANG_TIDY` | Clang-Tidy static analysis | `OFF` |
| `ENABLE_WERROR` | Treat warnings as errors | `OFF` |

```bash
# Build with sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..

# Build with clang-tidy
cmake -DENABLE_CLANG_TIDY=ON ..
```

## Testing

```bash
# Python integration tests (requires system Python with GDAL)
/usr/bin/python3 -m pytest tests/integration/ -v

# C++ tests via CTest
cd build && ctest --output-on-failure
```

## Code Style

Run `clang-format` before committing:

```bash
clang-format --style=file -i src/*.cpp src/*.h
```

## CI

GitHub Actions runs on every PR:
- Ubuntu, macOS, Windows builds
- Python integration tests (including network tests against real EODC data)
- cppcheck, clang-tidy static analysis
- AddressSanitizer / UndefinedBehaviorSanitizer
- Docker Hub image rebuild on merge to `main`
