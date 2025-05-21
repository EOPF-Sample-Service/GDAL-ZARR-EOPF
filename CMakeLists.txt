cmake_minimum_required(VERSION 3.14)
project(EOPFZarr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Find GDAL using the modern imported target
find_package(GDAL REQUIRED)

add_library(gdal_EOPFZarr MODULE
    src/eopfzarr_driver.cpp
    src/eopfzarr_dataset.cpp
    src/eopf_metadata.cpp
)

target_link_libraries(gdal_EOPFZarr PRIVATE GDAL::GDAL)

# Output properties for plugin
if (WIN32)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        OUTPUT_NAME "gdal_EOPFZarr"
        SUFFIX ".dll")
else()
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        SUFFIX ".so")
endif()

enable_testing()
if(WIN32)
    add_test(NAME driver_registration 
             COMMAND cmd.exe /C "set GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} && gdalinfo --formats | findstr EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()
