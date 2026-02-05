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
    bool m_bIsRemoteDataset;  // Track if dataset is remote (for PAM save suppression)

    // Performance optimization flags
    mutable bool mMetadataLoaded;
    mutable bool mGeospatialInfoProcessed;

  public:
    EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDrv);
    ~EOPFZarrDataset();

    // Factory method
    static EOPFZarrDataset* Create(GDALDataset* inner,
                                   GDALDriver* drv,
                                   const char* pszSubdatasetPath = nullptr,
                                   bool bIsRemoteDataset = false);

    // Optimized metadata loading
    void LoadEOPFMetadata();
    void LoadGeospatialInfo() const;
    void ProcessCornerCoordinates() const;
    void ProcessGeolocationArrays();
    void UpdateBandDescriptionsFromMetadata();

    char** GetFileList() override;

    // Override metadata method to handle subdatasets
    char** GetMetadata(const char* pszDomain = nullptr) override;

    // The following methods are now handled by GDALPamDataset
  private:
    char* mProjectionRef = nullptr;
    // but you may want to delegate to inner dataset first
    CPLErr SetSpatialRef(const OGRSpatialReference* poSRS) override;

    // GDAL 3.12+ changed GeoTransform methods to use GDALGeoTransform reference
#ifdef HAVE_GDAL_GEOTRANSFORM
    CPLErr SetGeoTransform(const GDALGeoTransform& padfTransform) override;
    CPLErr GetGeoTransform(GDALGeoTransform& padfTransform) const override;
#else
    CPLErr SetGeoTransform(double* padfTransform) override;
    CPLErr GetGeoTransform(double* padfTransform) override;
#endif

    const OGRSpatialReference* GetSpatialRef() const override;

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
    bool LoadGeoTransformFromCoordinateArrays();
};

/**
 * EOPFZarrMultiBandDataset - A dataset that combines multiple polarization
 * subdatasets (VV/VH or HH/HV) from Sentinel-1 GRD products into a single
 * multi-band dataset for easier access.
 */
class EOPFZarrMultiBandDataset : public GDALPamDataset
{
  private:
    std::vector<std::unique_ptr<GDALDataset>> mPolarizationDatasets;
    std::vector<std::string> mPolarizationNames;
    mutable OGRSpatialReference* mCachedSpatialRef = nullptr;
    bool m_bIsRemoteDataset;

  public:
    EOPFZarrMultiBandDataset();
    ~EOPFZarrMultiBandDataset() override;

    /**
     * Create a multi-band GRD dataset from polarization subdatasets.
     * @param rootPath The root path of the GRD product
     * @param polPaths Vector of pairs: (polarization name, subdataset path)
     * @param drv The EOPFZARR driver
     * @param bIsRemote Whether the dataset is remote
     * @return The created dataset or nullptr on failure
     */
    static EOPFZarrMultiBandDataset* CreateFromPolarizations(
        const std::string& rootPath,
        const std::vector<std::pair<std::string, std::string>>& polPaths,
        GDALDriver* drv,
        bool bIsRemote);

    const OGRSpatialReference* GetSpatialRef() const override;

#ifdef HAVE_GDAL_GEOTRANSFORM
    CPLErr GetGeoTransform(GDALGeoTransform& gt) const override;
#else
    CPLErr GetGeoTransform(double* gt) override;
#endif

    char** GetMetadata(const char* pszDomain = nullptr) override;
    const char* GetMetadataItem(const char* pszName, const char* pszDomain = nullptr) override;
};

/**
 * EOPFZarrMultiBandRasterBand - A raster band that proxies to a polarization
 * subdataset band in a multi-band GRD dataset.
 */
class EOPFZarrMultiBandRasterBand : public GDALPamRasterBand
{
  private:
    GDALDataset* mSourceDataset;  // Not owned - owned by parent EOPFZarrMultiBandDataset
    std::string mPolarizationName;

  public:
    EOPFZarrMultiBandRasterBand(EOPFZarrMultiBandDataset* poDSIn,
                                int nBandIn,
                                GDALDataset* poSourceDS,
                                const std::string& polName);
    ~EOPFZarrMultiBandRasterBand() override;

    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
    CPLErr IRasterIO(GDALRWFlag eRWFlag,
                     int nXOff,
                     int nYOff,
                     int nXSize,
                     int nYSize,
                     void* pData,
                     int nBufXSize,
                     int nBufYSize,
                     GDALDataType eBufType,
                     GSpacing nPixelSpace,
                     GSpacing nLineSpace,
                     GDALRasterIOExtraArg* psExtraArg) override;
};

// Helper function to detect and parse GRD products
bool IsGRDProduct(const std::string& path);
std::vector<std::pair<std::string, std::string>> FindGRDPolarizations(GDALDataset* rootDS,
                                                                      const std::string& rootPath);

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