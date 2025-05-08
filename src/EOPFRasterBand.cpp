#include "EOPFRasterBand.h"
#include "EOPFDataset.h"
#include "cpl_conv.h"
#include "cpl_vsi.h"
#include "cpl_string.h"

EOPFRasterBand::EOPFRasterBand(GDALDataset* poDSIn, int nBandIn, GDALDataType eDataTypeIn) {
    poDS = poDSIn;
    nBand = nBandIn;
    eDataType = eDataTypeIn;

    EOPFDataset* poEOPFDS = static_cast<EOPFDataset*>(poDSIn);
    if (!poEOPFDS) {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid dataset pointer");
        return;
    }

    // Set block size from dataset's chunk dimensions
    nBlockXSize = poEOPFDS->GetChunkSizeX();
    nBlockYSize = poEOPFDS->GetChunkSizeY();

    // Get dataset path for chunk directory
    m_osChunkDir = poEOPFDS->GetPath();
}

CPLErr EOPFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) {
    EOPFDataset* poEOPFDS = static_cast<EOPFDataset*>(poDS);

    // Zarr chunk path: <array_dir>/<y>.<x>
    CPLString osChunkFile = CPLFormFilename(
        m_osChunkDir.c_str(),
        CPLSPrintf("%d.%d", nBlockYOff, nBlockXOff),
        nullptr
    );

    VSILFILE* fp = VSIFOpenL(osChunkFile, "rb");
    if (!fp) {
        memset(pImage, 0, nBlockXSize * nBlockYSize * GDALGetDataTypeSizeBytes(eDataType));
        return CE_None;
    }

    size_t nRead = VSIFReadL(pImage, 1, nBlockXSize * nBlockYSize * GDALGetDataTypeSizeBytes(eDataType), fp);
    VSIFCloseL(fp);

    return (nRead == static_cast<size_t>(nBlockXSize * nBlockYSize * GDALGetDataTypeSizeBytes(eDataType)))
        ? CE_None : CE_Failure;
}
