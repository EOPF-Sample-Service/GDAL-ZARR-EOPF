#ifndef EOPF_RASTERBAND_H
#define EOPF_RASTERBAND_H

#include "gdal_priv.h"
#include <string>

class EOPFRasterBand final : public GDALRasterBand
{
public:
    EOPFRasterBand(GDALDataset* poDS, int nBand, GDALDataType eDataType);

    // Override methods from GDALRasterBand
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;

private:
    std::string m_osVarName;      // Name of the Zarr variable/array
    std::string m_osChunkDir;     // Path to directory containing Zarr chunks
};

#endif /* EOPF_RASTERBAND_H */
