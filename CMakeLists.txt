cmake_minimum_required(VERSION 3.10)
project(eopf-gdal-driver VERSION 0.1.0 LANGUAGES CXX)


#  Find GDAL Package
find_package(GDAL REQUIRED)

# Add the GDAL include directory to the include directories
include_directories(${GDAL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set up the shared library
add_library(eopfdriver SHARED
  src/EOPFDriver.cpp
  src/EOPFDataset.cpp
  src/EOPFRasterBand.cpp
 "include/EOPFDataset.h")

if(TARGET GDAL::GDAL)
  target_link_libraries(eopfdriver PRIVATE GDAL::GDAL)
else()
  include_directories(${GDAL_INCLUDE_DIR})
  target_link_libraries(eopfdriver PRIVATE ${GDAL_LIBRARY})
endif()



# Set the output name with gdal_ prefix for GDAL plugin detection
set_target_properties(eopfdriver PROPERTIES PREFIX "gdal_" OUTPUT_NAME "EOPF")


# Installation settings for the driver
install(TARGETS eopfdriver
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gdalplugins
)
enable_testing()
# Create a test executable
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