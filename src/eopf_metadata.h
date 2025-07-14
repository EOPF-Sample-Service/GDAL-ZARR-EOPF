#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4189)
#pragma warning(disable: 4100)
#pragma warning(disable: 4067)
#endif
#include "gdal_priv.h"
#include "cpl_json.h"
#include <string>
#include "gdal_pam.h"


namespace EOPF {
    void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
    void DiscoverSubdatasets(GDALDataset& ds, const std::string& rootPath, const CPLJSONObject& metadata);
    std::string ConstructMetadataPath(const std::string& base, const std::string& sub);
}

bool IsUrlOrVirtualPath(const std::string& path);