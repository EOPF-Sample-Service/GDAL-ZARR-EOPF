#pragma once

#include "gdal_priv.h"
#include "gdal_pam.h"
#include "ogr_spatialref.h"
#include "cpl_string.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

// Forward declarations
class EOPFZarrRasterBand;

/**
 * @brief Performance cache for metadata, spatial reference, and network operations
 * 
 * This class provides caching mechanisms to avoid redundant expensive operations:
 * - Metadata parsing and string operations
 * - Spatial reference computations
 * - Network file existence checks
 * - Geotransform calculations
 */
class EOPFPerformanceCache
{
public:
    struct CacheEntry
    {
        std::chrono::time_point<std::chrono::steady_clock> timestamp;
        std::string value;
        bool isValid;
        
        CacheEntry() : isValid(false) {}
        CacheEntry(const std::string& val) 
            : timestamp(std::chrono::steady_clock::now()), value(val), isValid(true) {}
    };
    
    struct NetworkCacheEntry
    {
        std::chrono::time_point<std::chrono::steady_clock> timestamp;
        bool exists;
        bool isValid;
        
        NetworkCacheEntry() : isValid(false) {}
        NetworkCacheEntry(bool fileExists) 
            : timestamp(std::chrono::steady_clock::now()), exists(fileExists), isValid(true) {}
    };

private:
    static constexpr std::chrono::minutes CACHE_TTL{5}; // 5 minutes cache TTL
    static constexpr std::chrono::minutes NETWORK_CACHE_TTL{2}; // 2 minutes for network operations
    
    std::unordered_map<std::string, CacheEntry> mMetadataCache;
    std::unordered_map<std::string, NetworkCacheEntry> mNetworkCache;
    char** mCachedSubdatasets;
    char** mCachedMetadata;
    std::unique_ptr<OGRSpatialReference> mCachedSpatialRef;
    double mCachedGeoTransform[6];
    bool mHasCachedGeoTransform;
    
public:
    EOPFPerformanceCache();
    ~EOPFPerformanceCache();
    
    // Metadata caching
    const char* GetCachedMetadataItem(const std::string& key);
    void SetCachedMetadataItem(const std::string& key, const std::string& value);
    
    // Network operation caching
    bool HasCachedFileCheck(const std::string& path);
    bool GetCachedFileExists(const std::string& path);
    void SetCachedFileExists(const std::string& path, bool exists);
    
    // Subdataset caching
    char** GetCachedSubdatasets();
    void SetCachedSubdatasets(char** subdatasets);
    bool HasCachedSubdatasets() const;
    
    // Full metadata caching
    char** GetCachedMetadata();
    void SetCachedMetadata(char** metadata);
    bool HasCachedMetadata() const;
    
    // Spatial reference caching
    const OGRSpatialReference* GetCachedSpatialRef();
    void SetCachedSpatialRef(const OGRSpatialReference* srs);
    bool HasCachedSpatialRef() const;
    
    // Geotransform caching
    bool GetCachedGeoTransform(double* transform);
    void SetCachedGeoTransform(const double* transform);
    bool HasCachedGeoTransform() const;
    
    // Cache management
    void ClearExpiredEntries();
    void ClearAllCaches();
    
private:
    bool IsExpired(const std::chrono::time_point<std::chrono::steady_clock>& timestamp, 
                   std::chrono::minutes ttl) const;
};

/**
 * @brief Optimized EOPF-Zarr Dataset wrapper with performance enhancements
 * 
 * Performance optimizations include:
 * - Metadata caching to avoid repeated string parsing
 * - Spatial reference caching for expensive CRS operations  
 * - Network operation caching to reduce HTTP calls
 * - Lazy loading of expensive operations
 * - Memory pool for frequently allocated objects
 */
class EOPFZarrDatasetPerf : public GDALPamDataset
{
private:
    std::unique_ptr<GDALDataset> mInner;
    mutable EOPFPerformanceCache mCache;
    
