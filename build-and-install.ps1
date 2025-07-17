# Set error action preference to stop on errors
$ErrorActionPreference = "Stop"

# Navigate to project directory
Set-Location $PSScriptRoot

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Green
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/OSGeo4W;C:/OSGeo4W/apps/gdal"

# Build the project
Write-Host "Building project..." -ForegroundColor Green
cmake --build build --config Release

# Copy the DLL to the GDAL plugins directory
Write-Host "Copying gdal_EOPFZarr.dll to GDAL plugins directory..." -ForegroundColor Green

# Check if the DLL was built successfully
if (-not (Test-Path "build\Release\gdal_EOPFZarr.dll")) {
    Write-Error "gdal_EOPFZarr.dll was not found in build\Release\ - build may have failed"
    exit 1
}

# Ensure the plugins directory exists
$pluginDir = "C:\OSGeo4W\apps\gdal\lib\gdalplugins"
if (-not (Test-Path $pluginDir)) {
    Write-Warning "Plugins directory $pluginDir does not exist - creating it"
    New-Item -ItemType Directory -Path $pluginDir -Force
}

Copy-Item -Path "build\Release\gdal_EOPFZarr.dll" -Destination $pluginDir -Force

Write-Host "Build, installation, and file copy completed successfully!" -ForegroundColor Green
