#pragma once
#include "gdal_priv.h"
#include "gdal_pam.h"
#include "gdal_version.h"
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
    
    // XMLInit signature matches base class GDALPamDataset
    CPLErr XMLInit(CPLXMLNode *psTree, const char *pszUnused) override;

    CPLXMLNode *SerializeToXML(const char *pszUnused) override;
};

class EOPFZarrRasterBand : public GDALProxyRasterBand
{
  private:
    GDALRasterBand *m_poUnderlyingBand;
    EOPFZarrDataset *m_poDS;

  public:
    EOPFZarrRasterBand(EOPFZarrDataset *poDS, GDALRasterBand *poUnderlyingBand,
                       int nBand);
    ~EOPFZarrRasterBand();

#if GDAL_VERSION_NUM >= 3060000
    // Modern GDAL (3.6.0+) - const method with bool parameter
    GDALRasterBand *RefUnderlyingRasterBand(bool bForceOpen = true) const override;
#else
    // Older GDAL - simple signature
    GDALRasterBand *RefUnderlyingRasterBand() override;
#endif
    // Add the IReadBlock method
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void *pImage) override;
};