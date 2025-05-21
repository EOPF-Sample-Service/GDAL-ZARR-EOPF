#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "cpl_json.h"
#include "cpl_vsi.h"
#include "cpl_string.h"
#include <cstring>

EOPFZarrDataset::EOPFZarrDataset(std::unique_ptr<GDALDataset> inner,
    GDALDriver* selfDrv)
    : mInner(std::move(inner)), mProjectionRef(nullptr), mSubdatasets(nullptr)
{
    // Initialize geotransform with identity transform
    mGeoTransform[0] = 0.0;
    mGeoTransform[1] = 1.0;
    mGeoTransform[2] = 0.0;
    mGeoTransform[3] = 0.0;
    mGeoTransform[4] = 0.0;
    mGeoTransform[5] = 1.0;

    poDriver = selfDrv;        // make GetDriver() report us
    nRasterXSize = mInner->GetRasterXSize();
    nRasterYSize = mInner->GetRasterYSize();
    nBands = mInner->GetRasterCount();
    for (int i = 1; i <= nBands; ++i)
        SetBand(i, mInner->GetRasterBand(i));

    LoadEOPFMetadata();
}

EOPFZarrDataset::~EOPFZarrDataset()
{
    if (mProjectionRef)
        CPLFree(mProjectionRef);

    if (mSubdatasets)
        CSLDestroy(mSubdatasets);
    if(mSubdatasets)
		delete mCachedSpatialRef;
}

EOPFZarrDataset* EOPFZarrDataset::Create(GDALDataset* inner, GDALDriver* drv)
{
    return new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv);
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    CPLDebug("EOPFZARR", "Loading EOPF metadata...");
    EOPF::AttachMetadata(*this, mInner->GetDescription());

    // Get EPSG code and spatial reference from metadata
    const char* pszEPSG = GetMetadataItem("EPSG");
    const char* pszSpatialRef = GetMetadataItem("spatial_ref");

    // Store spatial reference if it exists
    if (pszSpatialRef) {
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
        CSLDestroy(papszTokens);
    }
    else {
        // Keep default geotransform initialized in constructor
        CPLDebug("EOPFZARR", "No geotransform metadata found, using default");
    }
}



CPLErr EOPFZarrDataset::GetGeoTransform(double* padfTransform)
{
    if (padfTransform)
    {
        // Get the geo_transform metadata item
        const char* pszGeoTransform = GetMetadataItem("geo_transform");
        if (pszGeoTransform)
        {
            // Parse the comma-separated values
            char** papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
            if (CSLCount(papszTokens) == 6)
            {
                for (int i = 0; i < 6; i++)
                    padfTransform[i] = CPLAtof(papszTokens[i]);

                CSLDestroy(papszTokens);
                return CE_None;
            }
            CSLDestroy(papszTokens);
        }

        // If we don't have a valid geo_transform, use the stored geotransform
        memcpy(padfTransform, mGeoTransform, sizeof(double) * 6);
        return CE_None;
    }
    return CE_Failure;
}

CPLErr EOPFZarrDataset::SetSpatialRef(const OGRSpatialReference* poSRS)
{
    // Silently accept - do nothing

    return CE_None;
}

CPLErr EOPFZarrDataset::SetGeoTransform(double* padfTransform)
{
    // Silently accept - do nothing
    return CE_None;
}

const OGRSpatialReference* EOPFZarrDataset::GetSpatialRef() const
{
    // Return nullptr to prevent coordinate system information from being displayed
    if(!mProjectionRef)
        return nullptr;
	OGRSpatialReference* poRS = new OGRSpatialReference();
    if (poRS->importFromWkt(mProjectionRef) != OGRERR_NONE)
    {
        delete poRS;
        return nullptr;
    }
    return poRS;
}

char** EOPFZarrDataset::GetMetadata(const char* pszDomain)
{
    // For subdatasets, return the subdataset metadata
    if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS")) {
        return GDALDataset::GetMetadata("SUBDATASETS");
    }

    // For the default domain, filter out redundant coordinate information
    if (pszDomain == nullptr) {
        char** papszMeta = GDALDataset::GetMetadata();
        // Create filtered metadata list
        char** papszFiltered = nullptr;
        for (int i = 0; papszMeta && papszMeta[i]; i++) {
            char* pszKey = nullptr;
            const char* pszValue = CPLParseNameValue(papszMeta[i], &pszKey);

            // Skip proj:epsg and spatial_ref items
            if (pszKey &&
                !EQUAL(pszKey, "proj:epsg") &&
                !EQUAL(pszKey, "proj=epsg") &&
                !STARTS_WITH_CI(pszKey, "proj") &&
                !EQUAL(pszKey, "spatial_ref")) {
                papszFiltered = CSLAddNameValue(papszFiltered, pszKey, pszValue);
            }
            CPLFree(pszKey);
        }

        // Store filtered metadata for future use
        static char** papszLastFiltered = nullptr;
        if (papszLastFiltered)
            CSLDestroy(papszLastFiltered);
        papszLastFiltered = papszFiltered;

        return papszFiltered;
    }

    // For other domains, return the original metadata
    return GDALDataset::GetMetadata(pszDomain);
}

