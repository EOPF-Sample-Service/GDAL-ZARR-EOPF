#include "eopfzarr_registry.h"

#include "eopfzarr_errors.h"
#include "gdal_priv.h"

namespace EOPFZarrRegistry
{
GDALDriver* DriverRegistry::s_driver = nullptr;

bool DriverRegistry::RegisterDriver(int (*pfnIdentify)(GDALOpenInfo*),
                                    GDALDataset* (*pfnOpen)(GDALOpenInfo*) )
{
    // Check if already registered
    if (IsRegistered())
    {
        return true;
    }

    if (!GDAL_CHECK_VERSION("EOPFZARR"))
    {
        return false;
    }

    // Create our own driver without modifying Zarr driver
    GDALDriver* driver = new GDALDriver();
    SetupDriverMetadata(driver);

    // Set function pointers
    driver->pfnIdentify = pfnIdentify;
    driver->pfnOpen = pfnOpen;

    GetGDALDriverManager()->RegisterDriver(driver);
    s_driver = driver;

    return true;
}

void DriverRegistry::DeregisterDriver()
{
    if (s_driver)
    {
        // Get the driver manager - do not delete the driver itself
        GDALDriverManager* poDM = GetGDALDriverManager();
        if (poDM)
        {
            poDM->DeregisterDriver(s_driver);
        }
        s_driver = nullptr;
    }
}

GDALDriver* DriverRegistry::GetDriver()
{
    return s_driver;
}

bool DriverRegistry::IsRegistered()
{
    return (GDALGetDriverByName("EOPFZARR") != nullptr);
}

void DriverRegistry::SetupDriverMetadata(GDALDriver* driver)
{
    driver->SetDescription("EOPFZARR");
    driver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Zarr Wrapper Driver");
    driver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");
    driver->SetMetadataItem(GDAL_DMD_SUBDATASETS, "YES");

    // Add our own EOPF_PROCESS option
    const char* pszOptions =
        "<OpenOptionList>"
        "  <Option name='EOPF_PROCESS' type='boolean' default='NO' description='Enable EOPF "
        "features'>"
        "    <Value>YES</Value>"
        "    <Value>NO</Value>"
        "  </Option>"
        "</OpenOptionList>";
    driver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, pszOptions);
}
}  // namespace EOPFZarrRegistry
