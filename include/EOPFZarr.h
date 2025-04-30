// eopf_zarr.h
#ifndef EOPF_ZARR_H
#define EOPF_ZARR_H

#include "gdal_priv.h"
#include "cpl_vsi.h"
#include "cpl_json.h"
#include <vector>
#include <memory>
#include <string>

class EOPFZarrRasterBand;  // Forward declaration

class EOPFZarrDataset final : public GDALDataset {
    friend class EOPFZarrRasterBand;
private:
    std::vector<GDALDataset*> m_apoUnderDS;  // Underlying Zarr datasets (one per array)
    char** m_papszSubdatasets = nullptr;     // Subdataset name list (for multiple arrays)
public:
    EOPFZarrDataset() = default;
    ~EOPFZarrDataset() override;

    // Identify and Open static methods for GDAL
    static int Identify(GDALOpenInfo* poOpenInfo);
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);

    // (Optional: implementation for Create() if write support is needed in future)

    // Override GetMetadata to expose subdatasets if present
    char** GetMetadata(const char* pszDomain) override {
        if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS") && m_papszSubdatasets)
            return m_papszSubdatasets;
        return GDALDataset::GetMetadata(pszDomain);
    }
};

class EOPFZarrRasterBand final : public GDALRasterBand {
private:
    GDALRasterBand* m_poBaseBand;  // Underlying Zarr raster band
public:
    EOPFZarrRasterBand(EOPFZarrDataset* poDS, int nBand, GDALRasterBand* poBaseBand) {
        this->poDS = poDS;
        this->nBand = nBand;
        m_poBaseBand = poBaseBand;
        // Match data type and block size to underlying band
        this->eDataType = m_poBaseBand->GetRasterDataType();
        int nBlkX, nBlkY;
        m_poBaseBand->GetBlockSize(&nBlkX, &nBlkY);
        this->nBlockXSize = nBlkX;
        this->nBlockYSize = nBlkY;
        // Set raster dimensions (in case not already set in dataset)
        this->nRasterXSize = m_poBaseBand->GetXSize();
        this->nRasterYSize = m_poBaseBand->GetYSize();
    }

    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
    // (Optionally override other methods like IWriteBlock, if write support is added)
};

#endif // EOPF_ZARR_H
