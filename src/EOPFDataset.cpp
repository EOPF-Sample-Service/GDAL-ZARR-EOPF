#include "EOPFDataset.h"
#include "EOPFRasterBand.h"
#include "cpl_conv.h"
#include "cpl_string.h"

EOPFDataset::EOPFDataset() {}
EOPFDataset::~EOPFDataset() {}

int EOPFDataset::Identify(GDALOpenInfo* poOpenInfo)
{
    // We'll handle .zarr extension for Issue #2 (Zarr read).
    if (poOpenInfo->pszFilename
        && EQUAL(CPLGetExtension(poOpenInfo->pszFilename), "zarr"))
    {
        return TRUE;
    }
    // fallback: check if extension is .eopf (Issue #1 skeleton)
    if (poOpenInfo->pszFilename
        && EQUAL(CPLGetExtension(poOpenInfo->pszFilename), "eopf"))
    {
        return TRUE;
    }
    return FALSE;
}

GDALDataset* EOPFDataset::Open(GDALOpenInfo* poOpenInfo)
{
    if (!Identify(poOpenInfo))
        return nullptr;

    if (poOpenInfo->eAccess == GA_Update)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
            "EOPF plugin is read-only for now");
        return nullptr;
    }

    // Create dataset
    EOPFDataset* poDS = new EOPFDataset();

    // Distinguish if it's .zarr or .eopf
    const char* ext = CPLGetExtension(poOpenInfo->pszFilename);
    if (EQUAL(ext, "zarr")) {
        poDS->bIsZarr = true;
        // Maybe we do some dummy read of a .zarray file if it existed
        // ...
        // We'll store dummy size, e.g. 512 x 512 for Zarr
        poDS->nRasterXSize = 512;
        poDS->nRasterYSize = 512;
        // multi-bands possible, but let's do 1 for now
        poDS->nBands = 1;
    }
    else {
        // fallback skeleton from Issue #1
        poDS->nRasterXSize = 256;
        poDS->nRasterYSize = 256;
        poDS->nBands = 1;
    }

    // create band(s)
    poDS->SetBand(1, new EOPFRasterBand(poDS, 1));

    return poDS;
}

// This is the "chunk read" placeholder
CPLErr EOPFDataset::ReadChunk(int chunkX, int chunkY, int band, void* pBuffer)
{
    // For demonstration: fill chunk with a distinct pattern
    // e.g. band # in the pattern so we see difference if multi-band
    unsigned char fillVal = static_cast<unsigned char>(band * 50 + 25);
    // we do chunkSizeX * chunkSizeY bytes
    int totalBytes = chunkSizeX * chunkSizeY;
    memset(pBuffer, fillVal, totalBytes);

    return CE_None;
}
