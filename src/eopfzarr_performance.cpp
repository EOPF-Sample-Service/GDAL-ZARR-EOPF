#include "eopfzarr_performance.h"

#include <algorithm>
#include <sstream>

#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// ============================================================================
// EOPFPerformanceCache Implementation
// ============================================================================

EOPFPerformanceCache::EOPFPerformanceCache()
    : mCachedSubdatasets(nullptr),
      mCachedMetadata(nullptr),
      mCachedSpatialRef(nullptr),
      mHasCachedGeoTransform(false)
{
    std::fill(mCachedGeoTransform, mCachedGeoTransform + 6, 0.0);
}

EOPFPerformanceCache::~EOPFPerformanceCache()
{
    ClearAllCaches();
}

const char* EOPFPerformanceCache::GetCachedMetadataItem(const std::string& key)
{
    auto it = mMetadataCache.find(key);
    if (it != mMetadataCache.end() && it->second.isValid)
    {
        if (!IsExpired(it->second.timestamp, CACHE_TTL))
        {
            return it->second.value.c_str();
        }
        else
        {
            // Mark as invalid but don't erase yet (cleanup will handle it)
            it->second.isValid = false;
        }
    }
    return nullptr;
}

void EOPFPerformanceCache::SetCachedMetadataItem(const std::string& key, const std::string& value)
{
    mMetadataCache[key] = CacheEntry(value);
}

bool EOPFPerformanceCache::HasCachedFileCheck(const std::string& path)
{
    auto it = mNetworkCache.find(path);
    if (it != mNetworkCache.end() && it->second.isValid)
    {
        return !IsExpired(it->second.timestamp, NETWORK_CACHE_TTL);
    }
    return false;
}

bool EOPFPerformanceCache::GetCachedFileExists(const std::string& path)
{
    auto it = mNetworkCache.find(path);
    if (it != mNetworkCache.end() && it->second.isValid)
    {
        if (!IsExpired(it->second.timestamp, NETWORK_CACHE_TTL))
        {
            return it->second.exists;
        }
        else
        {
            it->second.isValid = false;
        }
    }
    return false;  // Default to false if not cached or expired
}

void EOPFPerformanceCache::SetCachedFileExists(const std::string& path, bool exists)
{
    mNetworkCache[path] = NetworkCacheEntry(exists);
}

char** EOPFPerformanceCache::GetCachedSubdatasets()
{
    return mCachedSubdatasets;
}

void EOPFPerformanceCache::SetCachedSubdatasets(char** subdatasets)
{
    if (mCachedSubdatasets)
    {
        CSLDestroy(mCachedSubdatasets);
    }
    mCachedSubdatasets = CSLDuplicate(subdatasets);
}

char** EOPFPerformanceCache::GetCachedMetadata()
{
    return mCachedMetadata;
}

void EOPFPerformanceCache::SetCachedMetadata(char** metadata)
{
    if (mCachedMetadata)
    {
        CSLDestroy(mCachedMetadata);
    }
    mCachedMetadata = CSLDuplicate(metadata);
}

const OGRSpatialReference* EOPFPerformanceCache::GetCachedSpatialRef()
{
    return mCachedSpatialRef.get();
}

void EOPFPerformanceCache::SetCachedSpatialRef(const OGRSpatialReference* srs)
{
    if (srs)
    {
        mCachedSpatialRef = std::make_unique<OGRSpatialReference>(*srs);
    }
    else
    {
        mCachedSpatialRef.reset();
    }
}

bool EOPFPerformanceCache::GetCachedGeoTransform(double* transform)
{
    if (mHasCachedGeoTransform && transform)
    {
        std::copy(mCachedGeoTransform, mCachedGeoTransform + 6, transform);
        return true;
    }
    return false;
}

void EOPFPerformanceCache::SetCachedGeoTransform(const double* transform)
{
    if (transform)
    {
        std::copy(transform, transform + 6, mCachedGeoTransform);
        mHasCachedGeoTransform = true;
    }
    else
    {
        mHasCachedGeoTransform = false;
    }
}

