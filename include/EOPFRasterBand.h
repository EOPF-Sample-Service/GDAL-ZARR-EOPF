#ifndef EOPF_RASTERBAND_H
#define EOPF_RASTERBAND_H

#include "gdal_priv.h"

class EOPFRasterBand final : public GDALRasterBand
{
public:
    EOPFRasterBand(GDALDataset* poDS, int nBand, GDALDataType eDataType);

    // Override methods from GDALRasterBand
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
};

#endif /* EOPF_RASTERBAND_H */