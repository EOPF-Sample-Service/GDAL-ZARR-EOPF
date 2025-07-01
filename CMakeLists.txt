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

# Windows-specific configuration for maximum compatibility
if(WIN32)
    # Check if we're building in GitHub Actions or other CI environment
    if(DEFINED ENV{GITHUB_ACTIONS} OR DEFINED ENV{CI})
        # For CI builds, use dynamic runtime to match OSGeo4W and most distributions
        set_property(TARGET gdal_EOPFZarr PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        
        target_compile_options(gdal_EOPFZarr PRIVATE
            $<$<CONFIG:Release>:/MD>
            $<$<CONFIG:Debug>:/MDd>
        )
        message(STATUS "CI Build: Using dynamic runtime (/MD) for GDAL ${GDAL_VERSION}")
    else()
        # For local builds, use static runtime to avoid dependency issues
        set_property(TARGET gdal_EOPFZarr PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        
        target_compile_options(gdal_EOPFZarr PRIVATE
            $<$<CONFIG:Release>:/MT>
            $<$<CONFIG:Debug>:/MTd>
        )
        message(STATUS "Local Build: Using static runtime (/MT) for GDAL ${GDAL_VERSION}")
    endif()
    
    # Add version info for compatibility checking
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" GDAL_VERSION_MAJOR_MINOR ${GDAL_VERSION})
    target_compile_definitions(gdal_EOPFZarr PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        GDAL_VERSION_MAJOR_MINOR="${GDAL_VERSION_MAJOR_MINOR}"
    )
    
    # Add comprehensive build information
    add_custom_command(TARGET gdal_EOPFZarr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "=== Build Environment ==="
        COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
        COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E echo "GDAL Version: ${GDAL_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E echo "Runtime Library: Dynamic (/MD)"
        COMMAND ${CMAKE_COMMAND} -E echo "Target File: $<TARGET_FILE:gdal_EOPFZarr>"
        COMMAND ${CMAKE_COMMAND} -E echo "=== Dependency Check ==="
        COMMAND dumpbin /dependents $<TARGET_FILE:gdal_EOPFZarr> || echo "dumpbin not available"
        VERBATIM
    )
endif()

# Output properties for plugin
if (WIN32)
    # Include GDAL version in output name for clarity
    string(REGEX REPLACE "\\." "_" GDAL_VERSION_UNDERSCORE ${GDAL_VERSION})
    
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        OUTPUT_NAME "gdal_EOPFZarr_v${GDAL_VERSION_UNDERSCORE}"
        SUFFIX ".dll")

    # Simplified .def file - export the correct function names
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/eopfzarr_exports.def")
    file(WRITE ${DEF_FILE} "EXPORTS\n")
    file(APPEND ${DEF_FILE} "GDALRegisterMe\n")
    file(APPEND ${DEF_FILE} "GDALRegister_EOPFZarr\n")
    file(APPEND ${DEF_FILE} "GDALDeregisterEOPFZarr\n")

    # Add the .def file to the library
    target_link_options(gdal_EOPFZarr PRIVATE "/DEF:${DEF_FILE}")
    
    # Remove delay load to avoid issues
    # target_link_options(gdal_EOPFZarr PRIVATE
    #     "/DELAYLOAD:shell32.dll"
    #     "/DELAYLOAD:user32.dll"
    # )
    
elseif (APPLE)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".dylib")
else()
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".so")
endif()

# Create a generic version without version suffix for local use
if(WIN32)
    add_custom_command(TARGET gdal_EOPFZarr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
            $<TARGET_FILE:gdal_EOPFZarr>
            $<TARGET_FILE_DIR:gdal_EOPFZarr>/gdal_EOPFZarr.dll
        COMMENT "Creating generic DLL copy"
    )
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

# Also install the generic version
if(WIN32)
    install(FILES $<TARGET_FILE_DIR:gdal_EOPFZarr>/gdal_EOPFZarr.dll
        DESTINATION ${GDAL_PLUGINS_DIR}
    )
endif()

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