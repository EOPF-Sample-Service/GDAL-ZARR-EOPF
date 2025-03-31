#ifndef EOPF_DATASET_H
#define EOPF_DATASET_H

#include "gdal_priv.h"
#include "gdal_pam.h"
#include <string>

enum class EOPFMode {
    SENSOR,      // Native hierarchical structure
    CONVENIENCE  // Simplified structure
};

class EOPFDataset final : public GDALPamDataset
{
public:
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);

    // Override methods from GDALDataset
    CPLErr GetGeoTransform(double* padfTransform) override;
     
private:
    // Private constructor - datasets should be created with Open()
    EOPFDataset();
    ~EOPFDataset() override;
    // Helper methods
    bool Initialize(const char* pszFilename, EOPFMode eMode);
    bool ParseZarrMetadata(const char* pszPath);

    // Dataset properties
    std::string m_osPath;         // Path to the EOPF dataset
    EOPFMode m_eMode;             // Operational mode
    bool m_bIsZarrV3;             // Whether this is a Zarr V3 dataset

    // Optionally store whether we recognized a .zarr format
    bool bIsZarr = false;

    // Dummy chunk sizes for partial read logic
    int chunkSizeX = 256;
    int chunkSizeY = 256;

    /**
     * @brief ReadChunk is a placeholder function that simulates reading a chunk
     * of data from (chunkX, chunkY) for a given band, filling pBuffer with some pattern.
     * In a real driver, you'd parse .zarray, handle compression, etc.
     */
    CPLErr ReadChunk(int chunkX, int chunkY, int band, void* pBuffer);
};

#endif /* EOPF_DATASET_H */