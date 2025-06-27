#pragma once
#include "gdal_priv.h"
#include "gdal_pam.h"
#include "gdal_proxy.h"
#include <memory>

namespace EOPF
{
    void AttachMetadata(GDALDataset &ds, const std::string &rootPath);
};

class EOPFZarrDataset : public GDALPamDataset
{
private:
    std::unique_ptr<GDALDataset> mInner;
    char **mSubdatasets;
    mutable OGRSpatialReference *mCachedSpatialRef = nullptr;
    char **m_papszDefaultDomainFilteredMetadata = nullptr;
    bool m_bPamInitialized;

public:
    EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver *selfDrv);
    ~EOPFZarrDataset();

    // Factory method
    static EOPFZarrDataset *Create(GDALDataset *inner, GDALDriver *drv);

    // Load metadata from Zarr file
    void LoadEOPFMetadata();

    char **GetFileList() override;

    // Override metadata method to handle subdatasets
    char **GetMetadata(const char *pszDomain = nullptr) override;

    // The following methods are now handled by GDALPamDataset
private:
    char *mProjectionRef = nullptr;
    // but you may want to delegate to inner dataset first
    CPLErr SetSpatialRef(const OGRSpatialReference *poSRS) override;
    CPLErr SetGeoTransform(double *padfTransform) override;
    const OGRSpatialReference *GetSpatialRef() const override;
    CPLErr GetGeoTransform(double *padfTransform) override;

    // Required PAM methods
    const char *GetDescription() const override;
    int CloseDependentDatasets() override;
    int GetGCPCount() override;
    const OGRSpatialReference *GetGCPSpatialRef() const override;
    const GDAL_GCP *GetGCPs() override;

protected:
    // In your header file:
    CPLErr TryLoadXML(char **papszSiblingFiles = nullptr);

// Use platform detection to handle different GDAL API versions
#if defined(_WIN32) || defined(_WIN64)
    // Windows version
    CPLErr XMLInit(const CPLXMLNode *psTree, const char *pszUnused) override;
#else
    // Linux/other version
    CPLErr XMLInit(CPLXMLNode *psTree, const char *pszUnused) override;
#endif
    CPLXMLNode *SerializeToXML(const char *pszUnused) override;
};

class EOPFZarrRasterBand : public GDALProxyRasterBand
{
private:
    GDALRasterBand *m_poUnderlyingBand;
    EOPFZarrDataset *m_poDS;

public:
    EOPFZarrRasterBand(EOPFZarrDataset *poDS, GDALRasterBand *poUnderlyingBand, int nBand);
    ~EOPFZarrRasterBand();

    // Update the RefUnderlyingRasterBand method

// For Windows, keep your current signature
#if defined(_WIN32) || defined(_WIN64)
    GDALRasterBand* RefUnderlyingRasterBand(bool bForceOpen = true) const override;
#else
    // For Linux, match the pure virtual signature
    GDALRasterBand* RefUnderlyingRasterBand() override;
#endif
};