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

install(TARGETS gdal_EOPF
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gdalplugins
)

# ========== Build test executables ==========
enable_testing()

# Test 1: skeleton
add_executable(test_eopf_driver
    tests/test_eopf_driver.cpp
)
# Link test against GDAL
if(TARGET GDAL::GDAL)
    target_link_libraries(test_eopf_driver PRIVATE GDAL::GDAL)
else()
    target_link_libraries(test_eopf_driver PRIVATE ${GDAL_LIBRARY})
endif()
# Optional: integrate with CTest (uncomment if desired)

add_test(NAME EOPFRegistrationTest COMMAND test_eopf_driver)

