#ifndef EOPF_DATASET_H
#define EOPF_DATASET_H

#include "gdal_priv.h"
#include <string>

enum class EOPFMode {
    SENSOR,      // Native hierarchical structure
    CONVENIENCE  // Simplified structure
};

class EOPFDataset final : public GDALDataset
{
public:
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);

    // Override methods from GDALDataset
    CPLErr GetGeoTransform(double* padfTransform) override;

private:
    // Private constructor - datasets should be created with Open()
    EOPFDataset();

    // Helper methods
    bool Initialize(const char* pszFilename, EOPFMode eMode);
    bool ParseZarrMetadata(const char* pszPath);

    // Dataset properties
    std::string m_osPath;         // Path to the EOPF dataset
    EOPFMode m_eMode;             // Operational mode
    bool m_bIsZarrV3;             // Whether this is a Zarr V3 dataset
};

#endif /* EOPF_DATASET_H */