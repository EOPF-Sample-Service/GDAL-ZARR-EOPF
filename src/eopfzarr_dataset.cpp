#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "eopfzarr_config.h" // Add this include
#include "eopfzarr_performance.h" // Add performance optimizations
#include "cpl_json.h"
#include "cpl_vsi.h"
#include "cpl_string.h"
#include <cstring>
#include <utility>

EOPFZarrDataset::EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver *selfDrv)
    : mInner(std::move(inner)),
      mSubdatasets(nullptr),
      mCachedSpatialRef(nullptr),
      m_papszDefaultDomainFilteredMetadata(nullptr),
      m_bPamInitialized(false),
      mMetadataLoaded(false),
      mGeospatialInfoProcessed(false)
{
    EOPF_PERF_TIMER("EOPFZarrDataset::Constructor");
    
    SetDescription(mInner->GetDescription());
    this->poDriver = selfDrv;

    nRasterXSize = mInner->GetRasterXSize();
    nRasterYSize = mInner->GetRasterYSize();

    // Transfer bands from inner dataset
    for (int i = 1; i <= mInner->GetRasterCount(); i++)
    {
        GDALRasterBand *poBand = mInner->GetRasterBand(i);
        if (poBand)
        {
            // Create a proxy band that references the inner band
            SetBand(i, new EOPFZarrRasterBand(this, poBand, i));

            // Copy band metadata and other properties if needed
            GetRasterBand(i)->SetDescription(poBand->GetDescription());

            // You might need to copy other band properties like color interpretation,
            // no data value, color table, etc., depending on your requirements
        }
    }

    // Initialize the PAM info
    TryLoadXML();
    m_bPamInitialized = true;
}

EOPFZarrDataset::~EOPFZarrDataset()
{

    if (mSubdatasets)
        CSLDestroy(mSubdatasets);

    if (mCachedSpatialRef)
    {
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr;
    }

    if (m_papszDefaultDomainFilteredMetadata)
    {
        CSLDestroy(m_papszDefaultDomainFilteredMetadata);
        m_papszDefaultDomainFilteredMetadata = nullptr;
    }
}

static std::string ExtractRootPath(const std::string &description)
{
    if (description.find("ZARR:\"") == 0)
    {
        size_t start = 6; // After "ZARR:\""
        size_t end = description.find("\":", start);
        if (end != std::string::npos)
        {
            // Subdataset case, e.g., "ZARR:\"/vsicurl/https://...\":subds_path"
            return description.substr(start, end - start);
        }
        else
        {
            // Root dataset case, e.g., "ZARR:\"/vsicurl/https://...\""
            size_t endQuote = description.find('\"', start);
            if (endQuote != std::string::npos)
            {
                return description.substr(start, endQuote - start);
            }
            else
            {
                return description.substr(start);
            }
        }
    }
    else
    {
        // Fallback: return description as is if not in ZARR:"..." format
        return description;
    }
}

EOPFZarrDataset *EOPFZarrDataset::Create(GDALDataset *inner, GDALDriver *drv)
{
    if (!inner)
        return nullptr;

    try
    {
        std::unique_ptr<EOPFZarrDataset> ds(new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv));
        ds->LoadEOPFMetadata(); // Always load metadata
        return ds.release();
    }
    catch (const std::exception &e)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Error creating EOPF wrapper: %s", e.what());
        return nullptr;
    }
    catch (...)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Unknown error creating EOPF wrapper");
        return nullptr;
    }
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    EOPF_PERF_TIMER("EOPFZarrDataset::LoadEOPFMetadata");
    
    if (mMetadataLoaded)
        return; // Already loaded
    
    // Get the directory path where the dataset is stored
    std::string description = mInner->GetDescription();
    std::string rootPath = ExtractRootPath(description);
    
    // Use the EOPF AttachMetadata function to load all metadata
    EOPF::AttachMetadata(*this, rootPath);
    
    mMetadataLoaded = true;
    
    // Process geospatial info lazily
    LoadGeospatialInfo();
}

