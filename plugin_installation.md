Below is a practical, step-by-step recipe that works on a “normal” Ubuntu system where QGIS has already been installed from the official repository.  
It shows how to build your own GDAL driver (plugin) and make it visible to both the system GDAL and to QGIS.

----------------------------------------------------------------
1.  Make sure you have the development packages QGIS is using
----------------------------------------------------------------
```bash
sudo apt update
sudo apt install build-essential cmake git \
                 libgdal-dev gdal-bin qgis-dev-tools
```
`libgdal-dev` brings the headers and the `gdal-config` utility.  
`qgis-dev-tools` contains the QGIS C++ headers in case you need them later.

----------------------------------------------------------------
2.  Decide where to install the plugin
----------------------------------------------------------------
GDAL looks for plugins in the directory returned by  
`gdal-config --plugindir`.  
Typical values:

- Debian/Ubuntu system GDAL: `/usr/lib/gdalplugins/3.X.Y`  
- QGIS flatpak or snap: another path, but the procedure is identical—just point to that path instead.

Create a writable directory for your plugin (example for system GDAL):

```bash
GDAL_PLUG=$(gdal-config --plugindir)
sudo mkdir -p "$GDAL_PLUG"
```

----------------------------------------------------------------
3.  Build the plugin
----------------------------------------------------------------
Suppose your driver is in a directory called `my_gdal_driver`.

```bash
cd ~/src/my_gdal_driver
mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DGDAL_INCLUDE_DIR=$(gdal-config --cflags | sed 's/-I//') \
  -DGDAL_LIBRARY=$(gdal-config --libs | awk '{print $1}')

make -j$(nproc)
sudo make install
```

The last step should copy `libgdal_MyDriver.so` to `/usr/local/lib/gdalplugins/`.
Link (or copy) it into the GDAL plugin directory so QGIS picks it up as well:

```bash
sudo ln -s /usr/local/lib/gdalplugins/libgdal_MyDriver.so \
           "$GDAL_PLUG"/
```

----------------------------------------------------------------
4.  Tell GDAL where to find the plugin (only if you used a non-standard dir)
----------------------------------------------------------------
If you installed into `/usr/local/lib/gdalplugins`, add it to the environment:

```bash
echo 'export GDAL_DRIVER_PATH=/usr/local/lib/gdalplugins:$GDAL_DRIVER_PATH' \
     >> ~/.bashrc
source ~/.bashrc
```

----------------------------------------------------------------
5.  Test outside QGIS first
----------------------------------------------------------------
```bash
# Should list your new driver
gdalinfo --formats | grep -i mydriver
# Try opening a dataset
gdalinfo /path/to/test.mydriver
```

----------------------------------------------------------------
6.  Run QGIS and enjoy
----------------------------------------------------------------
Start QGIS from the same shell (so the environment variables are inherited) or log out/in again.  
`Settings ► Options ► System ► Environment` can also be used to set `GDAL_DRIVER_PATH` permanently inside QGIS.

Your custom driver should now appear in  
`Processing ► Toolbox ► GDAL ► Vector/Raster ► MyDriver …` or wherever your provider registers itself.

----------------------------------------------------------------
Troubleshooting checklist
----------------------------------------------------------------
- The plugin must be compiled against **exactly the same GDAL version** that QGIS uses (`gdalinfo --version`).  
- If you installed QGIS from the `ubuntugis/ppa`, make sure you added the same PPA before installing `libgdal-dev`.  
- Use `ldd libgdal_MyDriver.so | grep "not found"` to spot missing shared libraries.  
- Inside QGIS you can open the Python console and run  
  `from osgeo import gdal; print(gdal.GetDriverByName('MyDriver'))`  
  to verify the driver is loaded.

That’s all—your custom GDAL plugin is now available to the system GDAL and to QGIS alike.