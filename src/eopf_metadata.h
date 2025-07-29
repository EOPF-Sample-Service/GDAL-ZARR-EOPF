#pragma once
#include <string>

#include "cpl_json.h"
#include "gdal_pam.h"
#include "gdal_priv.h"

namespace EOPF
{
void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
void DiscoverSubdatasets(GDALDataset& ds,
                         const std::string& rootPath,
                         const CPLJSONObject& metadata);
}  // namespace EOPF