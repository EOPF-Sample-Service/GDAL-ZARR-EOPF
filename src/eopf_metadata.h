#pragma once
#include "gdal_priv.h"
#include <string>
#include <mutex>

namespace EOPF {
    // Thread-safe metadata attachment for EOPF datasets
    void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
    
    // Helper function to discover subdatasets
    void DiscoverSubdatasets(GDALDataset& ds, const std::string& rootPath, const CPLJSONObject& metadata);
}