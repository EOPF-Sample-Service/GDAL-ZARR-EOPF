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
    if (!poEOPFDS) {
        CPLError(CE_Failure, CPLE_AppDefined, "Dataset unavailable");
        return CE_Failure;
    }

    // Zarr chunk naming convention: "y.x"
    CPLString osChunkFile = CPLFormFilename(
        m_osChunkDir.c_str(),
        CPLSPrintf("%d.%d", nBlockYOff, nBlockXOff), // Note Y comes first
        nullptr
    );

    VSILFILE* fp = VSIFOpenL(osChunkFile, "rb");
    if (!fp) {
        // Initialize with zeros if chunk missing
        int nPixels = nBlockXSize * nBlockYSize;
        memset(pImage, 0, nPixels * GDALGetDataTypeSizeBytes(eDataType));
        CPLError(CE_Warning, CPLE_FileIO, "Chunk %s not found", osChunkFile.c_str());
        return CE_None;
    }

    // Calculate expected bytes
    size_t nExpectedBytes = GDALGetDataTypeSizeBytes(eDataType) * nBlockXSize * nBlockYSize;
    size_t nRead = VSIFReadL(pImage, 1, nExpectedBytes, fp);
    VSIFCloseL(fp);

    if (nRead != nExpectedBytes) {
        CPLError(CE_Failure, CPLE_FileIO,
            "Short read (%d/%d bytes) for %s",
            (int)nRead, (int)nExpectedBytes, osChunkFile.c_str());
        return CE_Failure;
    }

    return CE_None;
}