void EOPFZarrDataset::LoadGeospatialInfo() const
{
    EOPF_PERF_TIMER("EOPFZarrDataset::LoadGeospatialInfo");
    
    if (mGeospatialInfoProcessed)
        return;
    
    // Check cache first
    double cachedTransform[6];
    if (mCache.GetCachedGeoTransform(cachedTransform))
    {
        const_cast<EOPFZarrDataset*>(this)->GDALPamDataset::SetGeoTransform(cachedTransform);
        mGeospatialInfoProcessed = true;
        return;
    }
    
    const OGRSpatialReference* cachedSRS = mCache.GetCachedSpatialRef();
    if (cachedSRS)
    {
        const_cast<EOPFZarrDataset*>(this)->GDALPamDataset::SetSpatialRef(cachedSRS);
        mGeospatialInfoProcessed = true;
        return;
    }

    // After metadata is attached, make sure geospatial info is processed

    // Check for geotransform in metadata (with fast path)
    const char *pszGeoTransform = nullptr;
    if (!TryFastPathMetadata("geo_transform", &pszGeoTransform))
    {
        pszGeoTransform = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("geo_transform");
    }
    
    if (pszGeoTransform)
    {
        CPLDebug("EOPFZARR", "Found geo_transform metadata: %s", pszGeoTransform);
        double adfGeoTransform[6] = {0};
        
        // Use optimized tokenization
        auto tokens = EOPFPerformanceUtils::FastTokenize(pszGeoTransform, ',');
        if (tokens.size() == 6)
        {
            for (int i = 0; i < 6; i++)
                adfGeoTransform[i] = CPLAtof(tokens[i].c_str());
            
            const_cast<EOPFZarrDataset*>(this)->GDALPamDataset::SetGeoTransform(adfGeoTransform);
            mCache.SetCachedGeoTransform(adfGeoTransform);
            
            CPLDebug("EOPFZARR", "Set geotransform: [%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]",
                     adfGeoTransform[0], adfGeoTransform[1], adfGeoTransform[2],
                     adfGeoTransform[3], adfGeoTransform[4], adfGeoTransform[5]);
        }
        else
        {
            CPLDebug("EOPFZARR", "Invalid geo_transform format, expected 6 elements");
        }
    }

    // Check for spatial reference (projection) in metadata (with fast path)
    const char *pszSpatialRef = nullptr;
    if (!TryFastPathMetadata("spatial_ref", &pszSpatialRef))
    {
        pszSpatialRef = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("spatial_ref");
    }
    
    if (pszSpatialRef && strlen(pszSpatialRef) > 0)
    {
        CPLDebug("EOPFZARR", "Found spatial_ref metadata: %s", pszSpatialRef);

        // Create OGR spatial reference from WKT
        OGRSpatialReference oSRS;
        if (oSRS.importFromWkt(pszSpatialRef) == OGRERR_NONE)
        {
            // Set the projection
            const_cast<EOPFZarrDataset*>(this)->GDALPamDataset::SetSpatialRef(&oSRS);
            mCache.SetCachedSpatialRef(&oSRS);
            CPLDebug("EOPFZARR", "Set spatial reference from WKT");
        }
        else
        {
            CPLDebug("EOPFZARR", "Failed to import WKT: %s", pszSpatialRef);
        }
    }

    // Check for EPSG code (with fast path)
    const char *pszEPSG = nullptr;
    if (!TryFastPathMetadata("EPSG", &pszEPSG))
    {
        pszEPSG = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("EPSG");
    }
    
    if (pszEPSG && strlen(pszEPSG) > 0 && !GetSpatialRef())
    {
        CPLDebug("EOPFZARR", "Found EPSG metadata: %s", pszEPSG);
        int nEPSG = atoi(pszEPSG);
        if (nEPSG > 0)
        {
            OGRSpatialReference oSRS;
            if (oSRS.importFromEPSG(nEPSG) == OGRERR_NONE)
            {
                // Set the projection
                const_cast<EOPFZarrDataset*>(this)->GDALPamDataset::SetSpatialRef(&oSRS);
                mCache.SetCachedSpatialRef(&oSRS);
                CPLDebug("EOPFZARR", "Set spatial reference from EPSG: %d", nEPSG);
            }
        }
    }

    // Process corner coordinates with caching
    ProcessCornerCoordinates();
    
    mGeospatialInfoProcessed = true;
}

