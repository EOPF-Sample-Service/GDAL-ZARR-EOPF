#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "cpl_json.h"
#include "cpl_vsi.h"
#include "cpl_string.h"
#include <cstring>
#include <utility>

EOPFZarrDataset::EOPFZarrDataset(std::unique_ptr<GDALDataset> inner,
    GDALDriver* selfDrv)
    : mInner(std::move(inner)), mProjectionRef(nullptr),mCachedSpatialRef(nullptr),
    m_papszDefaultDomainFilteredMetadata(nullptr), mSubdatasets(nullptr)
{
    // Initialize geotransform with identity transform
    mGeoTransform[0] = 0.0;
    mGeoTransform[1] = 1.0;
    mGeoTransform[2] = 0.0;
    mGeoTransform[3] = 0.0;
    mGeoTransform[4] = 0.0;
    mGeoTransform[5] = 1.0;

    poDriver = selfDrv;        // make GetDriver() report us
    if(mInner){
        nRasterXSize = mInner->GetRasterXSize();
        nRasterYSize = mInner->GetRasterYSize();
        nBands = mInner->GetRasterCount();
        for (int i = 1; i <= nBands; ++i) {
            SetBand(i, mInner->GetRasterBand(i));
        }
    } else {
        nRasterXSize = 0;
        nRasterYSize = 0;
        nBands = 0;
    }

    LoadEOPFMetadata();
}

EOPFZarrDataset::~EOPFZarrDataset()
{
    if (mProjectionRef)
        CPLFree(mProjectionRef);

    if (mSubdatasets)
        CSLDestroy(mSubdatasets);

    if(mSubdatasets){
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr;
    }

    if (m_papszDefaultDomainFilteredMetadata) {
        CSLDestroy(m_papszDefaultDomainFilteredMetadata);
        m_papszDefaultDomainFilteredMetadata = nullptr;
    }
		
}

EOPFZarrDataset* EOPFZarrDataset::Create(GDALDataset* inner, GDALDriver* drv)
{
    if (!inner || !drv) {
        CPLDebug("EOPFZARR", "EOPFZarrDataset::Create received null inner data");
        if (inner) GDALClose(inner);
		return nullptr;
    }
    return new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv);
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    if (!mInner) {
		CPLDebug("EOPFZARR", "LoadEOPFMetadata: mInner is null.");
        return;
    }
    CPLDebug("EOPFZARR", "Loading EOPF metadata...");
    EOPF::AttachMetadata(*this, mInner->GetDescription());

    // Get EPSG code and spatial reference from metadata
    // const char* pszEPSG = GetMetadataItem("EPSG");
    const char* pszSpatialRef = GetMetadataItem("spatial_ref");

    if (mProjectionRef) {
        CPLFree(mProjectionRef);
        mProjectionRef = nullptr;
    }
    if(mCachedSpatialRef) {
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr;
	}
    // Store spatial reference if it exists
    if (pszSpatialRef && strlen(pszSpatialRef) > 0) {
        CPLDebug("EOPFZARR", "Setting projection ref: %s", pszSpatialRef);
        mProjectionRef = CPLStrdup(pszSpatialRef);
    }
    else {
        // Set default WGS84 if no spatial reference found
        const char* wkt = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]]";
        mProjectionRef = CPLStrdup(wkt);
        CPLDebug("EOPFZARR", "Set default WGS84 projection");
    }

    // Get geotransform from metadata
    const char* pszGeoTransform = GetMetadataItem("geo_transform");
    if (pszGeoTransform) {
        CPLDebug("EOPFZARR", "Parsing geo_transform: %s", pszGeoTransform);
        char** papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
        if (CSLCount(papszTokens) == 6) {
            for (int i = 0; i < 6; i++)
                mGeoTransform[i] = CPLAtof(papszTokens[i]);

            CPLDebug("EOPFZARR", "Parsed geotransform: [%f,%f,%f,%f,%f,%f]",
                mGeoTransform[0], mGeoTransform[1], mGeoTransform[2],
                mGeoTransform[3], mGeoTransform[4], mGeoTransform[5]);
        }
        else {
            CPLDebug("EOPFZARR", "Failed to parse geo_transform: expected 6 tokens, got %d. Using default.", CSLCount(papszTokens));
        }
        CSLDestroy(papszTokens);
    }
    else {
        // Keep default geotransform initialized in constructor
        CPLDebug("EOPFZARR", "No geotransform metadata found, using default");
    }
}