bool EOPFPerformanceCache::HasCachedSubdatasets() const
{
    return mCachedSubdatasets != nullptr;
}

bool EOPFPerformanceCache::HasCachedMetadata() const
{
    return mCachedMetadata != nullptr;
}

bool EOPFPerformanceCache::HasCachedSpatialRef() const
{
    return mCachedSpatialRef != nullptr;
}

bool EOPFPerformanceCache::HasCachedGeoTransform() const
{
    return mHasCachedGeoTransform;
}

void EOPFPerformanceCache::ClearExpiredEntries()
{
    // Clear expired metadata entries
    auto metaIt = mMetadataCache.begin();
    while (metaIt != mMetadataCache.end())
    {
        if (!metaIt->second.isValid || IsExpired(metaIt->second.timestamp, CACHE_TTL))
        {
            metaIt = mMetadataCache.erase(metaIt);
        }
        else
        {
            ++metaIt;
        }
    }

    // Clear expired network entries
    auto netIt = mNetworkCache.begin();
    while (netIt != mNetworkCache.end())
    {
        if (!netIt->second.isValid || IsExpired(netIt->second.timestamp, NETWORK_CACHE_TTL))
        {
            netIt = mNetworkCache.erase(netIt);
        }
        else
        {
            ++netIt;
        }
    }
}

void EOPFPerformanceCache::ClearAllCaches()
{
    mMetadataCache.clear();
    mNetworkCache.clear();

    if (mCachedSubdatasets)
    {
        CSLDestroy(mCachedSubdatasets);
        mCachedSubdatasets = nullptr;
    }

    if (mCachedMetadata)
    {
        CSLDestroy(mCachedMetadata);
        mCachedMetadata = nullptr;
    }

    mCachedSpatialRef.reset();
    mHasCachedGeoTransform = false;
}

bool EOPFPerformanceCache::IsExpired(
    const std::chrono::time_point<std::chrono::steady_clock>& timestamp,
    std::chrono::minutes ttl) const
{
    auto now = std::chrono::steady_clock::now();
    return (now - timestamp) > ttl;
}

// ============================================================================
// EOPFPerformanceUtils Implementation
// ============================================================================

namespace EOPFPerformanceUtils
{

bool FastFileExists(const std::string& path, EOPFPerformanceCache& cache)
{
    // Check cache first for network paths
    if (IsNetworkPath(path))
    {
        if (cache.HasCachedFileCheck(path))
        {
            return cache.GetCachedFileExists(path);
        }
    }

    // Perform actual check
    VSIStatBufL sStat;
    bool exists = VSIStatL(path.c_str(), &sStat) == 0;

    // Cache result for network paths
    if (IsNetworkPath(path))
    {
        cache.SetCachedFileExists(path, exists);
    }

    return exists;
}

std::vector<std::string> FastTokenize(const std::string& input, char delimiter)
{
    std::vector<std::string> tokens;
    tokens.reserve(8);  // Reserve space for common case

    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.emplace_back(std::move(token));
    }

    return tokens;
}

char** OptimizedCSLDuplicate(char** papszSource)
{
    if (!papszSource)
        return nullptr;

    // Count entries first
    int nCount = CSLCount(papszSource);
    if (nCount == 0)
        return nullptr;

    // Allocate in one block for better memory locality
    char** papszDest = static_cast<char**>(CPLCalloc(nCount + 1, sizeof(char*)));

    for (int i = 0; i < nCount; i++)
    {
        papszDest[i] = CPLStrdup(papszSource[i]);
    }

    return papszDest;
}

char** OptimizedCSLSetNameValue(char** papszList, const char* pszName, const char* pszValue)
{
    // Use standard CSL function but could be optimized further with custom implementation
    return CSLSetNameValue(papszList, pszName, pszValue);
}

