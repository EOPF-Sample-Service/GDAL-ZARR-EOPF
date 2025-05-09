#pragma once
#include "gdal_priv.h"
#include <string>

namespace EOPF
{
	/* Reads <rootPath>/.zattrs, sets SRS, geotransform, and key/value pairs   */
	void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
}
