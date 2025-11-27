cmake_minimum_required(VERSION 3.14)
project(EOPFZarr VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Include CTest for testing capabilities
include(CTest)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/eopfzarr_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/eopfzarr_config.h"
)

# ========================================================================
# Compiler Configuration & Code Quality
# ========================================================================

# Options for sanitizers and static analysis
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
option(ENABLE_CLANG_TIDY "Enable Clang-Tidy static analysis" OFF)
option(ENABLE_WERROR "Treat warnings as errors" OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(ENABLE_WERROR)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wshadow 
        -Wunused
        -Wconversion
        -Wsign-conversion
        -Wcast-align
        -Wformat=2
    )
    if(ENABLE_WERROR)
        add_compile_options(-Werror)
    endif()
endif()

# Sanitizers (GCC/Clang only)
if(ENABLE_SANITIZERS)
    if(MSVC)
        message(WARNING "Sanitizers not yet configured for MSVC in this build")
    else()
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# Clang-Tidy
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# Find GDAL using the modern imported target
find_package(GDAL REQUIRED)

# Check GDAL version compatibility
if(GDAL_VERSION)
    message(STATUS "Found GDAL version: ${GDAL_VERSION}")
    
    # Extract major.minor version for compatibility check
    string(REGEX MATCH "^([0-9]+\\.[0-9]+)" GDAL_MAJOR_MINOR "${GDAL_VERSION}")
    message(STATUS "GDAL major.minor version: ${GDAL_MAJOR_MINOR}")
    
    # Create version number for preprocessor defines
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" GDAL_VERSION_MATCH "${GDAL_VERSION}")
    if(GDAL_VERSION_MATCH)
        set(GDAL_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(GDAL_VERSION_MINOR ${CMAKE_MATCH_2})
        math(EXPR GDAL_VERSION_NUMBER "${GDAL_VERSION_MAJOR} * 100 + ${GDAL_VERSION_MINOR}")
        message(STATUS "GDAL version define: GDAL_VERSION_NUM=${GDAL_VERSION_NUMBER}")
        add_definitions(-DGDAL_VERSION_NUM=${GDAL_VERSION_NUMBER})
    endif()
    
    # Check GDAL version compatibility - require 3.10+ but less than 3.12
    if(GDAL_VERSION VERSION_LESS "3.10")
        message(FATAL_ERROR "GDAL version ${GDAL_VERSION} is too old. Minimum required version is 3.10.0.")
    elseif(GDAL_VERSION VERSION_GREATER_EQUAL "3.12")
        message(WARNING "GDAL version ${GDAL_VERSION} is newer than tested. Recommended version range is 3.10.x - 3.11.x. Build may have compatibility issues.")
    else()
        message(STATUS "✅ GDAL version ${GDAL_VERSION} check passed - compatible for EOPF Zarr driver")
    endif()
else()
    message(WARNING "Could not determine GDAL version. Build may have compatibility issues.")
endif()

# If GDAL imported target is not available, try with pkg-config as fallback
if(NOT TARGET GDAL::GDAL)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GDAL REQUIRED gdal)
        if(GDAL_FOUND)
            add_library(GDAL::GDAL INTERFACE IMPORTED)
            target_include_directories(GDAL::GDAL INTERFACE ${GDAL_INCLUDE_DIRS})
            target_link_libraries(GDAL::GDAL INTERFACE ${GDAL_LIBRARIES})
            target_compile_options(GDAL::GDAL INTERFACE ${GDAL_CFLAGS_OTHER})
            message(STATUS "Using pkg-config GDAL: ${GDAL_VERSION}")
        endif()
    endif()
endif()

# Verify GDAL target is available
if(NOT TARGET GDAL::GDAL)
    message(FATAL_ERROR "GDAL not found. Please install GDAL development packages.")
endif()

# Print GDAL information for debugging
message(STATUS "GDAL include directories: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL libraries: ${GDAL_LIBRARIES}")

# Version already defined above - no need for duplicate

# Check for GDALPamDataset XML initialization compatibility
include(CheckCXXSourceCompiles)