void EOPFZarrDataset::ProcessCornerCoordinates() const
{
    // Check for corner coordinates that might be available in various metadata items

    // UTM coordinates
    bool hasUtmCorners = false;
    double utmMinX = 0, utmMaxX = 0, utmMinY = 0, utmMaxY = 0;

    // Use fast path metadata access
    const char *pszUtmMinX, *pszUtmMaxX, *pszUtmMinY, *pszUtmMaxY;
    bool fastPath = TryFastPathMetadata("utm_easting_min", &pszUtmMinX) &&
                   TryFastPathMetadata("utm_easting_max", &pszUtmMaxX) &&
                   TryFastPathMetadata("utm_northing_min", &pszUtmMinY) &&
                   TryFastPathMetadata("utm_northing_max", &pszUtmMaxY);
    
    if (!fastPath)
    {
        pszUtmMinX = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("utm_easting_min");
        pszUtmMaxX = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("utm_easting_max");
        pszUtmMinY = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("utm_northing_min");
        pszUtmMaxY = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("utm_northing_max");
    }

    if (pszUtmMinX && pszUtmMaxX && pszUtmMinY && pszUtmMaxY)
    {
        utmMinX = CPLAtof(pszUtmMinX);
        utmMaxX = CPLAtof(pszUtmMaxX);
        utmMinY = CPLAtof(pszUtmMinY);
        utmMaxY = CPLAtof(pszUtmMaxY);
        hasUtmCorners = true;

        CPLDebug("EOPFZARR", "Found UTM corners: MinX=%.2f, MaxX=%.2f, MinY=%.2f, MaxY=%.2f",
                 utmMinX, utmMaxX, utmMinY, utmMaxY);
    }

    // Geographic coordinates
    bool hasGeographicCorners = false;
    double lonMin = 0, lonMax = 0, latMin = 0, latMax = 0;

    // Use fast path metadata access
    const char *pszLonMin, *pszLonMax, *pszLatMin, *pszLatMax;
    fastPath = TryFastPathMetadata("geospatial_lon_min", &pszLonMin) &&
              TryFastPathMetadata("geospatial_lon_max", &pszLonMax) &&
              TryFastPathMetadata("geospatial_lat_min", &pszLatMin) &&
              TryFastPathMetadata("geospatial_lat_max", &pszLatMax);
              
    if (!fastPath)
    {
        pszLonMin = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("geospatial_lon_min");
        pszLonMax = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("geospatial_lon_max");
        pszLatMin = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("geospatial_lat_min");
        pszLatMax = const_cast<EOPFZarrDataset*>(this)->GetMetadataItem("geospatial_lat_max");
    }

    if (pszLonMin && pszLonMax && pszLatMin && pszLatMax)
    {
        lonMin = CPLAtof(pszLonMin);
        lonMax = CPLAtof(pszLonMax);
        latMin = CPLAtof(pszLatMin);
        latMax = CPLAtof(pszLatMax);
        hasGeographicCorners = true;

        CPLDebug("EOPFZARR", "Found geographic corners: LonMin=%.6f, LonMax=%.6f, LatMin=%.6f, LatMax=%.6f",
                 lonMin, lonMax, latMin, latMax);
    }

    // If we have corner coordinates but no geotransform, calculate one
    double existingTransform[6];
    if (const_cast<EOPFZarrDataset*>(this)->GDALPamDataset::GetGeoTransform(existingTransform) != CE_None && 
        (hasUtmCorners || hasGeographicCorners))
    {
        const_cast<EOPFZarrDataset*>(this)->CacheGeotransformFromCorners(
            hasUtmCorners ? utmMinX : lonMin,
            hasUtmCorners ? utmMaxX : lonMax,
            hasUtmCorners ? utmMinY : latMin,
            hasUtmCorners ? utmMaxY : latMax
        );
    }
}

CPLErr EOPFZarrDataset::GetGeoTransform(double *padfTransform)
{
    CPLErr eErr = GDALPamDataset::GetGeoTransform(padfTransform);
    if (eErr == CE_None)
        return eErr;

    // Fall back to inner dataset
    return mInner->GetGeoTransform(padfTransform);
}

CPLErr EOPFZarrDataset::SetSpatialRef(const OGRSpatialReference *poSRS)
{
    CPLDebug("EOPFZARR", "SetSpatialRef called, but EOPFZarrDataset is read-only for SRS. Ignored.");
    return CE_None;
}

CPLErr EOPFZarrDataset::SetGeoTransform(double *padfTransform)
{
    CPLDebug("EOPFZARR", "SetGeoTransform called, but EOPFZarrDataset is read-only for GeoTransform. Ignored.");
    return CE_None; // Or CE_Failure.
}

const OGRSpatialReference *EOPFZarrDataset::GetSpatialRef() const
{
    // First check PAM
    const OGRSpatialReference *poSRS = GDALPamDataset::GetSpatialRef();
    if (poSRS)
        return poSRS;

    // Fall back to inner dataset
    return mInner->GetSpatialRef();
}

