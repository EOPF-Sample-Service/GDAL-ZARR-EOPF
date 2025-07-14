#pragma once
#include "gdal_priv.h"
#include "cpl_json.h"
#include <string>
#include "gdal_pam.h"

namespace EOPF {
    void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
    void DiscoverSubdatasets(GDALDataset& ds, const std::string& rootPath, const CPLJSONObject& metadata);
    std::string ConstructMetadataPath(const std::string& basePath, const std::string& metadataFile);
}