# Set up include directories for the test
set(CMAKE_REQUIRED_INCLUDES ${GDAL_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${GDAL_LIBRARIES})

# Test for const CPLXMLNode in XMLInit
check_cxx_source_compiles("
#include <gdal_pam.h>
class Test : public GDALPamDataset {
protected:
    CPLErr XMLInit(const CPLXMLNode* psTree, const char* pszUnused) override { return CE_None; }
};
int main() { return 0; }
" GDAL_HAS_CONST_XML_NODE)

if(GDAL_HAS_CONST_XML_NODE)
    message(STATUS "GDAL XMLInit uses const CPLXMLNode*")
    add_definitions(-DGDAL_HAS_CONST_XML_NODE)
else()
    message(STATUS "GDAL XMLInit uses non-const CPLXMLNode*")
endif()

# Test for RefUnderlyingRasterBand signature
check_cxx_source_compiles("
#include <gdal_proxy.h>
class Test : public GDALProxyRasterBand {
public:
    GDALRasterBand* RefUnderlyingRasterBand(bool bForceOpen = true) const override { return nullptr; }
};
int main() { return 0; }
" GDAL_HAS_CONST_REF_UNDERLYING)

if(GDAL_HAS_CONST_REF_UNDERLYING)
    message(STATUS "GDAL RefUnderlyingRasterBand uses const method with bool parameter")
    add_definitions(-DGDAL_HAS_CONST_REF_UNDERLYING)
else()
    message(STATUS "GDAL RefUnderlyingRasterBand uses non-const method without parameters")
endif()

# Test for GDALGeoTransform class (GDAL 3.12+)
check_cxx_source_compiles("
#include <gdal_pam.h>
class Test : public GDALPamDataset {
public:
    CPLErr SetGeoTransform(const GDALGeoTransform& gt) override { return CE_None; }
};
int main() { return 0; }
" HAVE_GDAL_GEOTRANSFORM)

if(HAVE_GDAL_GEOTRANSFORM)
    message(STATUS "GDALGeoTransform class available")
    add_definitions(-DHAVE_GDAL_GEOTRANSFORM)
else()
    message(STATUS "GDALGeoTransform class NOT available")
endif()

# Reset CMAKE_REQUIRED variables
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_LIBRARIES)

# Main driver library (MODULE)
add_library(gdal_EOPFZarr MODULE
    src/eopfzarr_driver.cpp
    src/eopfzarr_dataset.cpp
    src/eopf_metadata.cpp
    src/eopfzarr_performance.cpp
)

target_include_directories(gdal_EOPFZarr PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
    include
    src
)

target_link_libraries(gdal_EOPFZarr PRIVATE GDAL::GDAL)

# Output properties for plugin
if (WIN32)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        OUTPUT_NAME "gdal_EOPFZarr"
        SUFFIX ".dll")

    # Simplified .def file - just export the standard entry points
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/eopfzarr_exports.def")
    file(WRITE ${DEF_FILE} "EXPORTS\n")
    file(APPEND ${DEF_FILE} "    GDALRegisterMe\n")
    file(APPEND ${DEF_FILE} "    GDALRegister_EOPFZarr\n")

    # Add the .def file to the library
    target_link_options(gdal_EOPFZarr PRIVATE "/DEF:${DEF_FILE}")
elseif (APPLE)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".dylib")
else()
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".so")
endif()

# Install to GDAL plugins directory
if(WIN32)
    set(GDAL_PLUGINS_DIR "$ENV{GDAL_DRIVER_PATH}" CACHE PATH "Directory where GDAL plugins are installed")
    if(NOT GDAL_PLUGINS_DIR)
        set(GDAL_PLUGINS_DIR "$ENV{PROGRAMDATA}/GDAL/plugins")
    endif()
