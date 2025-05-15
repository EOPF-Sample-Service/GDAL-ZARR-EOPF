#pragma once
#include "gdal_priv.h"
#include <string>

namespace EOPF {
    void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
    void DiscoverSubdatasets(GDALDataset& ds, const std::string& rootPath, const CPLJSONObject& metadata);
}

