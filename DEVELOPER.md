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

## CI/CD

Our GitHub Actions pipeline automatically runs:
1. **Linting**: `cppcheck` and `clang-format` checks.
2. **Build & Test**: Builds and runs tests on Linux, Windows, and macOS.
3. **Sanitizers**: Runs tests with ASan/UBSan on Linux.
4. **Static Analysis**: Runs `clang-tidy` on Linux.
