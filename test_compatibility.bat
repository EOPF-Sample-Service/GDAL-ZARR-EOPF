@echo off
echo Testing backward compatibility...
echo.
echo Testing old format (unquoted URL):
gdalinfo --debug on EOPFZARR:/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr
echo.
echo Testing new format (quoted URL with subdataset):
gdalinfo --debug on "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr/measurements/reflectance/r60m/b09\""
