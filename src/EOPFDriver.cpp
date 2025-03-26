// src/EOPFDriver.cpp
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include "EOPFDataset.h"

extern "C" void GDALRegister_EOPF();

// This function is found by the GDAL plugin loader
extern "C" void GDALRegister_EOPF()
{
    if (GDALGetDriverByName("EOPF") != nullptr)
        return; // Already registered

    GDALDriver* poDriver = new GDALDriver();
    poDriver->SetDescription("EOPF"); // short name
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Plugin (Skeleton)");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");

    // We reference EOPFDataset's static methods
    poDriver->pfnIdentify = EOPFDataset::Identify;
    poDriver->pfnOpen = EOPFDataset::Open;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}
