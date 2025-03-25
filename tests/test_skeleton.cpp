// tests/test_skeleton.cpp
#include <iostream>
#include "gdal_priv.h"
#include "cpl_conv.h"

int main()
{
    // Register all known drivers, including our plugin if found
    GDALAllRegister();

    // Check EOPF driver by name
    auto* poDriver = GetGDALDriverManager()->GetDriverByName("EOPF");
    if (!poDriver) {
        std::cerr << "[Skeleton] ERROR: EOPF driver not found.\n";
        return 1;
    }

    // Attempt to open a file with extension .eopf => triggers Identify/Open
    GDALDataset* poDS = static_cast<GDALDataset*>(
        GDALOpen("dummy.eopf", GA_ReadOnly)
        );
    if (!poDS) {
        std::cerr << "[Skeleton] ERROR: Could not open dummy.eopf.\n";
        return 1;
    }

    // Confirm dataset size
    std::cout << "Raster Size: "
        << poDS->GetRasterXSize() << " x "
        << poDS->GetRasterYSize() << "\n";

    GDALClose(poDS);

    std::cout << "[Skeleton] SUCCESS: The EOPF skeleton driver test passed.\n";
    return 0;
}
