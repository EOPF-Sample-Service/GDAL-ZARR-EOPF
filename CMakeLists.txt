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

# If GDAL imported target is not available, try with pkg-config as fallback
if(NOT TARGET GDAL::GDAL)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GDAL REQUIRED gdal)
        if(GDAL_FOUND)
            add_library(GDAL::GDAL INTERFACE IMPORTED)
            target_include_directories(GDAL::GDAL INTERFACE ${GDAL_INCLUDE_DIRS})
            target_link_libraries(GDAL::GDAL INTERFACE ${GDAL_LIBRARIES})
            target_compile_options(GDAL::GDAL INTERFACE ${GDAL_CFLAGS_OTHER})
        endif()
    endif()
endif()

# Verify GDAL target is available
if(NOT TARGET GDAL::GDAL)
    message(FATAL_ERROR "GDAL not found. Please install GDAL development packages.")
endif()

# Print GDAL information for debugging
message(STATUS "Found GDAL version: ${GDAL_VERSION}")
if(GDAL_INCLUDE_DIRS)
    message(STATUS "GDAL include directories: ${GDAL_INCLUDE_DIRS}")
endif()
if(GDAL_LIBRARIES)  
    message(STATUS "GDAL libraries: ${GDAL_LIBRARIES}")
endif()

# Windows-specific configuration
if(WIN32)
    # Set MSVC runtime library to be compatible with GDAL
    if(MSVC)
        # Use MultiThreaded DLL runtime to match typical GDAL builds
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        
        # Add Windows-specific compile definitions
        add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
        
        # Ensure we're building for the correct architecture
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(CMAKE_GENERATOR_PLATFORM x64)
        endif()
    endif()
    
    # Add Windows-specific libraries that GDAL might need
    set(WIN32_LIBRARIES ws2_32 kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid comdlg32 advapi32)
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

# Windows-specific linking
if(WIN32)
    target_link_libraries(gdal_EOPFZarr PRIVATE ${WIN32_LIBRARIES})
    
    # Add Windows-specific compile definitions for the target
    target_compile_definitions(gdal_EOPFZarr PRIVATE 
        -DEOPF_ZARR_BUILDING=1
        -D_CRT_SECURE_NO_WARNINGS
    )
endif()

# Output properties for plugin
if (WIN32)
    set_target_properties(gdal_EOPFZarr PROPERTIES
        PREFIX ""
        OUTPUT_NAME "gdal_EOPFZarr"
        SUFFIX ".dll")

    # Improved .def file generation with better exports
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/eopfzarr_exports.def")
    file(WRITE ${DEF_FILE} "EXPORTS\n")
    file(APPEND ${DEF_FILE} "    GDALRegisterMe\n")
    file(APPEND ${DEF_FILE} "    GDALRegister_EOPFZarr\n")
    file(APPEND ${DEF_FILE} "    GDALDeregisterEOPFZarr\n")

    # Add the .def file to the library
    target_link_options(gdal_EOPFZarr PRIVATE "/DEF:${DEF_FILE}")
    
    # Ensure proper module definition for Windows DLL
    set_target_properties(gdal_EOPFZarr PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF  # Use .def file instead
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
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
    # Improved OSGeo4W and Windows GDAL path detection
    set(GDAL_PLUGINS_DIR "$ENV{GDAL_DRIVER_PATH}" CACHE PATH "Directory where GDAL plugins are installed")
    
    if(NOT GDAL_PLUGINS_DIR)
        # Try common OSGeo4W locations
        if(EXISTS "C:/OSGeo4W64/bin/gdal/plugins")
            set(GDAL_PLUGINS_DIR "C:/OSGeo4W64/bin/gdal/plugins")
        elseif(EXISTS "C:/OSGeo4W/bin/gdal/plugins")
            set(GDAL_PLUGINS_DIR "C:/OSGeo4W/bin/gdal/plugins")
        elseif(EXISTS "C:/OSGeo4W64/apps/gdal/lib/gdalplugins")
            set(GDAL_PLUGINS_DIR "C:/OSGeo4W64/apps/gdal/lib/gdalplugins")
        elseif(EXISTS "C:/OSGeo4W/apps/gdal/lib/gdalplugins")
            set(GDAL_PLUGINS_DIR "C:/OSGeo4W/apps/gdal/lib/gdalplugins")
        else()
            # Fallback to environment variables or default
            set(GDAL_PLUGINS_DIR "$ENV{PROGRAMDATA}/GDAL/plugins")
        endif()
    endif()
    
    message(STATUS "Windows GDAL plugins directory: ${GDAL_PLUGINS_DIR}")
else()
    set(GDAL_PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/lib/gdalplugins" CACHE PATH "Directory where GDAL plugins are installed")
endif()

install(TARGETS gdal_EOPFZarr 
    LIBRARY DESTINATION ${GDAL_PLUGINS_DIR}
    RUNTIME DESTINATION ${GDAL_PLUGINS_DIR}  # For Windows DLLs
)

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
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
else()
    add_test(NAME driver_registration 
             COMMAND bash -c "GDAL_DRIVER_PATH=${CMAKE_BINARY_DIR} gdalinfo --formats | grep -q EOPFZARR")
endif()