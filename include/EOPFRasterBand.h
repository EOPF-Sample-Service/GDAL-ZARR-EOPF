#ifndef EOPF_RASTERBAND_H
#define EOPF_RASTERBAND_H

#include "gdal_priv.h"
#include <string>

class EOPFDataset;

class EOPFRasterBand final : public GDALRasterBand {
public:
    EOPFRasterBand(GDALDataset* poDS, int nBand, GDALDataType eDataType);
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;

private:
    std::string m_osChunkDir;
};

#endif // EOPF_RASTERBAND_H
