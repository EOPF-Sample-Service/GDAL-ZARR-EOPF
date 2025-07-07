@echo off
setlocal

set "PLUGIN_DIR=%GDAL_DRIVER_PATH%"
if "%PLUGIN_DIR%"=="" set "PLUGIN_DIR=C:\OSGeo4W\bin\gdal\plugins"
set "PLUGIN_FILE=gdal_EOPFZarr.dll"

echo Installing GDAL EOPF Plugin for Windows...

if not exist "%PLUGIN_DIR%" mkdir "%PLUGIN_DIR%"
copy "windows\%PLUGIN_FILE%" "%PLUGIN_DIR%\"

echo Plugin installed to: %PLUGIN_DIR%\%PLUGIN_FILE%
echo Add this to your environment:
echo set GDAL_DRIVER_PATH=%PLUGIN_DIR%;%%GDAL_DRIVER_PATH%%
