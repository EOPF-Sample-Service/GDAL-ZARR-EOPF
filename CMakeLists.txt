cmake_minimum_required(VERSION 3.14)
project(EOPFZarr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Include CTest for testing capabilities
include(CTest)
enable_testing()

# Configuration options
option(USE_FILENAME_PREFIX "Use filename prefix to identify EOPF datasets" ON)
set(EOPF_FILENAME_PREFIX "eopf:" CACHE STRING "Filename prefix to identify EOPF datasets")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/eopfzarr_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/eopfzarr_config.h"
)

# Find GDAL using the modern imported target
find_package(GDAL REQUIRED)

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
elseif (APPLE)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".dylib")
else()
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".so")
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
             COMMAND bash -c "export GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} && gdalinfo --formats | grep -q EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()