CPLErr EOPFZarrDataset::GetGeoTransform(double* padfTransform)
{
    if (!padfTransform) {
		return CE_Failure;
    }
    memcpy(padfTransform, mGeoTransform, sizeof(double) * 6);
    return CE_None;
    // Option 2: Prioritize live metadata item (original behavior)
    // This could be useful if metadata items can be updated post-LoadEOPFMetadata.
    // However, it might be inconsistent if mGeoTransform is not also updated.
    // For simplicity and consistency with LoadEOPFMetadata, Option 1 is often preferred
    // unless dynamic updates to the "geo_transform" metadata item are expected to override mGeoTransform.
    // Sticking to original behavior for now:
    /*
    const char* pszGeoTransform = GetMetadataItem("geo_transform");
    if (pszGeoTransform)
    {
        char** papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
        if (CSLCount(papszTokens) == 6)
        {
            for (int i = 0; i < 6; i++)
                padfTransform[i] = CPLAtof(papszTokens[i]);
            CSLDestroy(papszTokens);
            return CE_None;
        }
        CSLDestroy(papszTokens);
        // If parsing metadata item fails, fall through to use mGeoTransform
        CPLDebug("EOPFZARR", "geo_transform metadata item found but not parseable, using internal mGeoTransform.");
    }
    memcpy(padfTransform, mGeoTransform, sizeof(double) * 6);
    return CE_None;
    */
}

CPLErr EOPFZarrDataset::SetSpatialRef(const OGRSpatialReference* poSRS)
{
    CPLDebug("EOPFZARR", "SetSpatialRef called, but EOPFZarrDataset is read-only for SRS. Ignored.");
    return CE_None;
}

CPLErr EOPFZarrDataset::SetGeoTransform(double* padfTransform)
{
    CPLDebug("EOPFZARR", "SetGeoTransform called, but EOPFZarrDataset is read-only for GeoTransform. Ignored.");
    return CE_None; // Or CE_Failure.
}

const OGRSpatialReference* EOPFZarrDataset::GetSpatialRef() const
{
    // Return nullptr to prevent coordinate system information from being displayed
    if (mCachedSpatialRef) {
        return mCachedSpatialRef;
    }

    if (!mProjectionRef || strlen(mProjectionRef) == 0) {
        CPLDebug("EOPFZARR", "GetSpatialRef: No projection WKT (mProjectionRef) available.");
        return nullptr;
    }

    // mCachedSpatialRef is mutable, so we can allocate and set it in a const method.
    mCachedSpatialRef = new OGRSpatialReference();
    if (mCachedSpatialRef->importFromWkt(mProjectionRef) != OGRERR_NONE) {
        CPLDebug("EOPFZARR", "GetSpatialRef: Failed to import WKT: %s", mProjectionRef);
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr; // Set to null on failure
        return nullptr;
    }

    // Optional: Auto-identify EPSG. Some applications might find this useful.
    // mCachedSpatialRef->AutoIdentifyEPSG();

    return mCachedSpatialRef;
}

char** EOPFZarrDataset::GetMetadata(const char* pszDomain)
{
    // For subdatasets, return the subdataset metadata
    if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS")) {
        return GDALDataset::GetMetadata(pszDomain);
    }

    if (pszDomain == nullptr) { // Default domain
        // Clear previous cache for the default domain if it exists
        if (m_papszDefaultDomainFilteredMetadata) {
            CSLDestroy(m_papszDefaultDomainFilteredMetadata);
            m_papszDefaultDomainFilteredMetadata = nullptr;
        }

        char** papszBaseMeta = GDALDataset::GetMetadata(nullptr); // Get from base (or mInner if overridden)
        char** papszFiltered = nullptr;

        for (int i = 0; papszBaseMeta && papszBaseMeta[i]; i++) {
            char* pszKey = nullptr;
            const char* pszValue = CPLParseNameValue(papszBaseMeta[i], &pszKey);

            if (pszKey && pszValue) { // Ensure both key and value are valid
                // Skip items typically related to projection/SRS if they are handled elsewhere
                // or to avoid redundancy if GetSpatialRef() is the authority.
                if (!EQUAL(pszKey, "proj:epsg") &&
                    !EQUAL(pszKey, "proj=epsg") && // A less common variant
                    !STARTS_WITH_CI(pszKey, "proj") && // General filter for "proj..." keys
                    !EQUAL(pszKey, "spatial_ref")) {
                    papszFiltered = CSLAddNameValue(papszFiltered, pszKey, pszValue);
                }
            }
            CPLFree(pszKey); // Free key allocated by CPLParseNameValue
        }

        // papszFiltered is now the cached list for this instance.
        m_papszDefaultDomainFilteredMetadata = papszFiltered;
        return m_papszDefaultDomainFilteredMetadata;
    }

    // For other domains, return the metadata from the base class
    return GDALDataset::GetMetadata(pszDomain);
}

