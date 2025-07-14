cmake_minimum_required(VERSION 3.16)
project(GDAL_EOPF_Zarr)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/eopfzarr_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/eopfzarr_config.h"
)

# Find GDAL
find_package(GDAL REQUIRED)

# Enhanced GDAL version detection and compatibility
if(GDAL_VERSION)
    message(STATUS "Found GDAL version: ${GDAL_VERSION}")
    
    # Create proper version number for preprocessor defines
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" GDAL_VERSION_MATCH "${GDAL_VERSION}")
    if(GDAL_VERSION_MATCH)
        set(GDAL_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(GDAL_VERSION_MINOR ${CMAKE_MATCH_2})
        set(GDAL_VERSION_PATCH ${CMAKE_MATCH_3})
        
        # Calculate version number (e.g., 3.11.0 -> 3110000)
        math(EXPR GDAL_VERSION_NUM "${GDAL_VERSION_MAJOR} * 1000000 + ${GDAL_VERSION_MINOR} * 10000 + ${GDAL_VERSION_PATCH} * 100")
        
        message(STATUS "GDAL version number: ${GDAL_VERSION_NUM}")
        add_definitions(-DGDAL_VERSION_NUM=${GDAL_VERSION_NUM})
    endif()
    
    # Compatibility warnings
    if(GDAL_VERSION VERSION_LESS "3.4")
        message(WARNING "GDAL version ${GDAL_VERSION} may have limited Zarr support. Recommended: 3.8+")
    elseif(GDAL_VERSION VERSION_LESS "3.8")
        message(WARNING "GDAL version ${GDAL_VERSION} has basic Zarr support. Recommended: 3.8+ for best performance")
    else()
        message(STATUS "✅ GDAL version ${GDAL_VERSION} - excellent Zarr support")
    endif()
else()
    message(FATAL_ERROR "Could not determine GDAL version. Please check your GDAL installation.")
endif()

# Feature detection for XMLInit signature
if(GDAL_VERSION VERSION_GREATER_EQUAL "3.4.0")
    add_definitions(-DGDAL_HAS_CONST_XML_NODE=1)
    message(STATUS "Using const XMLInit signature for GDAL ${GDAL_VERSION}")
else()
    message(STATUS "Using non-const XMLInit signature for GDAL ${GDAL_VERSION}")
endif()

# Feature detection for RefUnderlyingRasterBand signature
if(GDAL_VERSION VERSION_GREATER_EQUAL "3.5.0")
    add_definitions(-DGDAL_HAS_CONST_REF_UNDERLYING=1)
    message(STATUS "Using const RefUnderlyingRasterBand signature for GDAL ${GDAL_VERSION}")
else()
    message(STATUS "Using non-const RefUnderlyingRasterBand signature for GDAL ${GDAL_VERSION}")
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    message(STATUS "Debug build enabled")
else()
    add_definitions(-DNDEBUG)
    message(STATUS "Release build enabled")
endif()

# Source files
set(SOURCES
    src/eopfzarr_driver.cpp
    src/eopfzarr_dataset.cpp
    src/eopf_metadata.cpp
)

# Header files
set(HEADERS
    src/eopfzarr_dataset.h
    src/eopf_metadata.h
)

# Create the plugin library
add_library(gdal_EOPFZarr SHARED ${SOURCES} ${HEADERS})

# Set library properties
set_target_properties(gdal_EOPFZarr PROPERTIES
    PREFIX ""  # Remove lib prefix on Unix systems
    SUFFIX ".so"  # Ensure .so extension on Unix
)

# Platform-specific library naming
if(WIN32)
    set_target_properties(gdal_EOPFZarr PROPERTIES SUFFIX ".dll")
elseif(APPLE)
    set_target_properties(gdal_EOPFZarr PROPERTIES SUFFIX ".dylib")
endif()

target_include_directories(gdal_EOPFZarr PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Include directories
target_include_directories(gdal_EOPFZarr PRIVATE
    ${GDAL_INCLUDE_DIRS}
    src/
)

# Link libraries
target_link_libraries(gdal_EOPFZarr PRIVATE
    ${GDAL_LIBRARIES}
)

# Compiler definitions
target_compile_definitions(gdal_EOPFZarr PRIVATE
    GDAL_COMPILATION
)

# Find GDAL installation directory dynamically
find_program(GDAL_CONFIG gdal-config)
if(GDAL_CONFIG)
    execute_process(COMMAND ${GDAL_CONFIG} --prefix
                   OUTPUT_VARIABLE GDAL_PREFIX
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GDAL_PLUGINS_DIR "${GDAL_PREFIX}/lib/gdalplugins")
    message(STATUS "GDAL plugins directory: ${GDAL_PLUGINS_DIR}")
else()
    # Fallback for Homebrew installation on Mac
    if(APPLE)
        set(GDAL_PLUGINS_DIR "/opt/homebrew/lib/gdalplugins")
    elseif(WIN32)
        set(GDAL_PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/lib/gdalplugins")
    else()
        set(GDAL_PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/lib/gdalplugins")
    endif()
    message(WARNING "gdal-config not found, using fallback path: ${GDAL_PLUGINS_DIR}")
endif()

# Install target with dynamic path and proper permissions
install(TARGETS gdal_EOPFZarr
    LIBRARY DESTINATION ${GDAL_PLUGINS_DIR}
    RUNTIME DESTINATION ${GDAL_PLUGINS_DIR}  # For Windows DLLs
    COMPONENT Runtime
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Add custom target for plugin-only installation during development
add_custom_target(install-plugin
    COMMAND ${CMAKE_COMMAND} --install . --component Runtime
    DEPENDS gdal_EOPFZarr
    COMMENT "Installing EOPF Zarr plugin to GDAL plugins directory"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add custom target for build and install in one step
add_custom_target(build-install
    COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG> --target gdal_EOPFZarr
    COMMAND ${CMAKE_COMMAND} --install . --component Runtime
    COMMENT "Building and installing EOPF Zarr plugin"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add custom target for clean build and install
add_custom_target(rebuild-install
    COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG> --target clean
    COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG> --target gdal_EOPFZarr
    COMMAND ${CMAKE_COMMAND} --install . --component Runtime
    COMMENT "Clean building and installing EOPF Zarr plugin"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print configuration summary
message(STATUS "")
message(STATUS "=== EOPF Zarr Plugin Configuration Summary ===")
message(STATUS "GDAL Version: ${GDAL_VERSION}")
message(STATUS "GDAL Version Number: ${GDAL_VERSION_NUM}")
message(STATUS "GDAL Include Dirs: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL Libraries: ${GDAL_LIBRARIES}")
message(STATUS "Plugin Install Directory: ${GDAL_PLUGINS_DIR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
if(GDAL_VERSION VERSION_GREATER_EQUAL "3.4.0")
    message(STATUS "XMLInit Signature: const CPLXMLNode*")
else()
    message(STATUS "XMLInit Signature: CPLXMLNode*")
endif()
if(GDAL_VERSION VERSION_GREATER_EQUAL "3.5.0")
    message(STATUS "RefUnderlyingRasterBand: const version")
else()
    message(STATUS "RefUnderlyingRasterBand: non-const version")
endif()
message(STATUS "===============================================")
message(STATUS "")

# Usage instructions
message(STATUS "Usage:")
message(STATUS "  Build only:           cmake --build . --target gdal_EOPFZarr")
message(STATUS "  Build and install:    cmake --build . --target build-install")
message(STATUS "  Install only:         cmake --build . --target install-plugin")
message(STATUS "  Clean rebuild:        cmake --build . --target rebuild-install")
message(STATUS "")
