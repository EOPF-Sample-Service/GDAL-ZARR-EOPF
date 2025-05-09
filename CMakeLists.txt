cmake_minimum_required(VERSION 3.10)
project(EOPFZarr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(GDAL CONFIG REQUIRED)

add_library(gdal_EOPFZarr MODULE
    src/eopfzarr_driver.cpp
    src/eopfzarr_dataset.cpp
    )

target_link_libraries(gdal_EOPFZarr PRIVATE GDAL::GDAL)

# produce gdal_EOPFZarr.so  (no "lib" prefix –  mandatory for plugins)
set_target_properties(gdal_EOPFZarr PROPERTIES
    PREFIX ""                      # no lib
    # DO NOT override OUTPUT_NAME, or set it to "gdal_EOPFZarr"
    SUFFIX ".so")
