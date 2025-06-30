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
        endif()
    endif()
endif()

# Verify GDAL target is available
if(NOT TARGET GDAL::GDAL)
    message(FATAL_ERROR "GDAL not found. Please install GDAL development packages.")
endif()

# Print GDAL information for debugging
message(STATUS "Found GDAL version: ${GDAL_VERSION}")
if(GDAL_INCLUDE_DIRS)
    message(STATUS "GDAL include directories: ${GDAL_INCLUDE_DIRS}")
endif()
if(GDAL_LIBRARIES)  
    message(STATUS "GDAL libraries: ${GDAL_LIBRARIES}")
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
    # Use the correct configuration-specific directory for the DLL
    add_test(NAME driver_registration
             COMMAND gdalinfo --formats)
    set_tests_properties(driver_registration PROPERTIES
        ENVIRONMENT "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR}/Debug"
        PASS_REGULAR_EXPRESSION "EOPFZARR")
             
elseif(APPLE)

    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()

# Check for GDALPamDataset XML initialization compatibility
include(CheckCXXSourceCompiles)

# Set up the required variables for the test
set(CMAKE_REQUIRED_INCLUDES ${GDAL_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${GDAL_LIBRARIES})

check_cxx_source_compiles("
#include <gdal_pam.h>
class Test : public GDALPamDataset {
protected:
    CPLErr XMLInit(const CPLXMLNode* psTree, const char* pszUnused) override { return CE_None; }
};
int main() { return 0; }
" GDAL_HAS_CONST_XML_NODE)

if(GDAL_HAS_CONST_XML_NODE)
    target_compile_definitions(gdal_EOPFZarr PRIVATE GDAL_HAS_CONST_XML_NODE)
    message(STATUS "GDAL XMLInit uses const CPLXMLNode*")
else()
    message(STATUS "GDAL XMLInit uses non-const CPLXMLNode*")
endif()

# Windows-specific dependency handling
if(WIN32)
    get_filename_component(GDAL_BIN_DIR "${GDAL_LIBRARIES}" DIRECTORY)
    get_filename_component(GDAL_BIN_DIR "${GDAL_BIN_DIR}/../bin" ABSOLUTE)
    message(STATUS "GDAL binary directory: ${GDAL_BIN_DIR}")
    
    # Update test environment
    set_tests_properties(driver_registration PROPERTIES
        ENVIRONMENT "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR}/Release;PATH=${GDAL_BIN_DIR};$ENV{PATH}"
        PASS_REGULAR_EXPRESSION "EOPFZARR")
endif()