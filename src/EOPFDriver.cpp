#include "gdal_priv.h"
#include "EOPFDataset.h"
#include "cpl_string.h"

extern "C" void GDALRegister_EOPF() {
    if (GDALGetDriverByName("EOPF-Zarr") != nullptr)
        return;

    GDALDriver* poDriver = new GDALDriver();

    // Core metadata
    poDriver->SetDescription("EOPF-Zarr");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
        "Earth Observation Processing Framework Zarr Driver");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DCAP_MULTIDIM_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC,
        "https://eopf.esa.int/docs/gdal-driver");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSIONS, "zarr");

    // Open options
    poDriver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST,
        "<OpenOptionList>"
        "  <Option name='MODE' type='string-select' default='CONVENIENCE'>"
        "    <Value>CONVENIENCE</Value>"
        "    <Value>SENSOR</Value>"
        "  </Option>"
        "</OpenOptionList>");

    // Function pointers
    poDriver->pfnOpen = EOPFDataset::Open;
    poDriver->pfnIdentify = EOPFDataset::Identify;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}
