#include "EOPFRasterBand.h"
#include "EOPFDataset.h"
#include "cpl_conv.h"
#include "cpl_vsi.h"
#include "cpl_string.h"
#include <cstring>
#include <vector>

EOPFRasterBand::EOPFRasterBand(GDALDataset* poDSIn, int nBandIn, GDALDataType eDataTypeIn)
{
    poDS = poDSIn;
    nBand = nBandIn;
    eDataType = eDataTypeIn;

    // For Zarr, set block size equal to chunk dimensions if available.
    EOPFDataset* poEOPFDS = static_cast<EOPFDataset*>(poDSIn);
    // Use public getters instead of private members
    nBlockXSize = poEOPFDS->GetChunkSizeX();
    nBlockYSize = poEOPFDS->GetChunkSizeY();

    // Set a default variable name and chunk directory.
    m_osVarName = CPLSPrintf("band%d", nBand);
    m_osChunkDir = poEOPFDS->GetPath(); // assume chunk files are stored in the same directory
}

CPLErr EOPFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    EOPFDataset* poEOPFDS = static_cast<EOPFDataset*>(poDS);
    if (!poEOPFDS)
        return CE_Failure;

    // For simplicity, assume block corresponds exactly to one chunk.
    // Compute chunk indices:
    int chunkX = nBlockXOff / poEOPFDS->GetChunkSizeX();
    int chunkY = nBlockYOff / poEOPFDS->GetChunkSizeY();

    std::string res = "r10m";  // Extract from m_osCurrentPath
    if (poEOPFDS->m_osCurrentPath.find("r20m") != std::string::npos) res = "r20m";
    else if (poEOPFDS->m_osCurrentPath.find("r60m") != std::string::npos) res = "r60m";
    // Construct chunk filename based on Zarr V2 convention: "chunkY.chunkX"
    CPLString osChunkFile = CPLFormFilename(
        poEOPFDS->m_osPath.c_str(),
        CPLSPrintf("%s/%s/%d.%d",
            poEOPFDS->m_osCurrentPath.c_str(),
            res.c_str(),
            nBlockYOff,
            nBlockXOff),
        nullptr
    );

    // Open the chunk file using VSI functions
    VSILFILE* fp = VSIFOpenL(osChunkFile, "rb");
    if (fp == nullptr)
    {
        // If the chunk file does not exist, fill with fill value (here 0)
        memset(pImage, 0, nBlockXSize * nBlockYSize * GDALGetDataTypeSizeBytes(eDataType));
        return CE_None;
    }

    // Allocate a buffer for the chunk data
    int nBytes = GDALGetDataTypeSizeBytes(eDataType) * poEOPFDS->GetChunkSizeX() * poEOPFDS->GetChunkSizeY();
    std::vector<char> chunkData(nBytes, 0);

    size_t nRead = VSIFReadL(chunkData.data(), 1, nBytes, fp);
    VSIFCloseL(fp);

    if (nRead != (size_t)nBytes)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to read complete chunk from %s", osChunkFile.c_str());
        return CE_Failure;
    }

    // For this implementation, assume the entire chunk is used.
    // If block size exactly equals chunk size, we can simply copy the data.
    memcpy(pImage, chunkData.data(), nBytes);

    CPLDebug("EOPF", "Successfully read chunk (%d, %d) from %s", chunkX, chunkY, osChunkFile.c_str());

    return CE_None;
}