char **EOPFZarrDataset::GetMetadata(const char *pszDomain)
{
    EOPF_PERF_TIMER("EOPFZarrDataset::GetMetadata");
    
    // For subdatasets, return the subdataset metadata
    if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS"))
    {
        // Check cache first
        char** cached = mCache.GetCachedSubdatasets();
        if (cached)
        {
            return cached;
        }
        
        // First try from GDALDataset - this would be the subdatasets set by EOPFOpen
        char **papszSubdatasets = GDALDataset::GetMetadata(pszDomain);
        if (papszSubdatasets && CSLCount(papszSubdatasets) > 0)
        {
            CPLDebug("EOPFZARR", "GetMetadata(SUBDATASETS): Returning %d subdataset items from base dataset",
                     CSLCount(papszSubdatasets));
            mCache.SetCachedSubdatasets(papszSubdatasets);
            return papszSubdatasets;
        }

        // If not found in base dataset, try the inner dataset but with modified formats
        if (mInner)
        {
            papszSubdatasets = mInner->GetMetadata("SUBDATASETS");
            if (papszSubdatasets && CSLCount(papszSubdatasets) > 0)
            {
                // Clean up any previous subdatasets
                if (mSubdatasets)
                {
                    CSLDestroy(mSubdatasets);
                    mSubdatasets = nullptr;
                }

                // Create new list with updated format
                for (int i = 0; papszSubdatasets[i] != nullptr; i++)
                {
                    char *pszKey = nullptr;
                    const char *pszValue = CPLParseNameValue(papszSubdatasets[i], &pszKey);

                    if (pszKey && pszValue)
                    {
                        // If this is a NAME field, convert ZARR: to EOPFZARR:
                        if (strstr(pszKey, "_NAME") && STARTS_WITH_CI(pszValue, "ZARR:"))
                        {
                            CPLString eopfValue("EOPFZARR:");
                            std::string zarrPath = pszValue + 5; // Skip "ZARR:"
                            
                            // Check if this is a URL path that needs special handling
                            if (zarrPath.find("/vsicurl/http") != std::string::npos)
                            {
                                // For URLs, we need to combine the quoted path with subdataset path
                                // Format: ZARR:"/vsicurl/https://...file.zarr":/subdataset/path
                                // Should become: EOPFZARR:"/vsicurl/https://...file.zarr/subdataset/path"
                                
                                size_t firstQuote = zarrPath.find('\"');
                                size_t secondQuote = zarrPath.find('\"', firstQuote + 1);
                                size_t colonAfterQuote = zarrPath.find(':', secondQuote + 1);
                                
                                if (firstQuote != std::string::npos && secondQuote != std::string::npos && 
                                    colonAfterQuote != std::string::npos && colonAfterQuote == secondQuote + 1)
                                {
                                    // Extract the quoted URL and the subdataset path
                                    std::string quotedUrl = zarrPath.substr(firstQuote, secondQuote - firstQuote + 1);
                                    std::string subdatasetPath = zarrPath.substr(colonAfterQuote + 1);
                                    
                                    // Remove leading slash from subdataset path if present
                                    if (!subdatasetPath.empty() && subdatasetPath[0] == '/')
                                    {
                                        subdatasetPath = subdatasetPath.substr(1);
                                    }
                                    
                                    // Remove the closing quote, add subdataset path, then add closing quote
                                    quotedUrl.pop_back(); // Remove closing quote
                                    quotedUrl += "/" + subdatasetPath + "\"";
                                    
                                    eopfValue += quotedUrl;
                                }
                                else
                                {
                                    // Fallback to original format if parsing fails
                                    eopfValue += zarrPath;
                                }
                            }
                            else
                            {
                                // For local files, keep the original format
                                eopfValue += zarrPath;
                            }
                            
                            mSubdatasets = CSLSetNameValue(mSubdatasets, pszKey, eopfValue);
                        }
                        else
                        {
                            // For DESC fields, keep as is
                            mSubdatasets = CSLSetNameValue(mSubdatasets, pszKey, pszValue);
                        }
                    }
                    CPLFree(pszKey);
                }

                mCache.SetCachedSubdatasets(mSubdatasets);
                return mSubdatasets;
            }
        }
        // No subdatasets found
        return nullptr;
    }

    else if (pszDomain == nullptr || EQUAL(pszDomain, ""))
    {
        // Use optimized metadata merging
        if (m_papszDefaultDomainFilteredMetadata == nullptr)
        {
            OptimizedMetadataMerge();
        }
        return m_papszDefaultDomainFilteredMetadata;
    }

    else
    {
        // First try PAM
        char **papszMD = GDALPamDataset::GetMetadata(pszDomain);
        if (papszMD && CSLCount(papszMD) > 0)
            return papszMD;

        // Fall back to inner dataset
        if (mInner)
            return mInner->GetMetadata(pszDomain);
    }

    // For other domains, handle as before
    return GDALPamDataset::GetMetadata(pszDomain);
}

