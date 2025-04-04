#ifndef EOPF_DATASET_H
#define EOPF_DATASET_H

#include "gdal_priv.h"
#include <string>

enum class EOPFMode {
    SENSOR,      // Native hierarchical structure
    CONVENIENCE  // Simplified structure
};

/**
 * EOPFDataset class represents an EOPF dataset stored in a Zarr-like structure.
 * Inherits from GDALPamDataset to benefit from Persistent Auxiliary Metadata (PAM)
 */

class EOPFDataset final : public GDALDataset
{
public:
    // Registration callbacks
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);

    // Override methods from GDALDataset
    CPLErr GetGeoTransform(double* padfTransform) override;
    const std::string& GetPath() const { return m_osPath; }  // For m_osPath 
    int GetChunkSizeX() const { return chunkSizeX; }
    int GetChunkSizeY() const { return chunkSizeY; }
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
	int chunkSizeX = 256;          // Default chunk size X (to be parsed from metadata)
	int chunkSizeY = 256;          // Default chunk size Y (to be parsed from metadata)
};

#endif /* EOPF_DATASET_H */
