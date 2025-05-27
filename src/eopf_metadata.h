#pragma once
#include "gdal_priv.h"
#include "cpl_json.h"
#include <string>

namespace EOPF {

    enum class Mode {
        NATIVE,
        ANALYSIS
    };

    void AttachMetadata(GDALDataset& ds, const std::string& rootPath, Mode mode = Mode::ANALYSIS);
    void DiscoverSubdatasets(GDALDataset& ds, const std::string& rootPath, const CPLJSONObject& metadata, Mode mode=Mode::ANALYSIS);
}

