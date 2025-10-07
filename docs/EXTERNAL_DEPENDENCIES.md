# External Dependencies Management

## Philosophy

Asyncle minimizes external dependencies for its core functionality. Optional features (like JSON parsing) may require external libraries. This document establishes standards for dependency management.

## Dependency Categories

### 1. Core Dependencies (Required)
- **C++23 compiler**: GCC 12+, Clang 17+, MSVC 19.35+
- **CMake**: 3.23+
- **Standard Library**: C++23 std::expected support

### 2. Optional Dependencies (Format Layer)
- **simdjson**: JSON parser for streaming/high-performance use cases
- **Glaze**: JSON parser for structured/compile-time reflection

## Installation Methods

### Preferred Order
1. **System Package Manager** (recommended for production)
2. **CMake FetchContent** (recommended for development)
3. **Manual Installation** (last resort)

## simdjson Integration

### Overview
- **License**: Apache 2.0
- **Language**: C++17
- **Type**: Compiled library (not header-only)
- **CMake Target**: `simdjson::simdjson`

### Method 1: System Package Manager (Recommended for Production)

#### Ubuntu/Debian
```bash
sudo apt install libsimdjson-dev
```

#### Arch Linux
```bash
sudo pacman -S simdjson
```

#### macOS (Homebrew)
```bash
brew install simdjson
```

#### Verification
```bash
pkg-config --modversion simdjson
```

### Method 2: CMake FetchContent (Recommended for Development)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG v3.10.1  # Use specific version, not 'main'
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(simdjson)

target_link_libraries(your_target PRIVATE simdjson::simdjson)
```

### Method 3: Manual Installation

```bash
git clone https://github.com/simdjson/simdjson.git
cd simdjson
mkdir build && cd build
cmake ..
make
sudo make install
```

### CMake Integration in asyncle

```cmake
if(FORMAT_ENABLE_SIMDJSON)
    find_package(simdjson QUIET)
    if(simdjson_FOUND)
        message(STATUS "simdjson found (version ${simdjson_VERSION})")
        target_compile_definitions(format INTERFACE FORMAT_HAS_SIMDJSON)
        target_link_libraries(format INTERFACE simdjson::simdjson)
    else()
        message(WARNING "simdjson not found")
        message(WARNING "  Install: sudo apt install libsimdjson-dev")
        message(WARNING "  Or use: -DFORMAT_USE_FETCHCONTENT=ON")
    endif()
endif()
```

## Glaze Integration

### Overview
- **License**: MIT
- **Language**: C++23
- **Type**: Header-only
- **CMake Target**: `glaze::glaze`

### Method 1: CMake FetchContent (Recommended)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v3.6.3  # Use specific version, not 'main'
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

target_link_libraries(your_target PRIVATE glaze::glaze)
```

### Method 2: System Package Manager

#### Arch Linux
```bash
sudo pacman -S glaze
# Or from AUR:
yay -S glaze-git
```

#### Ubuntu/Debian
Currently not in official repositories. Use FetchContent or manual installation.

#### Conan
```bash
conan install glaze/[>=3.0.0]
```

```cmake
find_package(glaze REQUIRED)
target_link_libraries(your_target PRIVATE glaze::glaze)
```

### Method 3: Manual Installation (Header-Only)

```bash
git clone https://github.com/stephenberry/glaze.git
cd glaze
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
sudo make install
```

Or simply copy headers:
```bash
git clone https://github.com/stephenberry/glaze.git
sudo cp -r glaze/include/glaze /usr/local/include/
```

### CMake Integration in asyncle

```cmake
if(FORMAT_ENABLE_GLAZE)
    find_package(glaze QUIET)
    if(glaze_FOUND)
        message(STATUS "Glaze found (version ${glaze_VERSION})")
        target_compile_definitions(format INTERFACE FORMAT_HAS_GLAZE)
        target_link_libraries(format INTERFACE glaze::glaze)
    else()
        message(WARNING "Glaze not found")
        message(WARNING "  Install: Use FetchContent (see docs/EXTERNAL_DEPENDENCIES.md)")
        message(WARNING "  Or use: -DFORMAT_USE_FETCHCONTENT=ON")
    endif()
endif()
```

## asyncle CMake Options

### Build Options

```cmake
# Core options
option(ASYNCLE_BUILD_TESTS "Build tests" ON)

# Format library options
option(FORMAT_ENABLE_SIMDJSON "Enable simdjson JSON parser" OFF)
option(FORMAT_ENABLE_GLAZE "Enable Glaze JSON parser" OFF)
option(FORMAT_USE_FETCHCONTENT "Auto-fetch missing dependencies" OFF)
```

