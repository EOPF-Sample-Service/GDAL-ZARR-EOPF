// src/EOPFDataset.cpp
#include "EOPFDataset.h"
#include "EOPFRasterBand.h"
#include "cpl_conv.h"
#include "cpl_string.h"

EOPFDataset::EOPFDataset() {}
EOPFDataset::~EOPFDataset() {}

int EOPFDataset::Identify(GDALOpenInfo* poOpenInfo)
{
    // minimal check: if the extension is .eopf
    if (poOpenInfo->pszFilename && EQUAL(CPLGetExtension(poOpenInfo->pszFilename), "eopf"))
        return TRUE;
    return FALSE;
}

GDALDataset* EOPFDataset::Open(GDALOpenInfo* poOpenInfo)
{
    if (!Identify(poOpenInfo))
        return nullptr;

    if (poOpenInfo->eAccess == GA_Update)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
            "EOPF skeleton plugin is read-only");
        return nullptr;
    }

    EOPFDataset* poDS = new EOPFDataset();
    // set a dummy 256x256 dimension
    poDS->nRasterXSize = 256;
    poDS->nRasterYSize = 256;
    poDS->nBands = 1;

    // create 1 band
    poDS->SetBand(1, new EOPFRasterBand(poDS, 1));
    return poDS;
}