PathType DetectPathType(const std::string& path)
{
    if (path.empty())
        return PathType::UNKNOWN;

    // Check for VSI prefixes (fast string comparison)
    if (path.length() >= 9 && path.substr(0, 9) == "/vsicurl/")
        return PathType::VSI_CURL;

    if (path.length() >= 6 && path.substr(0, 6) == "/vsis3/")
        return PathType::VSI_S3;

    if ((path.length() >= 7 && path.substr(0, 7) == "/vsiaz/") ||
        (path.length() >= 11 && path.substr(0, 11) == "/vsiazure/"))
        return PathType::VSI_AZURE;

    // Check for HTTP/HTTPS
    if (path.length() >= 7 && path.substr(0, 7) == "http://")
        return PathType::NETWORK_HTTP;

    if (path.length() >= 8 && path.substr(0, 8) == "https://")
        return PathType::NETWORK_HTTP;

    // Check for other VSI types
    if (path.length() >= 4 && path.substr(0, 4) == "/vsi")
        return PathType::VSI_CURL;  // Generic VSI

    return PathType::LOCAL_FILE;
}

bool IsNetworkPath(const std::string& path)
{
    PathType type = DetectPathType(path);
    return type == PathType::NETWORK_HTTP || type == PathType::VSI_CURL ||
           type == PathType::VSI_S3 || type == PathType::VSI_AZURE;
}

ScopedTimer::ScopedTimer(const char* op)
    : start(std::chrono::high_resolution_clock::now()), operation(op)
{
}

ScopedTimer::~ScopedTimer()
{
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Only log if operation takes more than 1ms to avoid spam
    if (duration.count() > 1000)
    {
        CPLDebug("EOPFZARR_PERF",
                 "%s took %lld microseconds",
                 operation,
                 static_cast<long long>(duration.count()));
    }
}

}  // namespace EOPFPerformanceUtils

// ============================================================================
// EOPFZarrDatasetPerf Implementation
// ============================================================================

EOPFZarrDatasetPerf::EOPFZarrDatasetPerf(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDrv)
    : mInner(std::move(inner)),
      mSubdatasets(nullptr),
      mCachedSpatialRef(nullptr),
      m_papszDefaultDomainFilteredMetadata(nullptr),
      m_bPamInitialized(false),
      mMetadataLoaded(false),
      mGeospatialInfoProcessed(false)
{
    poDriver = selfDrv;
    SetDescription(mInner->GetDescription());

    // Initialize GDAL raster dimensions
    nRasterXSize = mInner->GetRasterXSize();
    nRasterYSize = mInner->GetRasterYSize();

    // Create performance-optimized bands
    for (int i = 1; i <= mInner->GetRasterCount(); ++i)
    {
        GDALRasterBand* originalBand = mInner->GetRasterBand(i);
        SetBand(i, new EOPFZarrRasterBandPerf(this, originalBand, i));
    }
}

EOPFZarrDatasetPerf::~EOPFZarrDatasetPerf()
{
    if (mSubdatasets)
        CSLDestroy(mSubdatasets);
    if (m_papszDefaultDomainFilteredMetadata)
        CSLDestroy(m_papszDefaultDomainFilteredMetadata);
    if (mCachedSpatialRef)
        delete mCachedSpatialRef;
}

EOPFZarrDatasetPerf* EOPFZarrDatasetPerf::Create(GDALDataset* inner, GDALDriver* drv)
{
    if (!inner || !drv)
        return nullptr;

    // Transfer ownership to unique_ptr
    std::unique_ptr<GDALDataset> innerPtr(inner);
    return new EOPFZarrDatasetPerf(std::move(innerPtr), drv);
}

void EOPFZarrDatasetPerf::LoadEOPFMetadata()
{
    EOPF_PERF_TIMER("EOPFZarrDatasetPerf::LoadEOPFMetadata");

    if (mMetadataLoaded)
        return;

    // Use cached metadata if available
    if (mCache.HasCachedMetadata())
    {
        m_papszDefaultDomainFilteredMetadata = CSLDuplicate(mCache.GetCachedMetadata());
        mMetadataLoaded = true;
        return;
    }

    // Load metadata from inner dataset
    char** innerMetadata = mInner->GetMetadata();
    if (innerMetadata)
    {
        OptimizedMetadataMerge();
        mCache.SetCachedMetadata(m_papszDefaultDomainFilteredMetadata);
    }

    mMetadataLoaded = true;
}

