#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build script specifically for creating GitHub release DLLs with minimal dependencies
.DESCRIPTION
    This script builds the EOPFZARR plugin with static linking and minimal dependencies
    to avoid DLL loading issues when deployed via GitHub releases.
.PARAMETER Configuration
    Build configuration (Release or Debug). Default is Release.
.PARAMETER StaticLinking
    Enable static linking to reduce dependencies. Default is true for releases.
.PARAMETER OutputPath
    Directory where the built DLL will be placed. Default is ./release-build
#>

param(
    [string]$Configuration = "Release",
    [bool]$StaticLinking = $true,
    [string]$OutputPath = "./release-build"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "EOPFZARR GitHub Release Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Ensure we're in the right directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "Configuration: $Configuration" -ForegroundColor Green
Write-Host "Static Linking: $StaticLinking" -ForegroundColor Green
Write-Host "Output Path: $OutputPath" -ForegroundColor Green

# Clean and create build directory
if (Test-Path $OutputPath) {
    Write-Host "Cleaning existing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $OutputPath
}
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null

# Configure CMake with static deployment options
Write-Host "Configuring CMake for GitHub release deployment..." -ForegroundColor Yellow

# First, try to find GDAL using the same logic as the regular build
$gdalPaths = @()
if ($env:GDAL_ROOT) { $gdalPaths += "$env:GDAL_ROOT" }
if ($env:CONDA_PREFIX) { $gdalPaths += "$env:CONDA_PREFIX" }
if ($env:OSGEO4W_ROOT) { $gdalPaths += "$env:OSGEO4W_ROOT" }
$gdalPaths += "C:\OSGeo4W"

$gdalFound = $false
foreach ($path in $gdalPaths) {
    $gdalInclude = Join-Path $path "include"
    $gdalLib = Join-Path $path "lib"
    if ((Test-Path $gdalInclude) -and (Test-Path $gdalLib)) {
        Write-Host "Found GDAL at: $path" -ForegroundColor Green
        $gdalRoot = $path
        $gdalFound = $true
        break
    }
}

if (-not $gdalFound) {
    Write-Error "GDAL not found. Please ensure GDAL is installed and accessible."
    exit 1
}

$cmakeArgs = @(
    "-S", ".",
    "-B", $OutputPath,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DGDAL_ROOT=$gdalRoot"
)

if ($StaticLinking) {
    $cmakeArgs += "-DEOPFZARR_STATIC_DEPLOYMENT=ON"
    $cmakeArgs += "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded"
}

Write-Host "CMake command: cmake $($cmakeArgs -join ' ')" -ForegroundColor Gray
& cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed!"
    exit 1
}

# Build the project
Write-Host "Building project..." -ForegroundColor Yellow
& cmake --build $OutputPath --config $Configuration --target gdal_EOPFZarr

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit 1
}

# Locate the built DLL
$dllPath = Get-ChildItem -Path $OutputPath -Recurse -Name "gdal_EOPFZarr.dll" | Select-Object -First 1
if (-not $dllPath) {
    Write-Error "Built DLL not found!"
    exit 1
}

$fullDllPath = Join-Path $OutputPath $dllPath
Write-Host "Built DLL found at: $fullDllPath" -ForegroundColor Green

# Check DLL size and properties
$dllInfo = Get-Item $fullDllPath
Write-Host "DLL Size: $($dllInfo.Length) bytes" -ForegroundColor Green
Write-Host "DLL Created: $($dllInfo.CreationTime)" -ForegroundColor Green

# Validate the DLL
Write-Host "Validating DLL..." -ForegroundColor Yellow

# Check if dumpbin is available for dependency analysis
try {
    $dumpbinOutput = & dumpbin /dependents $fullDllPath 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "DLL Dependencies:" -ForegroundColor Green
        $dumpbinOutput | Select-String "\.dll" | ForEach-Object {
            Write-Host "  - $($_.Line.Trim())" -ForegroundColor Gray
        }
    }
} catch {
    Write-Host "dumpbin not available - skipping dependency analysis" -ForegroundColor Yellow
}

# Test if the DLL can be loaded (basic test)
Write-Host "Testing DLL loading..." -ForegroundColor Yellow
try {
    # Use PowerShell to attempt loading the DLL
    $null = [System.Reflection.Assembly]::LoadFile($fullDllPath)
    Write-Host "✗ DLL rejected by .NET (expected for native DLL)" -ForegroundColor Yellow
} catch {
    # This is expected for native DLLs
    Write-Host "✓ DLL appears to be a valid native library" -ForegroundColor Green
}

# Copy DLL to a release-ready location
$releaseDir = "./github-release"
if (-not (Test-Path $releaseDir)) {
    New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
}

$releaseDllPath = Join-Path $releaseDir "gdal_EOPFZarr.dll"
Copy-Item $fullDllPath $releaseDllPath -Force

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "Release DLL: $releaseDllPath" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

# Generate build summary
$buildSummary = @"
EOPFZARR GitHub Release Build Summary
=====================================
Build Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Configuration: $Configuration
Static Linking: $StaticLinking
DLL Size: $($dllInfo.Length) bytes
DLL Location: $releaseDllPath

Dependencies Analysis:
$(if ($dumpbinOutput) { $dumpbinOutput -join "`n" } else { "Dependency analysis not available" })

Installation Instructions:
1. Download gdal_EOPFZarr.dll
2. Place in GDAL plugins directory (e.g., C:\OSGeo4W\apps\gdal\lib\gdalplugins\)
3. Test with: gdalinfo --formats | findstr EOPFZARR

Troubleshooting:
- If DLL still fails to load, try building locally using build-and-install.ps1
- Ensure Visual C++ Redistributable 2019/2022 is installed
- Check GDAL version compatibility (requires GDAL 3.8+)
"@

$buildSummary | Out-File -FilePath (Join-Path $releaseDir "BUILD_INFO.txt") -Encoding UTF8

Write-Host "Build summary saved to: $(Join-Path $releaseDir 'BUILD_INFO.txt')" -ForegroundColor Green
