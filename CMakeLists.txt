cmake_minimum_required(VERSION 3.10)
project(EOPFZarr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Add more detailed paths for Windows
set(CMAKE_PREFIX_PATH
    "C:/OSGeo4W"
    "C:/OSGeo4W/apps/gdal"
    "C:/OSGeo4W/lib") 

# Use explicit module search for GDAL
find_package(GDAL REQUIRED)

# Print GDAL info for debugging
message(STATUS "Found GDAL: ${GDAL_FOUND}")
message(STATUS "GDAL Include Dirs: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL Libraries: ${GDAL_LIBRARIES}")

# Add OSGeo4W additional libraries path
if(WIN32)
    link_directories("C:/OSGeo4W/lib")
endif()

add_library(gdal_EOPFZarr MODULE
    src/eopfzarr_driver.cpp
    src/eopfzarr_dataset.cpp
    src/eopf_metadata.cpp
)

# Use correct variables for MODULE mode
target_include_directories(gdal_EOPFZarr PRIVATE ${GDAL_INCLUDE_DIRS})

# On Windows, specify all the libraries explicitly
if(WIN32)
    target_link_libraries(gdal_EOPFZarr 
        PRIVATE ${GDAL_LIBRARIES}
        # Add other potential libraries that might contain CPLDebug
        "C:/OSGeo4W/lib/gdal_i.lib"
    )
else()
    target_link_libraries(gdal_EOPFZarr PRIVATE ${GDAL_LIBRARIES})
endif()

# Output properties
if (WIN32)
    # Produce gdal_EOPFZarr.dll (no "lib" prefix, .dll suffix)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""                       # prevent "lib" prefix
        OUTPUT_NAME "gdal_EOPFZarr"     # keep the gdal_ prefix
        SUFFIX ".dll")
else()
    # produce gdal_EOPFZarr.so (no "lib" prefix – mandatory for plugins)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""                      # no lib
        SUFFIX ".so")
endif()

# Add test to verify driver registration
enable_testing()
if(WIN32)
    add_test(NAME driver_registration 
             COMMAND cmd.exe /C "set GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} && gdalinfo --formats | findstr EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()