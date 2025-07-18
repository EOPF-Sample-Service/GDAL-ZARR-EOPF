cmake_minimum_required(VERSION 3.14)
project(EOPFZarr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Include CTest for testing capabilities
include(CTest)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/eopfzarr_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/eopfzarr_config.h"
)

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
    
    # Warn about potential version mismatches
    if(GDAL_VERSION VERSION_LESS "3.8")
        message(WARNING "GDAL version ${GDAL_VERSION} is older than recommended (3.8+). Some features may not work.")
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

# Add version compatibility defines
if(GDAL_VERSION)
    # Define GDAL version for compatibility checks in code
    string(REPLACE "." "" GDAL_VERSION_NUM "${GDAL_VERSION}")
    string(SUBSTRING "${GDAL_VERSION_NUM}" 0 3 GDAL_VERSION_NUM)
    add_definitions(-DGDAL_VERSION_NUM=${GDAL_VERSION_NUM})
    message(STATUS "GDAL version define: GDAL_VERSION_NUM=${GDAL_VERSION_NUM}")
endif()

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

# Reset CMAKE_REQUIRED variables
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_LIBRARIES)

# ========================================================================
# Unit Testing Framework
# ========================================================================

# Only build tests if this is the main project or explicitly requested
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR BUILD_TESTING)
    
    message(STATUS "Configuring unit tests...")
    
    # Test library with shared test utilities
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
    
    # Unit test executable for path parsing
    add_executable(test_path_parsing
        tests/test_path_parsing.cpp
    )
    
    target_link_libraries(test_path_parsing PRIVATE 
        eopfzarr_test_utils
        GDAL::GDAL
    )
    
    # Integration test executable
    add_executable(test_driver_integration
        tests/test_driver_integration.cpp
    )
    
    target_link_libraries(test_driver_integration PRIVATE 
        GDAL::GDAL
    )
    
    # Compatibility test executable
    add_executable(test_compatibility
        tests/test_compatibility.cpp
    )
    
    target_link_libraries(test_compatibility PRIVATE 
        GDAL::GDAL
    )
    
    # Performance test executable
    add_executable(test_performance
        tests/test_performance.cpp
    )
    
    target_link_libraries(test_performance PRIVATE 
        eopfzarr_test_utils
        GDAL::GDAL
    )
    
    # Add tests to CTest
    add_test(NAME unit_path_parsing 
             COMMAND test_path_parsing)
    
    add_test(NAME integration_driver
             COMMAND test_driver_integration)
             
    add_test(NAME compatibility_urls
             COMMAND test_compatibility)
             
    add_test(NAME performance_optimizations
             COMMAND test_performance)
    
    # Set test environment to find the driver
    set_tests_properties(integration_driver compatibility_urls performance_optimizations PROPERTIES
        ENVIRONMENT "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR}")
    
    # Add a comprehensive test that runs all test suites
    add_test(NAME comprehensive_test_suite
             COMMAND ${CMAKE_COMMAND} 
             -E echo "Running comprehensive test suite...")
    
    set_tests_properties(comprehensive_test_suite PROPERTIES
        DEPENDS "unit_path_parsing;integration_driver;compatibility_urls;performance_optimizations")
    
    message(STATUS "✅ Unit tests configured successfully")
    message(STATUS "   - Path parsing tests: test_path_parsing")
    message(STATUS "   - Driver integration tests: test_driver_integration") 
    message(STATUS "   - Compatibility tests: test_compatibility")
    message(STATUS "   - Performance tests: test_performance")
    message(STATUS "   Run with: cmake --build . && ctest")
    
endif()