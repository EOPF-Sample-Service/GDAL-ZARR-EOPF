cmake_minimum_required(VERSION 3.10)
project(GDAL_Zarr_EOPF LANGUAGES CXX)

# Ensure GDAL dev files are installed (libgdal-dev, etc.)
find_package(GDAL REQUIRED)

# ========== Build the plugin library ==========
add_library(gdal_EOPF SHARED
    src/EOPFDriver.cpp
    src/EOPFDataset.cpp
    src/EOPFRasterBand.cpp
)
# Link to GDAL
if(TARGET GDAL::GDAL)
    target_link_libraries(gdal_EOPF PRIVATE GDAL::GDAL)
else()
    include_directories(${GDAL_INCLUDE_DIR})
    target_link_libraries(gdal_EOPF PRIVATE ${GDAL_LIBRARY})
endif()
# Name the output
set_target_properties(gdal_EOPF PROPERTIES OUTPUT_NAME "gdal_EOPF")


# ========== Build test executables ==========
enable_testing()

# Test 1: skeleton
add_executable(test_skeleton tests/test_skeleton.cpp)
if(TARGET GDAL::GDAL)
    target_link_libraries(test_skeleton PRIVATE GDAL::GDAL)
else()
    target_link_libraries(test_skeleton PRIVATE ${GDAL_LIBRARY})
endif()
add_test(NAME SkeletonTest COMMAND test_skeleton)

# Test 2: Zarr read
add_executable(test_zarr_read tests/test_zarr_read.cpp)
if(TARGET GDAL::GDAL)
    target_link_libraries(test_zarr_read PRIVATE GDAL::GDAL)
else()
    target_link_libraries(test_zarr_read PRIVATE ${GDAL_LIBRARY})
endif()
add_test(NAME ZarrReadTest COMMAND test_zarr_read)
