#include "EOPFRasterBand.h"

EOPFRasterBand::EOPFRasterBand(GDALDataset* poDS, int nBand, GDALDataType eDataType)
{
    this->poDS = poDS;
    this->nBand = nBand;
    this->eDataType = eDataType;

    // Set default block size (to be determined from Zarr chunks)
    nBlockXSize = 256;
    nBlockYSize = 256;
}

CPLErr EOPFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    // Not implemented yet - just fill with zeros for now
    memset(pImage, 0, nBlockXSize * nBlockYSize * GDALGetDataTypeSizeBytes(eDataType));
    return CE_None;
}