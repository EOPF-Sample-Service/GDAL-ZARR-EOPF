#ifndef EOPFRASTERBAND_H
#define EOPFRASTERBAND_H

#include "gdal_pam.h"

class EOPFDataset;

class EOPFRasterBand final : public GDALPamRasterBand
{
public:
    EOPFRasterBand(EOPFDataset* poDS, int nBand);
    ~EOPFRasterBand() override {}

    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
};

#endif
