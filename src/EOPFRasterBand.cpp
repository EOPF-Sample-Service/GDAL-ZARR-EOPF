#include "EOPFRasterBand.h"
#include "EOPFDataset.h"
#include <cstring>

EOPFRasterBand::EOPFRasterBand(EOPFDataset* poDSIn, int nBandIn)
{
    poDS = poDSIn;
    nBand = nBandIn;
    eDataType = GDT_Byte;

    // entire row as block if not Zarr-based chunk logic
    // but let's keep it for skeleton
    nBlockXSize = poDSIn->GetRasterXSize();
    nBlockYSize = 1;
}

CPLErr EOPFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    auto* poEDS = reinterpret_cast<EOPFDataset*>(poDS);
    if (!poEDS)
        return CE_Failure;

    if (poEDS->bIsZarr)
    {
        // compute which chunk we want
        // We'll do a naive approach: each block is "one row"
        // so chunk coords might be (nBlockXOff=0, nBlockYOff=some row).
        // In real Zarr, you'd do chunkX = nBlockXOff / chunkSizeX, chunkY = nBlockYOff / chunkSizeY
        // Then offset within chunk. But let's do a direct call:
        unsigned char buffer[256 * 256]; // if chunk=256x256
        poEDS->ReadChunk(0, 0, nBand, buffer);

        // We only need 1 row from that chunk
        // For demonstration, fill pImage with the first row of the chunk
        // i.e. 256 bytes from buffer
        // if nBlockXSize=512, we handle partial, etc.

        int rowWidth = poEDS->nRasterXSize;
        if (rowWidth > 256) rowWidth = 256; // clamp
        std::memcpy(pImage, buffer, rowWidth);
    }
    else
    {
        // fallback skeleton from Issue#1 (fill zero)
        std::memset(pImage, 0, nBlockXSize);
    }

    return CE_None;
}
