cmake_minimum_required(VERSION 3.10)
project(EOPFPlugin LANGUAGES CXX)

# Find GDAL dev libraries
find_package(GDAL REQUIRED)

# ---------- Build the plugin ----------
add_library(gdal_EOPF SHARED
    src/EOPFDriver.cpp
    src/EOPFDataset.cpp
    src/EOPFRasterBand.cpp
)

# Link plugin to GDAL
if(TARGET GDAL::GDAL)
    target_link_libraries(gdal_EOPF PRIVATE GDAL::GDAL)
else()
    include_directories(${GDAL_INCLUDE_DIR})
    target_link_libraries(gdal_EOPF PRIVATE ${GDAL_LIBRARY})
endif()

# Name it "gdal_EOPF"
set_target_properties(gdal_EOPF PROPERTIES OUTPUT_NAME "gdal_EOPF")

# ---------- Build the test ----------
enable_testing()

add_executable(test_eopf_driver tests/test_eopf_driver.cpp)
if(TARGET GDAL::GDAL)
    target_link_libraries(test_eopf_driver PRIVATE GDAL::GDAL)
else()
    target_link_libraries(test_eopf_driver PRIVATE ${GDAL_LIBRARY})
endif()

add_test(NAME EOPFRegistrationTest COMMAND test_eopf_driver)