### Usage Examples

#### Production Build (System Packages)
```bash
# Install dependencies first
sudo apt install libsimdjson-dev

# Build asyncle
cmake -B build \
  -DFORMAT_ENABLE_SIMDJSON=ON \
  -DFORMAT_ENABLE_GLAZE=ON \
  -DASYNCLE_BUILD_TESTS=ON

cmake --build build
ctest --test-dir build
```

#### Development Build (FetchContent)
```bash
# No dependencies needed, will auto-fetch
cmake -B build \
  -DFORMAT_ENABLE_SIMDJSON=ON \
  -DFORMAT_ENABLE_GLAZE=ON \
  -DFORMAT_USE_FETCHCONTENT=ON \
  -DASYNCLE_BUILD_TESTS=ON

cmake --build build
ctest --test-dir build
```

#### Minimal Build (No Optional Dependencies)
```bash
cmake -B build \
  -DFORMAT_ENABLE_SIMDJSON=OFF \
  -DFORMAT_ENABLE_GLAZE=OFF

cmake --build build
```

## Dependency Version Requirements

| Library | Minimum Version | Tested Version | Notes |
|---------|----------------|----------------|-------|
| simdjson | 3.0.0 | 3.10.1 | C++17 required |
| Glaze | 3.0.0 | 3.6.3 | C++23 required |

## Troubleshooting

### simdjson not found

```bash
# Check if installed
pkg-config --modversion simdjson

# If not found, install
sudo apt install libsimdjson-dev

# Or use FetchContent
cmake -DFORMAT_USE_FETCHCONTENT=ON ..
```

### Glaze not found

```bash
# Glaze is header-only, usually needs FetchContent
cmake -DFORMAT_USE_FETCHCONTENT=ON ..

# Or install to system
git clone https://github.com/stephenberry/glaze.git
cd glaze && mkdir build && cd build
cmake .. && sudo make install
```

### Compiler Errors with Glaze

Glaze requires C++23. Ensure your compiler supports it:
```bash
g++ --version  # Need GCC 12+
clang++ --version  # Need Clang 17+
```

### Link Errors with simdjson

simdjson is not header-only. Ensure the library is linked:
```cmake
target_link_libraries(your_target PRIVATE simdjson::simdjson)
```

## Best Practices

### 1. Pin Dependency Versions

**Good**:
```cmake
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG v3.10.1  # Specific version
  GIT_SHALLOW TRUE
)
```

**Bad**:
```cmake
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG main  # Unstable, can break builds
)
```

### 2. Prefer System Packages for Production

System packages are:
- Tested and stable
- Maintained by distribution
- Shared across projects (smaller disk usage)
- Faster to build (already compiled)

### 3. Use FetchContent for Development

FetchContent is good for:
- Quick setup without system dependencies
- Testing specific versions
- CI/CD environments
- Development machines

### 4. Check Feature Availability

```cpp
#ifdef FORMAT_HAS_SIMDJSON
    format::json::simdjson_parser parser;
#else
    #error "simdjson not available"
#endif
```

### 5. Document Dependencies in README

Users should know what optional features require which dependencies.

## CI/CD Considerations

### GitHub Actions Example

```yaml
name: Build

on: [push, pull_request]

jobs:
  build-with-deps:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y libsimdjson-dev

      - name: Configure CMake
        run: |
          cmake -B build \
            -DFORMAT_ENABLE_SIMDJSON=ON \
            -DFORMAT_ENABLE_GLAZE=ON \
            -DFORMAT_USE_FETCHCONTENT=ON

      - name: Build
        run: cmake --build build

      - name: Test
        run: ctest --test-dir build --output-on-failure
```

## License Compatibility

| Library | License | Compatible with Asyncle |
|---------|---------|-------------------------|
| simdjson | Apache 2.0 | ✅ Yes |
| Glaze | MIT | ✅ Yes |

Both licenses are permissive and compatible with commercial use.

## Future Dependencies

When adding new optional dependencies:

1. **Evaluate necessity**: Is it really needed, or can we implement ourselves?
2. **Check license**: Must be permissive (MIT, Apache, BSD)
3. **Assess quality**: Well-maintained? Active community?
4. **Consider size**: Header-only preferred for optional features
5. **Document thoroughly**: Update this file and relevant docs

## See Also

- [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md) - Format layer design
- [FORMAT_JSON_EXAMPLES.md](FORMAT_JSON_EXAMPLES.md) - Usage examples
- [CMakeLists.txt](../CMakeLists.txt) - Build configuration