void EOPFZarrDatasetPerf::LoadGeospatialInfo() const
{
    EOPF_PERF_TIMER("EOPFZarrDatasetPerf::LoadGeospatialInfo");

    if (mGeospatialInfoProcessed)
        return;

    // Check cache for spatial reference
    if (mCache.HasCachedSpatialRef())
    {
        EOPFZarrDatasetPerf* self = const_cast<EOPFZarrDatasetPerf*>(this);
        if (!self->mCachedSpatialRef)
            self->mCachedSpatialRef = new OGRSpatialReference();
        *self->mCachedSpatialRef = *mCache.GetCachedSpatialRef();
    }
    else if (mInner->GetSpatialRef())
    {
        EOPFZarrDatasetPerf* self = const_cast<EOPFZarrDatasetPerf*>(this);
        if (!self->mCachedSpatialRef)
            self->mCachedSpatialRef = new OGRSpatialReference();
        *self->mCachedSpatialRef = *mInner->GetSpatialRef();
        self->mCache.SetCachedSpatialRef(self->mCachedSpatialRef);
    }

    mGeospatialInfoProcessed = true;
}

#ifdef HAVE_GDAL_GEOTRANSFORM
CPLErr EOPFZarrDatasetPerf::GetGeoTransform(GDALGeoTransform& padfTransform) const
#else
CPLErr EOPFZarrDatasetPerf::GetGeoTransform(double* padfTransform)
#endif
{
    EOPF_PERF_TIMER("EOPFZarrDatasetPerf::GetGeoTransform");

    if (mCache.HasCachedGeoTransform())
    {
#ifdef HAVE_GDAL_GEOTRANSFORM
        if (mCache.GetCachedGeoTransform(padfTransform.data()))
#else
        if (mCache.GetCachedGeoTransform(padfTransform))
#endif
        {
            return CE_None;
        }
    }

    CPLErr err = mInner->GetGeoTransform(padfTransform);
    if (err == CE_None)
    {
#ifdef HAVE_GDAL_GEOTRANSFORM
        mCache.SetCachedGeoTransform(padfTransform.data());
#else
        mCache.SetCachedGeoTransform(padfTransform);
#endif
    }

    return err;
}

CPLErr EOPFZarrDatasetPerf::SetSpatialRef(const OGRSpatialReference* poSRS)
{
    CPLErr err = mInner->SetSpatialRef(poSRS);
    if (err == CE_None && poSRS)
    {
        if (!mCachedSpatialRef)
            mCachedSpatialRef = new OGRSpatialReference();
        *mCachedSpatialRef = *poSRS;
        mCache.SetCachedSpatialRef(mCachedSpatialRef);
    }
    return err;
}

#ifdef HAVE_GDAL_GEOTRANSFORM
CPLErr EOPFZarrDatasetPerf::SetGeoTransform(const GDALGeoTransform& padfTransform)
#else
CPLErr EOPFZarrDatasetPerf::SetGeoTransform(double* padfTransform)
#endif
{
    CPLErr err = mInner->SetGeoTransform(padfTransform);
    if (err == CE_None)
    {
#ifdef HAVE_GDAL_GEOTRANSFORM
        mCache.SetCachedGeoTransform(padfTransform.data());
#else
        mCache.SetCachedGeoTransform(padfTransform);
#endif
    }
    return err;
}

const OGRSpatialReference* EOPFZarrDatasetPerf::GetSpatialRef() const
{
    LoadGeospatialInfo();
    return mCachedSpatialRef ? mCachedSpatialRef : mInner->GetSpatialRef();
}

