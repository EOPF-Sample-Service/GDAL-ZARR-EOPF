@echo off
setlocal

echo Installing GDAL EOPF Plugin for Windows...
echo.

:: Check if GDAL is available
gdalinfo --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: GDAL not found in PATH. Please ensure GDAL is installed.
    echo.
    echo Install GDAL from: https://trac.osgeo.org/osgeo4w/
    echo.
    pause
    exit /b 1
)

:: Get GDAL version
for /f "tokens=2" %%i in ('gdalinfo --version 2^>nul') do set GDAL_VERSION=%%i
echo Detected GDAL version: %GDAL_VERSION%

:: Check version compatibility
echo.
if "%GDAL_VERSION:~0,4%"=="3.10" (
    echo ✅ GDAL 3.10.x detected - compatible
) else if "%GDAL_VERSION:~0,4%"=="3.11" (
    echo ✅ GDAL 3.11.x detected - compatible  
) else (
    echo ⚠️  GDAL version %GDAL_VERSION% compatibility unknown
    echo This plugin was built for GDAL 3.11.x
    echo Continue anyway? (Y/N)
    set /p continue=
    if /i not "%continue%"=="Y" exit /b 1
)

:: Determine plugin directory
set "PLUGIN_DIR=%GDAL_DRIVER_PATH%"

:: If GDAL_DRIVER_PATH is not set, try to detect it
if "%PLUGIN_DIR%"=="" (
    echo.
    echo Detecting GDAL plugin directory...
    
    :: Try OSGeo4W first (most common)
    if exist "C:\OSGeo4W\apps\gdal\lib\gdalplugins" (
        set "PLUGIN_DIR=C:\OSGeo4W\apps\gdal\lib\gdalplugins"
        echo Found OSGeo4W installation: %PLUGIN_DIR%
    ) else if exist "C:\Program Files\GDAL\gdalplugins" (
        set "PLUGIN_DIR=C:\Program Files\GDAL\gdalplugins"
        echo Found GDAL installation: %PLUGIN_DIR%
    ) else if exist "C:\Program Files (x86)\GDAL\gdalplugins" (
        set "PLUGIN_DIR=C:\Program Files (x86)\GDAL\gdalplugins"
        echo Found GDAL installation: %PLUGIN_DIR%
    ) else (
        echo.
        echo WARNING: Could not auto-detect GDAL plugin directory.
        echo Please enter the path to your GDAL plugins directory:
        echo Common paths:
        echo   C:\OSGeo4W\apps\gdal\lib\gdalplugins
        echo   C:\Program Files\GDAL\gdalplugins
        echo.
        set /p PLUGIN_DIR="Enter path: "
        if "%PLUGIN_DIR%"=="" (
            echo ERROR: No plugin directory specified
            pause
            exit /b 1
        )
    )
)

set "PLUGIN_FILE=gdal_EOPFZarr.dll"

echo.
echo Installing to: %PLUGIN_DIR%

::Try to find the plugin file in different locations
set "PLUGIN_SOURCE="
if exist "windows\%PLUGIN_FILE%" (
    set "PLUGIN_SOURCE=windows\%PLUGIN_FILE%"
    echo Found plugin in release package structure
) else if exist "build\Release\%PLUGIN_FILE%" (
    set "PLUGIN_SOURCE=build\Release\%PLUGIN_FILE%"
    echo Found plugin in local build directory
) else if exist "build_github_actions\dll_github\%PLUGIN_FILE%" (
    set "PLUGIN_SOURCE=build_github_actions\dll_github\%PLUGIN_FILE%"
    echo Found plugin in GitHub Actions artifacts directory
) else (
    echo ERROR: Plugin file not found in any of these locations:
    echo   - windows\%PLUGIN_FILE% (release package)
    echo   - build\Release\%PLUGIN_FILE% (local build)
    echo   - build_github_actions\dll_github\%PLUGIN_FILE% (GitHub artifacts)
    echo.
    echo Please ensure you have either:
    echo   1. Extracted a release package, or
    echo   2. Built the plugin locally, or  
    echo   3. Downloaded GitHub Actions artifacts
    pause
    exit /b 1
)

if not exist "%PLUGIN_DIR%" (
    echo Creating plugin directory: %PLUGIN_DIR%
    mkdir "%PLUGIN_DIR%"
)

copy "%PLUGIN_SOURCE%" "%PLUGIN_DIR%\"

if errorlevel 1 (
    echo ERROR: Failed to copy plugin file
    echo This may be due to permission issues. Try running as Administrator.
    pause
    exit /b 1
)

echo.
echo ✅ Plugin installed successfully!
echo.
echo Testing installation...
gdalinfo --formats | findstr EOPF >nul 2>&1
if errorlevel 1 (
    echo ⚠️  Plugin installed but not detected by GDAL
    echo This may be normal on some systems
    echo.
    echo To manually set the plugin path, add this to your environment:
    echo   set GDAL_DRIVER_PATH=%PLUGIN_DIR%;%%GDAL_DRIVER_PATH%%
) else (
    echo ✅ Plugin detected by GDAL successfully!
)

echo.
echo Installation complete. Test with:
echo   gdalinfo --formats ^| findstr EOPF
echo.
pause