    // Legacy members for compatibility
    char **mSubdatasets;
    OGRSpatialReference *mCachedSpatialRef;
    char **m_papszDefaultDomainFilteredMetadata;
    bool m_bPamInitialized;
    
    // Performance optimization flags
    mutable bool mMetadataLoaded;
    mutable bool mGeospatialInfoProcessed;

public:
    EOPFZarrDatasetPerf(std::unique_ptr<GDALDataset> inner, GDALDriver *selfDrv);
    virtual ~EOPFZarrDatasetPerf();

    static EOPFZarrDatasetPerf *Create(GDALDataset *inner, GDALDriver *drv);

    // Optimized metadata loading
    void LoadEOPFMetadata();
    void LoadGeospatialInfo() const;

    // GDALDataset interface - optimized implementations
    CPLErr GetGeoTransform(double *padfTransform) override;
    CPLErr SetSpatialRef(const OGRSpatialReference *poSRS) override;
    CPLErr SetGeoTransform(double *padfTransform) override;
    const OGRSpatialReference *GetSpatialRef() const override;
    char **GetMetadata(const char *pszDomain = "") override;
    char **GetFileList() override;
    const char *GetDescription() const override;
    int CloseDependentDatasets() override;

    // GCP support
    int GetGCPCount() override;
    const OGRSpatialReference *GetGCPSpatialRef() const override;
    const GDAL_GCP *GetGCPs() override;

private:
    // Performance helper methods
    void OptimizedMetadataMerge() const;
    void CacheGeotransformFromCorners(double minX, double maxX, double minY, double maxY);
    bool TryFastPathMetadata(const char* key, const char** outValue) const;
};

/**
 * @brief Optimized raster band with intelligent caching and prefetching
 */
class EOPFZarrRasterBandPerf : public GDALRasterBand
{
private:
    GDALRasterBand *m_poUnderlyingBand;
    EOPFZarrDatasetPerf *m_poDS;
    
    // Performance optimization members
    struct PairHash {
        size_t operator()(const std::pair<int,int>& p) const {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };
    
    mutable std::unordered_map<std::pair<int,int>, std::chrono::time_point<std::chrono::steady_clock>, 
                              PairHash> mBlockAccessTimes;
    static constexpr size_t MAX_BLOCK_CACHE_SIZE = 64; // Maximum blocks to track for access patterns

public:
    EOPFZarrRasterBandPerf(EOPFZarrDatasetPerf *poDS, GDALRasterBand *poUnderlyingBand, int nBand);
    virtual ~EOPFZarrRasterBandPerf();

    // Optimized data access
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void *pImage) override;

private:
    // Performance optimization methods
    void TrackBlockAccess(int nBlockXOff, int nBlockYOff) const;
    bool ShouldPrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff) const;
    void PrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff) const;
};

// Performance helper functions
namespace EOPFPerformanceUtils {
    /**
     * @brief Fast file existence check with caching for network paths
     */
    bool FastFileExists(const std::string& path, EOPFPerformanceCache& cache);
    
    /**
     * @brief Optimized string tokenization for metadata parsing
     */
    std::vector<std::string> FastTokenize(const std::string& input, char delimiter);
    
    /**
     * @brief Memory-efficient CSL operations
     */
    char** OptimizedCSLDuplicate(char** papszSource);
    char** OptimizedCSLSetNameValue(char** papszList, const char* pszName, const char* pszValue);
    
    /**
     * @brief Fast path detection for different URL types
     */
    enum class PathType {
        LOCAL_FILE,
        NETWORK_HTTP,
        VSI_CURL,
        VSI_S3,
        VSI_AZURE,
        UNKNOWN
    };
    
    PathType DetectPathType(const std::string& path);
    bool IsNetworkPath(const std::string& path);
    
    /**
     * @brief Performance profiling helpers
     */
    class ScopedTimer {
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        const char* operation;
    public:
        explicit ScopedTimer(const char* op);
        ~ScopedTimer();
    };
}

#define EOPF_PERF_TIMER(op) EOPFPerformanceUtils::ScopedTimer timer(op)
