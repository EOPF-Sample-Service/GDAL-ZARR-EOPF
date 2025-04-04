#ifndef EOPF_DATASET_H
#define EOPF_DATASET_H

#include "gdal_pam.h"
#include "cpl_json.h"
#include <vector>
#include <map>
#include <string>

enum class EOPFMode {
    SENSOR,      // Native hierarchical structure
    CONVENIENCE  // Simplified structure
};

class EOPFRasterBand;

class EOPFDataset final : public GDALDataset {
    friend class EOPFRasterBand;

public:
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);

    // Metadata and group access
    const std::string& GetPath() const { return m_osPath; }
    int GetChunkSizeX() const { return m_nChunkX; }
    int GetChunkSizeY() const { return m_nChunkY; }
    bool ParseZarrMetadata(const std::string& osMetadataPath);
    bool LoadGroupStructure(const std::string& osPath);
    std::vector<std::string> GetSubGroups() const;

    // Sentinel-2 specific metadata
    std::string GetSTACVersion() const { return m_osSTACVersion; }
    std::string GetProcessingLevel() const { return m_osProcessingLevel; }

private:
    struct GroupInfo {
        std::string osPath;
        std::map<std::string, std::string> attrs;
        std::vector<std::string> arrays;
        std::vector<GroupInfo> subgroups;
    };

    // Core properties
    std::string m_osPath;         // Path to the EOPF dataset
    int m_nChunkX = 256;
    int m_nChunkY = 256;
    GroupInfo m_oRootGroup;

    // Metadata
    std::string m_osSTACVersion;
    std::string m_osProcessingLevel;
    std::map<std::string, std::map<std::string, std::string>> m_bandMetadata;

    // Helper methods
    void ParseBandMetadata(const CPLJSONObject& oBandRoot);
    void GetSubGroupsRecursive(const GroupInfo& group,
        std::vector<std::string>& output) const;
};

#endif // EOPF_DATASET_H