char **EOPFZarrDataset::GetFileList()
{
    // First check if the inner dataset has a file list
    if (mInner)
    {
        char **papszFileList = mInner->GetFileList();
        if (papszFileList && CSLCount(papszFileList) > 0)
        {
            // Return the inner dataset's file list
            return papszFileList;
        }
    }

    return GDALDataset::GetFileList();
}

const char *EOPFZarrDataset::GetDescription() const
{
    return mInner->GetDescription();
}

int EOPFZarrDataset::CloseDependentDatasets()
{
    if (!mInner)
        return FALSE;

    mInner.reset();
    return TRUE;
}

int EOPFZarrDataset::GetGCPCount()
{
    return mInner->GetGCPCount();
}

const OGRSpatialReference *EOPFZarrDataset::GetGCPSpatialRef() const
{
    return mInner->GetGCPSpatialRef();
}

const GDAL_GCP *EOPFZarrDataset::GetGCPs()
{
    return mInner->GetGCPs();
}

CPLErr EOPFZarrDataset::TryLoadXML(char **papszSiblingFiles)
{
    if (!m_bPamInitialized)
        return CE_None;

    return GDALPamDataset::TryLoadXML(papszSiblingFiles);
}

// Update the implementation:

// XMLInit - using modern GDAL signature (const CPLXMLNode *)
CPLErr EOPFZarrDataset::XMLInit(const CPLXMLNode *psTree, const char *pszUnused)
{
    return GDALPamDataset::XMLInit(psTree, pszUnused);
}

CPLXMLNode *EOPFZarrDataset::SerializeToXML(const char *pszUnused)
{
    return GDALPamDataset::SerializeToXML(pszUnused);
}

// Implementation of EOPFZarrRasterBand

EOPFZarrRasterBand::EOPFZarrRasterBand(EOPFZarrDataset *poDS, GDALRasterBand *poUnderlyingBand, int nBand)
    : m_poUnderlyingBand(poUnderlyingBand),
      m_poDS(poDS)
{
    // Initialize GDALRasterBand base class properties directly
    this->poDS = poDS;
    this->nBand = nBand;
    this->eDataType = poUnderlyingBand->GetRasterDataType();
    poUnderlyingBand->GetBlockSize(&this->nBlockXSize, &this->nBlockYSize);

    // Copy other properties from underlying band
    this->eAccess = poUnderlyingBand->GetAccess();
}

EOPFZarrRasterBand::~EOPFZarrRasterBand()
{
    // No need to delete m_poUnderlyingBand as it's owned by the inner dataset
}

// RefUnderlyingRasterBand - signature is consistent in modern GDAL
GDALRasterBand *EOPFZarrRasterBand::RefUnderlyingRasterBand(bool /*bForceOpen*/) const
{
    return m_poUnderlyingBand;
}
CPLErr EOPFZarrRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void *pImage)
{
    EOPF_PERF_TIMER("EOPFZarrRasterBand::IReadBlock");
    
    // Track block access patterns for potential prefetching
    TrackBlockAccess(nBlockXOff, nBlockYOff);
    
    // Simply delegate to the underlying band
    if (m_poUnderlyingBand)
    {
        CPLErr result = m_poUnderlyingBand->ReadBlock(nBlockXOff, nBlockYOff, pImage);
        
        // Consider prefetching adjacent blocks if access pattern suggests it
        if (result == CE_None && ShouldPrefetchAdjacentBlocks(nBlockXOff, nBlockYOff))
        {
            PrefetchAdjacentBlocks(nBlockXOff, nBlockYOff);
        }
        
        return result;
    }

    // Return failure if there's no underlying band
    CPLError(CE_Failure, CPLE_AppDefined,
             "EOPFZarrRasterBand::IReadBlock: No underlying raster band");
    return CE_Failure;
}

// Performance optimization helper methods