char** EOPFZarrDatasetPerf::GetMetadata(const char* pszDomain)
{
    EOPF_PERF_TIMER("EOPFZarrDatasetPerf::GetMetadata");

    if (!pszDomain || EQUAL(pszDomain, ""))
    {
        const_cast<EOPFZarrDatasetPerf*>(this)->LoadEOPFMetadata();
        return m_papszDefaultDomainFilteredMetadata;
    }

    return mInner->GetMetadata(pszDomain);
}

char** EOPFZarrDatasetPerf::GetFileList()
{
    if (mCache.HasCachedSubdatasets())
    {
        return CSLDuplicate(mCache.GetCachedSubdatasets());
    }

    char** fileList = mInner->GetFileList();
    if (fileList)
    {
        mCache.SetCachedSubdatasets(fileList);
    }

    return fileList;
}

const char* EOPFZarrDatasetPerf::GetDescription() const
{
    return mInner->GetDescription();
}

int EOPFZarrDatasetPerf::CloseDependentDatasets()
{
    // GDAL base class implementation - just return success
    return 0;
}

int EOPFZarrDatasetPerf::GetGCPCount()
{
    return mInner->GetGCPCount();
}

const OGRSpatialReference* EOPFZarrDatasetPerf::GetGCPSpatialRef() const
{
    return mInner->GetGCPSpatialRef();
}

const GDAL_GCP* EOPFZarrDatasetPerf::GetGCPs()
{
    return mInner->GetGCPs();
}

void EOPFZarrDatasetPerf::OptimizedMetadataMerge() const
{
    // Fast path for empty metadata
    char** innerMetadata = mInner->GetMetadata();
    if (!innerMetadata)
        return;

    // Count entries first for efficient allocation
    int count = CSLCount(innerMetadata);
    if (count == 0)
        return;

    // Pre-allocate metadata array
    EOPFZarrDatasetPerf* self = const_cast<EOPFZarrDatasetPerf*>(this);
    if (self->m_papszDefaultDomainFilteredMetadata)
        CSLDestroy(self->m_papszDefaultDomainFilteredMetadata);

    self->m_papszDefaultDomainFilteredMetadata =
        static_cast<char**>(CPLCalloc(count + 1, sizeof(char*)));

    // Fast copy with minimal string operations
    for (int i = 0; i < count; ++i)
    {
        self->m_papszDefaultDomainFilteredMetadata[i] = CPLStrdup(innerMetadata[i]);
    }
}

void EOPFZarrDatasetPerf::CacheGeotransformFromCorners(double minX,
                                                       double maxX,
                                                       double minY,
                                                       double maxY)
{
    double gt[6];
    gt[0] = minX;                          // Top-left X
    gt[1] = (maxX - minX) / nRasterXSize;  // Pixel width
    gt[2] = 0.0;                           // Rotation (usually 0)
    gt[3] = maxY;                          // Top-left Y
    gt[4] = 0.0;                           // Rotation (usually 0)
    gt[5] = (minY - maxY) / nRasterYSize;  // Pixel height (negative)

    mCache.SetCachedGeoTransform(gt);
}

bool EOPFZarrDatasetPerf::TryFastPathMetadata(const char* key, const char** outValue) const
{
    if (!key || !outValue)
        return false;

    static std::unordered_map<std::string, std::string> commonMetadata = {
        {"DRIVER", "EOPFZarr"}, {"INTERLEAVE", "PIXEL"}, {"TILED", "YES"}};

    auto it = commonMetadata.find(key);
    if (it != commonMetadata.end())
    {
        *outValue = it->second.c_str();
        return true;
    }

    return false;
}

// ============================================================================
// EOPFZarrRasterBandPerf Implementation
// ============================================================================

