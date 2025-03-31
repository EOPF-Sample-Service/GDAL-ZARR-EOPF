// tests/test_zarr_read.cpp
#include <iostream>
#include "gdal_priv.h"

int main()
{
    GDALAllRegister();

    // Try opening a .zarr file => triggers the new logic
    const char* filename = "dummy.zarr";
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(filename, GA_ReadOnly));
    if (!ds) {
        std::cerr << "[ZarrTest] ERROR: Could not open " << filename << "\n";
        return 1;
    }

    // Check size => should be 512x512 if .zarr
    int xSize = ds->GetRasterXSize();
    int ySize = ds->GetRasterYSize();
    std::cout << "[ZarrTest] Dataset size: " << xSize << " x " << ySize << "\n";

    // read a small window => calls chunk logic
    GDALRasterBand* band = ds->GetRasterBand(1);
    if (!band) {
        std::cerr << "[ZarrTest] ERROR: no band found\n";
        GDALClose(ds);
        return 1;
    }

    std::vector<unsigned char> row(xSize);
    CPLErr err = band->RasterIO(GF_Read, 0, 0, xSize, 1, row.data(), xSize, 1, GDT_Byte, 0, 0);
    if (err != CE_None) {
        std::cerr << "[ZarrTest] ERROR: read row 0\n";
        GDALClose(ds);
        return 1;
    }
    // Print first pixel
    std::cout << "[ZarrTest] first pixel: " << (int)row[0] << "\n";

    GDALClose(ds);
    std::cout << "[ZarrTest] SUCCESS: partial Zarr read test.\n";
    return 0;
}
