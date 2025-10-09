#!/usr/bin/env 
# GDAL EOPF-Zarr Plugin - Code Formatting Script
# Usage: .\format-code.ps1 [file1] [file2] ... or .\format-code.ps1 (for all source files)
  
param(
[string[]]$Files = @()
)

# Find clang-format executable
$clangFormatPaths = @(
"clang-format",
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe",
"C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-format.exe"
)

$clangFormat = $null
foreach ($path in $clangFormatPaths) {
try {
$result = Get-Command $path -ErrorAction Stop
$clangFormat = $result.Source
break
}
catch {
continue
}
}
  
if (-not $clangFormat) {
Write-Error "clang-format not found. Please install it or add it to your PATH."
exit 1
}

Write-Host "Using clang-format: $clangFormat" -ForegroundColor Green
# If no files specified, format all source files
if ($Files.Count -eq 0) {
$Files = Get-ChildItem -Path src -Include "*.cpp", "*.h", "*.hpp" -Recurse | ForEach-Object { $_.FullName }
Write-Host "Formatting all source files..." -ForegroundColor Yellow
}

# Format each file
foreach ($file in $Files) {
if (Test-Path $file) {
Write-Host "Formatting: $file" -ForegroundColor Cyan
& $clangFormat -i $file
if ($LASTEXITCODE -eq 0) {
Write-Host " ✓ Formatted successfully" -ForegroundColor Green
} else {
Write-Host " ✗ Formatting failed" -ForegroundColor Red
}
} else {
Write-Host "File not found: $file" -ForegroundColor Red
}
}

Write-Host "Code formatting complete!" -ForegroundColor Green