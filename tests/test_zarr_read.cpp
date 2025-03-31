#include <iostream>
#include <fstream>
#include "gdal_priv.h"
#include "cpl_vsi.h"
#include "cpl_string.h"

// Create a mock Zarr dataset for testing
bool CreateMockZarrDataset(const char* pszPath) {
    // Create directory structure
    if (VSIMkdir(pszPath, 0755) != 0) {
        std::cerr << "Failed to create test directory" << std::endl;
        return false;
    }

    // Create .zarray metadata file (Zarr V2)
    std::string osZarrayPath = CPLFormFilename(pszPath, ".zarray", nullptr);
    VSILFILE* fpZarray = VSIFOpenL(osZarrayPath.c_str(), "wb");
    if (fpZarray == nullptr) {
        std::cerr << "Failed to create mock .zarray file" << std::endl;
        return false;
    }

    // Write content
    std::string jsonContent = R"({
        "chunks": [2, 3],
        "compressor": {"id": "zlib", "level": 1},
        "dtype": "<f4",
        "fill_value": "NaN",
        "filters": null,
        "order": "C",
        "shape": [4, 6],
        "zarr_format": 2
    })";

    VSIFWriteL(jsonContent.c_str(), 1, jsonContent.size(), fpZarray);
    VSIFCloseL(fpZarray);

    // Create a mock data chunk
    std::string osChunkPath = CPLFormFilename(pszPath, "0.0", nullptr);
    VSILFILE* fpChunk = VSIFOpenL(osChunkPath.c_str(), "wb");
    if (fpChunk == nullptr) {
        std::cerr << "Failed to create mock chunk file" << std::endl;
        return false;
    }

    // Write some mock float data (6 values)
    float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    VSIFWriteL(data, sizeof(float), 6, fpChunk);
    VSIFCloseL(fpChunk);

    return true;
}

// Clean up test files
void CleanupMockDataset(const char* pszPath) {
    std::string osZarrayPath = CPLFormFilename(pszPath, ".zarray", nullptr);
    std::string osChunkPath = CPLFormFilename(pszPath, "0.0", nullptr);

    VSIUnlink(osZarrayPath.c_str());
    VSIUnlink(osChunkPath.c_str());
    VSIRmdir(pszPath);
}


int main(int argc, char* argv[]) {
    // Register GDAL drivers
    GDALAllRegister();

    // Check if EOPF driver is registered
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("EOPF");
    if (poDriver == nullptr) {
        std::cerr << "EOPF driver not found!" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS: EOPF driver is registered." << std::endl;
    std::cout << "Short Name: " << poDriver->GetDescription() << std::endl;
    std::cout << "Long  Name: " << poDriver->GetMetadataItem(GDAL_DMD_LONGNAME) << std::endl;

    // Create test directory with mock data
    const char* pszTestDir = "/vsimem/eopf_zarr_test";
    if (!CreateMockZarrDataset(pszTestDir)) {
        std::cerr << "Failed to create mock Zarr dataset" << std::endl;
        return 1;
    }

    // Try to open the dataset
    GDALDataset* poDS = (GDALDataset*)GDALOpen(pszTestDir, GA_ReadOnly);
    if (poDS == nullptr) {
        std::cerr << "Failed to open Zarr dataset" << std::endl;
        CleanupMockDataset(pszTestDir);
        return 1;
    }

    std::cout << "Successfully opened Zarr dataset" << std::endl;
    std::cout << "Driver: " << poDS->GetDriver()->GetDescription() << std::endl;
    std::cout << "Size: " << poDS->GetRasterXSize() << "x"
        << poDS->GetRasterYSize() << std::endl;

    // Test metadata reading
    char** papszMetadata = poDS->GetMetadata();
    if (papszMetadata != nullptr) {
        std::cout << "Dataset metadata:" << std::endl;
        for (int i = 0; papszMetadata[i] != nullptr; i++) {
            std::cout << "  " << papszMetadata[i] << std::endl;
        }
    }

    // Clean up
    GDALClose(poDS);
    CleanupMockDataset(pszTestDir);

    std::cout << "Read support test completed successfully" << std::endl;
    return 0;
}
