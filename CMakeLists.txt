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


if(WIN32 AND MSVC)
    # Static link runtime to avoid dependency issues
    target_compile_options(gdal_EOPFZarr PRIVATE
        $<$<CONFIG:Release>:/MT>
        $<$<CONFIG:Debug>:/MTd>
    )

    # Use static runtime to reduce dependencies
    set_property(TARGET gdal_EOPFZarr PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Add debug information for dependency checking
    add_custom_command(TARGET gdal_EOPFZarr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Built DLL: $<TARGET_FILE:gdal_EOPFZarr>"
        COMMAND dumpbin /dependents $<TARGET_FILE:gdal_EOPFZarr> || echo "dumpbin not available"
        VERBATIM
    )
    
    # Ensure proper exports
    set_target_properties(gdal_EOPFZarr PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
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
       # Get GDAL binary directory for runtime
    get_filename_component(GDAL_BIN_DIR "${GDAL_LIBRARIES}" DIRECTORY)
    get_filename_component(GDAL_BIN_DIR "${GDAL_BIN_DIR}/../bin" ABSOLUTE)
    message(STATUS "GDAL binary directory: ${GDAL_BIN_DIR}")
    
    add_test(NAME driver_registration
             COMMAND gdalinfo --formats)
    set_tests_properties(driver_registration PROPERTIES
        ENVIRONMENT "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR}/Release;PATH=${GDAL_BIN_DIR};$ENV{PATH}"
        PASS_REGULAR_EXPRESSION "EOPFZARR")
elseif(APPLE)
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()

# Add this to your CMakeLists.txt for Windows
if(WIN32)
    # Find and copy required runtime DLLs
    include(InstallRequiredSystemLibraries)
    
    # Custom post-build step to copy dependencies
    add_custom_command(TARGET gdal_EOPFZarr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Checking DLL dependencies..."
        COMMAND dumpbin /dependents $<TARGET_FILE:gdal_EOPFZarr>
        VERBATIM
    )
endif()