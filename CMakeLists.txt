cmake_minimum_required(VERSION 3.10)
project(EOPFZarr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Use MODULE mode instead of CONFIG mode
find_package(GDAL REQUIRED)

add_library(gdal_EOPFZarr MODULE
    src/eopfzarr_driver.cpp
    src/eopfzarr_dataset.cpp
    src/eopf_metadata.cpp
)

# Use correct variables for MODULE mode
target_include_directories(gdal_EOPFZarr PRIVATE ${GDAL_INCLUDE_DIRS})
target_link_libraries(gdal_EOPFZarr PRIVATE ${GDAL_LIBRARIES})

# produce gdal_EOPFZarr.so (no "lib" prefix – mandatory for plugins)
set_target_properties(gdal_EOPFZarr PROPERTIES
    PREFIX ""                      # no lib
    # DO NOT override OUTPUT_NAME, or set it to "gdal_EOPFZarr"
    SUFFIX ".so")