void EOPFZarrDataset::OptimizedMetadataMerge() const
{
    EOPF_PERF_TIMER("EOPFZarrDataset::OptimizedMetadataMerge");
    
    // Check if already cached
    char** cached = mCache.GetCachedMetadata();
    if (cached)
    {
        m_papszDefaultDomainFilteredMetadata = cached;
        return;
    }
    
    // Get our metadata first using const_cast for GDAL method call
    EOPFZarrDataset* self = const_cast<EOPFZarrDataset*>(this);
    m_papszDefaultDomainFilteredMetadata = EOPFPerformanceUtils::OptimizedCSLDuplicate(
        self->GDALPamDataset::GetMetadata()
    );

    // Get metadata from inner dataset
    char **papszInnerMeta = mInner->GetMetadata();
    for (char **papszIter = papszInnerMeta; papszIter && *papszIter; ++papszIter)
    {
        char *pszKey = nullptr;
        const char *pszValue = CPLParseNameValue(*papszIter, &pszKey);
        if (pszKey && pszValue)
        {
            m_papszDefaultDomainFilteredMetadata =
                EOPFPerformanceUtils::OptimizedCSLSetNameValue(
                    m_papszDefaultDomainFilteredMetadata, pszKey, pszValue);
        }
        CPLFree(pszKey);
    }
    
    // Cache the result
    mCache.SetCachedMetadata(m_papszDefaultDomainFilteredMetadata);
}

void EOPFZarrDataset::CacheGeotransformFromCorners(double minX, double maxX, double minY, double maxY)
{
    double adfGeoTransform[6] = {0};

    int width = GetRasterXSize();
    int height = GetRasterYSize();

    if (width > 0 && height > 0)
    {
        adfGeoTransform[0] = minX;                               // top left x
        adfGeoTransform[1] = (maxX - minX) / width;              // w-e pixel resolution
        adfGeoTransform[2] = 0.0;                                // rotation, 0 if image is "north up"
        adfGeoTransform[3] = maxY;                               // top left y
        adfGeoTransform[4] = 0.0;                                // rotation, 0 if image is "north up"
        adfGeoTransform[5] = -std::fabs((maxY - minY) / height); // n-s pixel resolution (negative value)

        GDALPamDataset::SetGeoTransform(adfGeoTransform);
        mCache.SetCachedGeoTransform(adfGeoTransform);
        
        CPLDebug("EOPFZARR", "Created geotransform from corner coordinates: [%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]",
                 adfGeoTransform[0], adfGeoTransform[1], adfGeoTransform[2],
                 adfGeoTransform[3], adfGeoTransform[4], adfGeoTransform[5]);
    }
}

bool EOPFZarrDataset::TryFastPathMetadata(const char* key, const char** outValue) const
{
    // Try cache first
    const char* cached = mCache.GetCachedMetadataItem(key);
    if (cached)
    {
        *outValue = cached;
        return true;
    }
    return false;
}

// Performance optimization methods for RasterBand

void EOPFZarrRasterBand::TrackBlockAccess(int nBlockXOff, int nBlockYOff) const
{
    // Only track if we haven't exceeded cache size
    if (mBlockAccessTimes.size() < MAX_BLOCK_CACHE_SIZE)
    {
        auto key = std::make_pair(nBlockXOff, nBlockYOff);
        mBlockAccessTimes[key] = std::chrono::steady_clock::now();
    }
}

bool EOPFZarrRasterBand::ShouldPrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff) const
{
    // Simple heuristic: if we've accessed this area recently, prefetch adjacent blocks
    // This is especially useful for sequential reading patterns
    
    // Check if we've accessed nearby blocks recently
    auto now = std::chrono::steady_clock::now();
    const auto threshold = std::chrono::seconds(1); // 1 second threshold
    
    int adjacentCount = 0;
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0) continue; // Skip center block
            
            auto key = std::make_pair(nBlockXOff + dx, nBlockYOff + dy);
            auto it = mBlockAccessTimes.find(key);
            if (it != mBlockAccessTimes.end() && (now - it->second) < threshold)
            {
                adjacentCount++;
            }
        }
    }
    
    // If we've accessed 2 or more adjacent blocks recently, enable prefetching
    return adjacentCount >= 2;
}

void EOPFZarrRasterBand::PrefetchAdjacentBlocks(int nBlockXOff, int nBlockYOff)
{
    // This is a placeholder for prefetching logic
    // In a full implementation, you might:
    // 1. Check which adjacent blocks aren't in GDAL's cache
    // 2. Use a background thread to read them
    // 3. Prime GDAL's block cache
    
    // For now, just log the intent
    CPLDebug("EOPFZARR_PERF", "Would prefetch blocks around (%d,%d)", nBlockXOff, nBlockYOff);
}
