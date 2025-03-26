// src/EOPFDataset.cpp
#include "EOPFDataset.h"
#include "EOPFRasterBand.h"
#include "cpl_conv.h"
#include "cpl_string.h"

EOPFDataset::EOPFDataset() {}
EOPFDataset::~EOPFDataset() {}

// Minimal Identify
int EOPFDataset::Identify(GDALOpenInfo* poOpenInfo)
{
    // Check extension for skeleton
    if (poOpenInfo->pszFilename
        && EQUAL(CPLGetExtension(poOpenInfo->pszFilename), "eopf"))
    {
        return TRUE;
    }
    return FALSE;
}

// Minimal Open
GDALDataset* EOPFDataset::Open(GDALOpenInfo* poOpenInfo)
{
    if (!Identify(poOpenInfo))
        return nullptr;

    if (poOpenInfo->eAccess == GA_Update)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
            "EOPF skeleton driver: read-only");
        return nullptr;
    }

    // Create dataset
    EOPFDataset* poDS = new EOPFDataset();

    // For skeleton: set a dummy size, e.g. 256x256, 1 band
    poDS->nRasterXSize = 256;
    poDS->nRasterYSize = 256;
    poDS->nBands = 1;

    // Create the band
    poDS->SetBand(1, new EOPFRasterBand(poDS, 1));

    return poDS;
}
