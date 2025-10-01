#pragma once
#include <memory>

#include "eopfzarr_performance.h"
#include "gdal_pam.h"
#include "gdal_priv.h"
#include "gdal_proxy.h"
#include "gdal_version.h"

namespace EOPF
{
void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
};

class EOPFZarrDataset : public GDALPamDataset
{
  private:
    std::unique_ptr<GDALDataset> mInner;
    mutable EOPFPerformanceCache mCache;

    // Legacy members for compatibility
    char** mSubdatasets;
    mutable OGRSpatialReference* mCachedSpatialRef = nullptr;
    mutable char** m_papszDefaultDomainFilteredMetadata = nullptr;
    bool m_bPamInitialized;

    // Performance optimization flags
    mutable bool mMetadataLoaded;
    mutable bool mGeospatialInfoProcessed;

  public:
    EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDrv);
    ~EOPFZarrDataset();

    // Factory method
    static EOPFZarrDataset* Create(GDALDataset* inner,
                                   GDALDriver* drv,
                                   const char* pszSubdatasetPath = nullptr);

    // Optimized metadata loading
    void LoadEOPFMetadata();
    void LoadGeospatialInfo() const;
    void ProcessCornerCoordinates() const;
    void UpdateBandDescriptionsFromMetadata();

    char** GetFileList() override;

    // Override metadata method to handle subdatasets
    char** GetMetadata(const char* pszDomain = nullptr) override;

    // The following methods are now handled by GDALPamDataset
  private:
    char* mProjectionRef = nullptr;
    // but you may want to delegate to inner dataset first
    CPLErr SetSpatialRef(const OGRSpatialReference* poSRS) override;
    CPLErr SetGeoTransform(double* padfTransform) override;
    const OGRSpatialReference* GetSpatialRef() const override;
    CPLErr GetGeoTransform(double* padfTransform) override;

    // Required PAM methods
    const char* GetDescription() const override;
    int CloseDependentDatasets() override;
    int GetGCPCount() override;
    const OGRSpatialReference* GetGCPSpatialRef() const override;
    const GDAL_GCP* GetGCPs() override;

  protected:
    // In your header file:
    CPLErr TryLoadXML(char** papszSiblingFiles = nullptr);

    // XMLInit signature varies by GDAL version
#ifdef GDAL_HAS_CONST_XML_NODE
    CPLErr XMLInit(const CPLXMLNode* psTree, const char* pszUnused) override;
#else
    CPLErr XMLInit(CPLXMLNode* psTree, const char* pszUnused) override;
#endif

    CPLXMLNode* SerializeToXML(const char* pszUnused) override;

  private:
    // Performance helper methods
    void OptimizedMetadataMerge() const;
    void CacheGeotransformFromCorners(double minX, double maxX, double minY, double maxY);
    bool TryFastPathMetadata(const char* key, const char** outValue) const;
};

class EOPFZarrRasterBand : public GDALProxyRasterBand
{
  private:
    GDALRasterBand* m_poUnderlyingBand;
    EOPFZarrDataset* m_poDS;

    // Performance optimization members
    struct PairHash
    {
        size_t operator()(const std::pair<int, int>& p) const
        {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };

    mutable std::unordered_map<std::pair<int, int>,
                               std::chrono::time_point<std::chrono::steady_clock>,
                               PairHash>
        mBlockAccessTimes;
    static constexpr size_t MAX_BLOCK_CACHE_SIZE = 64;

  public:
    EOPFZarrRasterBand(EOPFZarrDataset* poDS, GDALRasterBand* poUnderlyingBand, int nBand);
    ~EOPFZarrRasterBand();

    // RefUnderlyingRasterBand signature varies by GDAL version
#ifdef GDAL_HAS_CONST_REF_UNDERLYING
    GDALRasterBand* RefUnderlyingRasterBand(bool bForceOpen = true) const override;
#else
    GDALRasterBand* RefUnderlyingRasterBand() override;
#endif
    // Add the IReadBlock method
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;

  private:
    // Performance optimization methods
    void TrackBlockAccess(int nBlockXOff, int nBlockYOff) const;
    bool ShouldPrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff) const;
    void PrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff);
};