EOPFZarrRasterBandPerf::EOPFZarrRasterBandPerf(EOPFZarrDatasetPerf* poDS,
                                               GDALRasterBand* poUnderlyingBand,
                                               int nBand)
    : m_poUnderlyingBand(poUnderlyingBand), m_poDS(poDS)
{
    this->poDS = poDS;
    this->nBand = nBand;

    // Copy band characteristics
    eDataType = poUnderlyingBand->GetRasterDataType();
    nRasterXSize = poUnderlyingBand->GetXSize();
    nRasterYSize = poUnderlyingBand->GetYSize();

    // Copy block size
    int nBlockX, nBlockY;
    poUnderlyingBand->GetBlockSize(&nBlockX, &nBlockY);
    this->nBlockXSize = nBlockX;
    this->nBlockYSize = nBlockY;
}

EOPFZarrRasterBandPerf::~EOPFZarrRasterBandPerf()
{
    // No need to delete m_poUnderlyingBand - managed by inner dataset
}

CPLErr EOPFZarrRasterBandPerf::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    EOPF_PERF_TIMER("EOPFZarrRasterBandPerf::IReadBlock");

    // Track block access for pattern optimization
    TrackBlockAccess(nBlockXOff, nBlockYOff);

    // Prefetch adjacent blocks if access pattern suggests it
    if (ShouldPrefetchAdjacentBlocks(nBlockXOff, nBlockYOff))
    {
        PrefetchAdjacentBlocks(nBlockXOff, nBlockYOff);
    }

    return m_poUnderlyingBand->ReadBlock(nBlockXOff, nBlockYOff, pImage);
}

void EOPFZarrRasterBandPerf::TrackBlockAccess(int nBlockXOff, int nBlockYOff) const
{
    auto key = std::make_pair(nBlockXOff, nBlockYOff);
    auto now = std::chrono::steady_clock::now();

    // Limit cache size to prevent unbounded growth
    if (mBlockAccessTimes.size() >= MAX_BLOCK_CACHE_SIZE)
    {
        // Remove oldest entry
        auto oldest =
            std::min_element(mBlockAccessTimes.begin(),
                             mBlockAccessTimes.end(),
                             [](const auto& a, const auto& b) { return a.second < b.second; });
        mBlockAccessTimes.erase(oldest);
    }

    mBlockAccessTimes[key] = now;
}

bool EOPFZarrRasterBandPerf::ShouldPrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff) const
{
    // Simple heuristic: if we've accessed 3+ blocks in the last second, prefetch
    auto now = std::chrono::steady_clock::now();
    auto oneSecondAgo = now - std::chrono::seconds(1);

    int recentAccesses = 0;
    for (const auto& access : mBlockAccessTimes)
    {
        if (access.second > oneSecondAgo)
        {
            recentAccesses++;
        }
    }

    return recentAccesses >= 3;
}

void EOPFZarrRasterBandPerf::PrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff)
{
    // Prefetch right and down blocks (common access patterns)
    int nBlocksX, nBlocksY;
    GetBlockSize(&nBlocksX, &nBlocksY);  // This gets block size, not count

    // Calculate total blocks
    int totalBlocksX = (nRasterXSize + nBlocksX - 1) / nBlocksX;
    int totalBlocksY = (nRasterYSize + nBlocksY - 1) / nBlocksY;

    // Prefetch adjacent blocks
    std::vector<std::pair<int, int>> prefetchBlocks = {
        {nBlockXOff + 1, nBlockYOff},     // Right
        {nBlockXOff, nBlockYOff + 1},     // Down
        {nBlockXOff + 1, nBlockYOff + 1}  // Diagonal
    };

    for (const auto& block : prefetchBlocks)
    {
        if (block.first < totalBlocksX && block.second < totalBlocksY)
        {
            // Prefetch by reading into a dummy buffer (GDAL will cache it)
            int blockSizeBytes = GDALGetDataTypeSizeBytes(eDataType) * nBlocksX * nBlocksY;
            void* dummyBuffer = CPLMalloc(blockSizeBytes);
            CPLErr err = m_poUnderlyingBand->ReadBlock(block.first, block.second, dummyBuffer);
            CPLFree(dummyBuffer);
            // Ignore errors in prefetching as it's just an optimization
            (void) err;
        }
    }
}
