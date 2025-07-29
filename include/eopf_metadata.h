#pragma once

#include <string>

#include "cpl_json.h"
#include "gdal_priv.h"

namespace EOPF
{

// Define the access modes for EOPF data
enum class Mode
{
    NATIVE,   // Original sensor data view structure
    ANALYSIS  // Simplified analysis-ready data structure (default)
};

// Main metadata attachment function
void AttachMetadata(GDALDataset& ds, const std::string& rootPath, Mode mode = Mode::ANALYSIS);

// Subdataset discovery function
void DiscoverSubdatasets(GDALDataset& ds,
                         const std::string& rootPath,
                         const CPLJSONObject& metadata,
                         Mode mode = Mode::ANALYSIS);
}  // namespace EOPF