#include "gdal_priv.h"
#include "EOPFDataset.h"



extern "C" void GDALRegister_EOPF();

extern "C" void GDALRegister_EOPF()
{
    // Avoid re-registering if the driver already exists
    if (GDALGetDriverByName("EOPF") != nullptr)
        return;

    // Create a new driver
    GDALDriver* poDriver = new GDALDriver();

    // Short name
    poDriver->SetDescription("EOPF");

    // Long name (for display)
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "Earth Observation Processing Framework Demo");

    // Indicate this is a raster driver
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");

    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopf.html");
    poDriver->SetMetadataItem(GDAL_DCAP_MULTIDIM_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

#if GDAL_VERSION_NUM >= 3100
    poDriver->SetMetadataItem(GDAL_DCAP_MULTIDIM, "YES");
#endif

    // Add open options
    char** papszOptions = nullptr;
    papszOptions = CSLAddString(papszOptions, "<Option name='MODE' type='string-select' default='CONVENIENCE'>");
    papszOptions = CSLAddString(papszOptions, "    <Value>SENSOR</Value>");
    papszOptions = CSLAddString(papszOptions, "    <Value>CONVENIENCE</Value>");
    papszOptions = CSLAddString(papszOptions, "</Option>");

    poDriver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, CSLSave(papszOptions));
    CSLDestroy(papszOptions);

    // Setting the open function
    poDriver->pfnOpen = EOPFDataset::Open;
    poDriver->pfnIdentify = EOPFDataset::Identify;
    // Register driver
    GetGDALDriverManager()->RegisterDriver(poDriver);
}
