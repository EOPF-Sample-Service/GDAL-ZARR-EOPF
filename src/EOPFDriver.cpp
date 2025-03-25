// src/EOPFDriver.cpp
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include "EOPFDataset.h"

extern "C" void GDALRegister_EOPF();

extern "C" void GDALRegister_EOPF()
{
    // Avoid re-registering
    if (GDALGetDriverByName("EOPF") != nullptr)
        return;

    // Create driver
    GDALDriver* poDriver = new GDALDriver();
    poDriver->SetDescription("EOPF");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Driver Skeleton");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");

    // pfnIdentify/pfnOpen reference EOPFDataset
    poDriver->pfnIdentify = EOPFDataset::Identify;
    poDriver->pfnOpen = EOPFDataset::Open;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}
