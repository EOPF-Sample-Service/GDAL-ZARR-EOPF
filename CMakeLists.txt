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
# Try multiple methods to find GDAL for better compatibility
if(GDAL_ROOT)
    # If GDAL_ROOT is provided, use it to set hints
    file(TO_CMAKE_PATH "${GDAL_ROOT}" GDAL_ROOT_CMAKE)
    set(GDAL_INCLUDE_DIR "${GDAL_ROOT_CMAKE}/include")
    
    # Try different possible library names for GDAL
    set(GDAL_POSSIBLE_LIBS 
        "${GDAL_ROOT_CMAKE}/lib/gdal.lib"
        "${GDAL_ROOT_CMAKE}/lib/gdal_i.lib"
        "${GDAL_ROOT_CMAKE}/lib/gdal310.lib"
    )
    
    foreach(lib_path ${GDAL_POSSIBLE_LIBS})
        if(EXISTS "${lib_path}")
            set(GDAL_LIBRARY "${lib_path}")
            message(STATUS "Found GDAL library: ${lib_path}")
            break()
        endif()
    endforeach()
    
    message(STATUS "Using GDAL_ROOT hint: ${GDAL_ROOT_CMAKE}")
    message(STATUS "GDAL_INCLUDE_DIR: ${GDAL_INCLUDE_DIR}")
    message(STATUS "GDAL_LIBRARY: ${GDAL_LIBRARY}")
    
    # Verify the paths exist
    if(EXISTS "${GDAL_INCLUDE_DIR}")
        message(STATUS "✓ GDAL include directory exists")
    else()
        message(WARNING "✗ GDAL include directory not found: ${GDAL_INCLUDE_DIR}")
    endif()
    
    if(GDAL_LIBRARY AND EXISTS "${GDAL_LIBRARY}")
        message(STATUS "✓ GDAL library exists")
    else()
        message(WARNING "✗ GDAL library not found: ${GDAL_LIBRARY}")
    endif()
endif()

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
)

target_include_directories(gdal_EOPFZarr PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(gdal_EOPFZarr PRIVATE GDAL::GDAL)

# Output properties for plugin
if (WIN32)
    # Static runtime linking to reduce dependencies
    set_property(TARGET gdal_EOPFZarr PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
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
    
    # Additional deployment options for GitHub releases
    option(EOPFZARR_STATIC_DEPLOYMENT "Build for static deployment (GitHub releases)" OFF)
    if(EOPFZARR_STATIC_DEPLOYMENT)
        message(STATUS "Building for static deployment (GitHub releases)")
        # Static linking flags
        target_link_options(gdal_EOPFZarr PRIVATE "/MT")
        # Ensure static CRT linking
        set_property(TARGET gdal_EOPFZarr PROPERTY 
            MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
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
             COMMAND cmd.exe /C "set GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} && gdalinfo --formats | findstr EOPFZARR")
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
if(GDAL_INCLUDE_DIR)
    set(CMAKE_REQUIRED_INCLUDES "${GDAL_INCLUDE_DIR}")
else()
    set(CMAKE_REQUIRED_INCLUDES ${GDAL_INCLUDE_DIRS})
endif()

if(GDAL_LIBRARY)
    # Convert path to proper format for CMake
    file(TO_CMAKE_PATH "${GDAL_LIBRARY}" GDAL_LIBRARY_CMAKE)
    set(CMAKE_REQUIRED_LIBRARIES "${GDAL_LIBRARY_CMAKE}")
else()
    set(CMAKE_REQUIRED_LIBRARIES ${GDAL_LIBRARIES})
endif()

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