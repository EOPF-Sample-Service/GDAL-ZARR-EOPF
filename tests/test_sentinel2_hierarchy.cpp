#include "gdal_priv.h"
#include "cpl_string.h"

int main() {
    GDALAllRegister();

    GDALDataset* poDS = (GDALDataset*)GDALOpenEx(
        "EOPF-Zarr:S2B_MSIL1C_20250113.zarr",
        GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
        nullptr, nullptr, nullptr
    );

    // List subdatasets
    char** papszMD = poDS->GetMetadata("SUBDATASETS");
    for (int i = 0; papszMD && papszMD[i]; i++) {
        printf("%s\n", papszMD[i]);
    }

    // Open 10m reflectance group
    GDALDataset* po10m = (GDALDataset*)GDALOpenEx(
        "EOPF-Zarr:S2B_MSIL1C_20250113.zarr/measurements/reflectance/r10m",
        GDAL_OF_READONLY,
        nullptr, nullptr, nullptr
    );

    printf("Bands in 10m group: %d\n", po10m->GetRasterCount());

    GDALClose(poDS);
    GDALClose(po10m);
    return 0;
}
