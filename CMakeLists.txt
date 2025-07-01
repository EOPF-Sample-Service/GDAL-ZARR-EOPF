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

# Print GDAL information for debugging
message(STATUS "GDAL Version: ${GDAL_VERSION}")
message(STATUS "GDAL Include Dirs: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL Libraries: ${GDAL_LIBRARIES}")

# Add this compatibility check
include(CheckCXXSourceCompiles)
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
    add_definitions(-DGDAL_HAS_CONST_XML_NODE)
    message(STATUS "GDAL XMLInit uses const CPLXMLNode*")
else()
    message(STATUS "GDAL XMLInit uses non-const CPLXMLNode*")
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

# Windows-specific configuration for GitHub Actions compatibility
if(WIN32)
    # Detect if we're in GitHub Actions or similar CI environment
    if(DEFINED ENV{GITHUB_ACTIONS} OR DEFINED ENV{CI})
        message(STATUS "Detected CI environment - using compatibility settings")
        
        # Force dynamic runtime to match most GDAL distributions
        set_property(TARGET gdal_EOPFZarr PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        
        # Ensure we don't have conflicting runtime settings
        target_compile_options(gdal_EOPFZarr PRIVATE
            $<$<CONFIG:Release>:/MD>
            $<$<CONFIG:Debug>:/MDd>
        )
    else()
        message(STATUS "Local build detected - using default settings")
        # Let CMake decide the runtime library for local builds
    endif()
    
    # Common Windows settings
    target_compile_definitions(gdal_EOPFZarr PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    # Add build information for debugging
    add_custom_command(TARGET gdal_EOPFZarr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "=== Build Environment ==="
        COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
        COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E echo "Target File: $<TARGET_FILE:gdal_EOPFZarr>"
        VERBATIM
    )
endif()

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
             COMMAND cmd.exe /C "set GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} && gdalinfo --formats | findstr EOPFZARR")
elseif(APPLE)
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()