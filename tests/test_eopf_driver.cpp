// tests/test_eopf_driver.cpp
#include <iostream>
#include "gdal_priv.h"
#include "cpl_conv.h"

int main()
{
    // Register all known drivers, including our plugin
    GDALAllRegister();

    // Check if "EOPF" is recognized
    auto* drv = GetGDALDriverManager()->GetDriverByName("EOPF");
    if (!drv) {
        std::cerr << "[EOPFDriver] ERROR: EOPF driver not found.\n";
        return 1;
    }

    // Attempt to open a file "test.eopf"
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen("test.eopf", GA_ReadOnly));
    if (!ds) {
        std::cerr << "[EOPFDriver] ERROR: Could not open test.eopf.\n";
        return 1;
    }

    std::cout << "Opened dataset with size "
        << ds->GetRasterXSize() << " x "
        << ds->GetRasterYSize() << "\n";

    GDALClose(ds);
    std::cout << "[EOPFDriver] SUCCESS: driver registration test passed.\n";
    return 0;
}
