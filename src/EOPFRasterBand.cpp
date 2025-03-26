// src/EOPFRasterBand.cpp
#include "EOPFRasterBand.h"
#include "EOPFDataset.h"
#include <cstring> // memset

EOPFRasterBand::EOPFRasterBand(EOPFDataset* poDSIn, int nBandIn)
{
    poDS = poDSIn;
    nBand = nBandIn;

    eDataType = GDT_Byte;
    // entire row is a block
    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

CPLErr EOPFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    // fill row with zeros
    std::memset(pImage, 0, nBlockXSize);
    return CE_None;
}
