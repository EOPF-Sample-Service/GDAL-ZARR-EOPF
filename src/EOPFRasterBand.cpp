// src/EOPFRasterBand.cpp
#include "EOPFRasterBand.h"
#include "EOPFDataset.h"
#include "cpl_conv.h"
#include <cstring>

EOPFRasterBand::EOPFRasterBand(EOPFDataset* poDSIn, int nBandIn)
{
    poDS = poDSIn;
    nBand = nBandIn;
    eDataType = GDT_Byte;
    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

// Minimal read => fill row with zeros
CPLErr EOPFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    memset(pImage, 0, nBlockXSize);
    return CE_None;
}
