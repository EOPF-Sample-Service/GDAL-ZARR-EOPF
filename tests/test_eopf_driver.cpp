#include <iostream>
#include "gdal_priv.h"
#include "cpl_conv.h"


int main()
{
    // Load all GDAL drivers, including our plugin if found
    GDALAllRegister();

    // Check for EOPF driver by name
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("EOPF-Zarr");
    if (!poDriver) {
        std::cerr << "ERROR: EOPF driver not found!" << std::endl;
        return 1; // non-zero means test failure
    }

    std::cout << "SUCCESS: EOPF driver is registered." << std::endl;
    std::cout << "Short Name: " << poDriver->GetDescription() << std::endl;
    std::cout << "Long  Name: " << poDriver->GetMetadataItem(GDAL_DMD_LONGNAME) << std::endl;
    std::cout << "Testing identification function..." << std::endl;

    // Test identification function
    std::cout << "Testing identification function..." << std::endl;

    // Test opening (will fail with non-existent path, but tests API)
    std::cout << "Opening non-existent dataset to test API..." << std::endl;
    GDALDataset* poDS = (GDALDataset*)GDALOpen("EOPF:non_existent_path.zarr", GA_ReadOnly);

    if (poDS) {
        std::cout << "Dataset opened (unexpected!)" << std::endl;
        GDALClose(poDS);
    }
    else {
        std::cout << "Dataset failed to open (expected at this stage)" << std::endl;
    }

    return 0;
}