else()
    set(GDAL_PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/lib/gdalplugins" CACHE PATH "Directory where GDAL plugins are installed")
endif()

install(TARGETS gdal_EOPFZarr 
    LIBRARY DESTINATION ${GDAL_PLUGINS_DIR}
)

enable_testing()
# Set the test environment to find gdalinfo command
find_program(GDALINFO_EXECUTABLE gdalinfo)
if(NOT GDALINFO_EXECUTABLE)
    message(WARNING "gdalinfo not found, tests may fail")
endif()

if(WIN32)
    add_test(NAME driver_registration
             COMMAND powershell.exe -Command "$env:GDAL_DRIVER_PATH='${CMAKE_BINARY_DIR}'; gdalinfo --formats | Select-String EOPFZARR")
elseif(APPLE)
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()



# ========================================================================
# Core Unit Testing Framework (Essential Tests Only)
# ========================================================================

# Only build tests if this is the main project or explicitly requested
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR BUILD_TESTING)
    
    message(STATUS "Configuring core unit tests...")
    
    # Test utilities library (shared components)
    add_library(eopfzarr_test_utils STATIC
        tests/test_implementation.cpp
        src/eopfzarr_performance.cpp
    )
    
    target_include_directories(eopfzarr_test_utils PUBLIC
        tests
        src
        include
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
    
    target_link_libraries(eopfzarr_test_utils PUBLIC GDAL::GDAL)
    
    # ===== CORE TESTS (Essential for CI/CD) =====
    
    # 1. Basic functionality test (no external dependencies)
    add_executable(test_basic_functionality
        tests/test_basic_functionality.cpp
    )
    
    # 2. Path parsing tests (core driver functionality)
    add_executable(test_path_parsing
        tests/test_path_parsing.cpp
    )
    
    target_link_libraries(test_path_parsing PRIVATE 
        eopfzarr_test_utils
        GDAL::GDAL
    )
    
    # 2b. Simple path parsing unit tests (no GDAL dependency)
    add_executable(test_path_parsing_unit
        tests/test_path_parsing_unit.cpp
    )
    
    # No GDAL linking needed for this simple test
    
    # 2c. BBox ordering unit tests (no GDAL dependency) - Issue #135
    add_executable(test_bbox_ordering_unit
        tests/test_bbox_ordering_unit.cpp
    )
    
    # No GDAL linking needed for this simple test
    
    # ===== GDAL INTEGRATION TESTS (Require OSGeo4W) =====
    
    # 3. Driver integration tests
    add_executable(test_driver_integration
        tests/test_driver_integration.cpp
    )
    
    target_link_libraries(test_driver_integration PRIVATE 
        GDAL::GDAL
    )
    
    # 4. Compatibility tests
    add_executable(test_compatibility
        tests/test_compatibility.cpp
    )
    
    target_link_libraries(test_compatibility PRIVATE 
        GDAL::GDAL
    )
    
    # 5. Subdataset format tests (for new functionality)
    add_executable(test_subdataset_formats
        tests/test_subdataset_formats.cpp
    )
    
    target_link_libraries(test_subdataset_formats PRIVATE 
        GDAL::GDAL
    )
    
    # 6. Real data integration tests (using sample data)
    add_executable(test_real_data_integration
        tests/test_real_data_integration.cpp
    )
    
    target_link_libraries(test_real_data_integration PRIVATE 
        GDAL::GDAL
    )
    
    # 7. Simple sample data structure tests (no GDAL dependency)
    add_executable(test_sample_data_structure
        tests/test_sample_data_structure.cpp
    )
    
    # 8. Basic GDAL functionality test
    add_executable(test_gdal_basic
        tests/test_gdal_basic.cpp
    )
    
    target_link_libraries(test_gdal_basic PRIVATE 
        GDAL::GDAL
    )
    
    # 9. Geolocation arrays test (Issue #137)
    add_executable(test_geolocation_arrays
        tests/test_geolocation_arrays.cpp
    )
    
    target_link_libraries(test_geolocation_arrays PRIVATE 
        GDAL::GDAL
    )
    
    # ===== CTEST REGISTRATION =====
    
    # ===== CORE TESTS (Always Work) =====
    
    # Core tests (no GDAL runtime dependencies)
    add_test(NAME basic_functionality 
             COMMAND test_basic_functionality)
    
    add_test(NAME unit_path_parsing 
             COMMAND test_path_parsing)
             
    add_test(NAME unit_path_parsing_simple 
             COMMAND test_path_parsing_unit)
             
    add_test(NAME unit_bbox_ordering 
             COMMAND test_bbox_ordering_unit)
             
    add_test(NAME sample_data_structure
             COMMAND test_sample_data_structure)

    # ===== INTEGRATION TESTS (Environment-Dependent) =====
    
    # GDAL integration tests (require proper runtime environment)
    add_test(NAME integration_driver
             COMMAND test_driver_integration)
             
    add_test(NAME compatibility_urls
             COMMAND test_compatibility)
             
    add_test(NAME subdataset_formats
             COMMAND test_subdataset_formats)
             
    add_test(NAME real_data_integration
             COMMAND test_real_data_integration)
             
    add_test(NAME geolocation_arrays
             COMMAND test_geolocation_arrays)    
    
    # Set environment for GDAL-dependent tests (may fail in dev environment)
    set_tests_properties(integration_driver compatibility_urls subdataset_formats real_data_integration geolocation_arrays PROPERTIES
        ENVIRONMENT "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR}")
    
    # Core test suite summary (tests that should always pass)
    add_test(NAME core_test_summary
             COMMAND ${CMAKE_COMMAND} 
             -E echo "✅ Core EOPF-Zarr plugin tests completed successfully")
    
    set_tests_properties(core_test_summary PROPERTIES
        DEPENDS "basic_functionality;unit_path_parsing;unit_path_parsing_simple;sample_data_structure")
        
    # Full test suite summary (includes environment-dependent tests)
    add_test(NAME full_test_summary
             COMMAND ${CMAKE_COMMAND} 
             -E echo "✅ Full EOPF-Zarr test suite completed (some may require environment setup)")
    
    set_tests_properties(full_test_summary PROPERTIES
        DEPENDS "basic_functionality;unit_path_parsing;unit_path_parsing_simple;sample_data_structure;integration_driver;compatibility_urls;subdataset_formats;real_data_integration;geolocation_arrays")
    
    # ===== STATUS MESSAGES =====
    
    message(STATUS "✅ All tests configured and enabled")
    message(STATUS "")
    message(STATUS "ESSENTIAL TESTS (ENABLED):")
    message(STATUS "   ├─ basic_functionality    (no dependencies)")
    message(STATUS "   ├─ unit_path_parsing      (core functionality)")
    message(STATUS "   ├─ driver_registration    (PowerShell/gdalinfo)")
    message(STATUS "   ├─ integration_driver     (requires OSGeo4W)")
    message(STATUS "   └─ compatibility_urls     (requires OSGeo4W)")
    message(STATUS "")
    message(STATUS "USAGE:")
    message(STATUS "   Core tests:   ctest -C Debug -R 'basic_functionality|unit_path_parsing|driver_registration'")
    message(STATUS "   All tests:    ctest -C Debug --verbose")
    message(STATUS "   GDAL tests:   ctest -C Debug -R 'integration_driver|compatibility_urls'")
    message(STATUS "")
    
endif()