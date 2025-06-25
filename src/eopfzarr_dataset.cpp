#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "eopfzarr_config.h" // Add this include
#include "cpl_json.h"
#include "cpl_vsi.h"
#include "cpl_string.h"
#include <cstring>
#include <utility>
#include <mutex> // Add this include for std::mutex

EOPFZarrDataset::EOPFZarrDataset(std::unique_ptr<GDALDataset> inner,
                                 GDALDriver *selfDrv)
    : mInner(std::move(inner)), mProjectionRef(nullptr), mCachedSpatialRef(nullptr),
      m_papszDefaultDomainFilteredMetadata(nullptr), mSubdatasets(nullptr)
{
    // Initialize geotransform with identity transform
    mGeoTransform[0] = 0.0;
    mGeoTransform[1] = 1.0;
    mGeoTransform[2] = 0.0;
    mGeoTransform[3] = 0.0;
    mGeoTransform[4] = 0.0;
    mGeoTransform[5] = 1.0;

    poDriver = selfDrv; // make GetDriver() report us
    if (mInner)
    {
        nRasterXSize = mInner->GetRasterXSize();
        nRasterYSize = mInner->GetRasterYSize();
        nBands = mInner->GetRasterCount();
        for (int i = 1; i <= nBands; ++i)
        {
            SetBand(i, mInner->GetRasterBand(i));
        }
    }
    else
    {
        nRasterXSize = 0;
        nRasterYSize = 0;
        nBands = 0;
    }
    if (mInner)
    {
        char **papszZarrSubdatasets = mInner->GetMetadata("SUBDATASETS");
        if (papszZarrSubdatasets)
        {
            int nSubdatasets = CSLCount(papszZarrSubdatasets) / 2;
            CPLDebug("EOPFZARR", "Inner dataset has %d subdatasets", nSubdatasets);
        }
        else
        {
            CPLDebug("EOPFZARR", "Inner dataset has no subdatasets");
        }
    }

    LoadEOPFMetadata();
}

EOPFZarrDataset::~EOPFZarrDataset()
{
    // Free bands memory
    for (int i = 0; i < nBands; i++)
    {
        delete papoBands[i];
    }
    nBands = 0;
    papoBands = nullptr;

    // Free projection reference
    if (mProjectionRef)
    {
        CPLFree(mProjectionRef);
        mProjectionRef = nullptr;
    }

    // Free subdatasets
    if (mSubdatasets)
    {
        CSLDestroy(mSubdatasets);
        mSubdatasets = nullptr;
    }

    // Free spatial reference (fix the logic error here)
    if (mCachedSpatialRef)  // NOT mSubdatasets again!
    {
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr;
    }

    // Free metadata
    if (m_papszDefaultDomainFilteredMetadata)
    {
        CSLDestroy(m_papszDefaultDomainFilteredMetadata);
        m_papszDefaultDomainFilteredMetadata = nullptr;
    }

    // Reset inner dataset (will trigger deletion)
    mInner.reset();
}

EOPFZarrDataset *EOPFZarrDataset::Create(GDALDataset *inner, GDALDriver *drv)
{
    if (!inner || !drv)
    {
        CPLDebug("EOPFZARR", "EOPFZarrDataset::Create received null inner data");
        if (inner)
            GDALClose(inner);
        return nullptr;
    }
    return new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv);
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    if (!mInner)
    {
        CPLDebug("EOPFZARR", "LoadEOPFMetadata: mInner is null.");
        return;
    }
    CPLDebug("EOPFZARR", "Loading EOPF metadata...");

    EOPF::AttachMetadata(*this, mInner->GetDescription());

    // Get EPSG code and spatial reference from metadata
    // const char* pszEPSG = GetMetadataItem("EPSG");
    const char *pszSpatialRef = GetMetadataItem("spatial_ref");

    if (mProjectionRef)
    {
        CPLFree(mProjectionRef);
        mProjectionRef = nullptr;
    }
    if (mCachedSpatialRef)
    {
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr;
    }
    // Store spatial reference if it exists
    if (pszSpatialRef && strlen(pszSpatialRef) > 0)
    {
        CPLDebug("EOPFZARR", "Setting projection ref: %s", pszSpatialRef);
        mProjectionRef = CPLStrdup(pszSpatialRef);
    }
    else
    {
        // Set default WGS84 if no spatial reference found
        const char *wkt = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]]";
        mProjectionRef = CPLStrdup(wkt);
        CPLDebug("EOPFZARR", "Set default WGS84 projection");
    }

    // Get geotransform from metadata
    const char *pszGeoTransform = GetMetadataItem("geo_transform");
    if (pszGeoTransform)
    {
        CPLDebug("EOPFZARR", "Parsing geo_transform: %s", pszGeoTransform);
        char **papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
        if (CSLCount(papszTokens) == 6)
        {
            for (int i = 0; i < 6; i++)
                mGeoTransform[i] = CPLAtof(papszTokens[i]);

            CPLDebug("EOPFZARR", "Parsed geotransform: [%f,%f,%f,%f,%f,%f]",
                     mGeoTransform[0], mGeoTransform[1], mGeoTransform[2],
                     mGeoTransform[3], mGeoTransform[4], mGeoTransform[5]);
        }
        else
        {
            CPLDebug("EOPFZARR", "Failed to parse geo_transform: expected 6 tokens, got %d. Using default.", CSLCount(papszTokens));
        }
        CSLDestroy(papszTokens);
    }
    else
    {
        // Keep default geotransform initialized in constructor
        CPLDebug("EOPFZARR", "No geotransform metadata found, using default");
    }
}

CPLErr EOPFZarrDataset::GetGeoTransform(double *padfTransform)
{
    if (!padfTransform)
    {
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
    std::lock_guard<std::mutex> lock(mSpatialRefMutex);
    
    // Return cached value if available
    if (mCachedSpatialRef)
    {
        return mCachedSpatialRef;
    }

    if (!mProjectionRef || strlen(mProjectionRef) == 0)
    {
        CPLDebug("EOPFZARR", "GetSpatialRef: No projection WKT (mProjectionRef) available.");
        return nullptr;
    }

    // mCachedSpatialRef is mutable, so we can allocate and set it in a const method.
    mCachedSpatialRef = new OGRSpatialReference();
    if (mCachedSpatialRef->importFromWkt(mProjectionRef) != OGRERR_NONE)
    {
        CPLDebug("EOPFZARR", "GetSpatialRef: Failed to import WKT: %s", mProjectionRef);
        delete mCachedSpatialRef;
        mCachedSpatialRef = nullptr; // Set to null on failure
        return nullptr;
    }

    return mCachedSpatialRef;
}

char** EOPFZarrDataset::GetMetadata(const char* pszDomain)
{
    // Add mutex protection for this method
    static std::mutex metadataMutex;
    std::lock_guard<std::mutex> lock(metadataMutex);
    
    // For subdatasets, return the subdataset metadata
    if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS"))
    {
        // First try from GDALDataset - this would be the subdatasets set by EOPFOpen
        char** papszSubdatasets = GDALDataset::GetMetadata(pszDomain);
        if (papszSubdatasets && CSLCount(papszSubdatasets) > 0)
        {
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

                int nSubDSCount = 0;

                // Create new list with updated format
                for (int i = 0; papszSubdatasets[i]; i++)
                {
                    if (strstr(papszSubdatasets[i], "_NAME=") == nullptr) {
                        continue;  // Skip DESC entries
                    }

                    char* pszKey = nullptr;
                    const char* pszValue = CPLParseNameValue(papszSubdatasets[i], &pszKey);

                    if (pszKey && pszValue)
                    {
                        std::string nameKey = CPLString().Printf("SUBDATASET_%d_NAME", nSubDSCount + 1);
                        std::string descKey = CPLString().Printf("SUBDATASET_%d_DESC", nSubDSCount + 1);

                        // Convert ZARR: paths to EOPFZARR:
                        std::string dsValue = pszValue;
                        if (STARTS_WITH_CI(dsValue.c_str(), "ZARR:"))
                        {
                            // Take the path part that follows ZARR: prefix
                            std::string pathWithoutPrefix = dsValue.substr(5);

                            // Create an EOPFZARR path that QGIS can understand
                            std::string eopfValue = "EOPFZARR:" + pathWithoutPrefix;

                            // Set the subdataset name with our prefix
                            mSubdatasets = CSLSetNameValue(mSubdatasets, nameKey.c_str(), eopfValue.c_str());

                            // Get the matching description if available
                            for (int j = 0; papszSubdatasets[j]; j++)
                            {
                                char* pszDescKey = nullptr;
                                const char* pszDescValue = CPLParseNameValue(papszSubdatasets[j], &pszDescKey);
                                if (pszDescKey && pszDescValue &&
                                    EQUAL(pszDescKey, CPLString().Printf("SUBDATASET_%d_DESC", (i / 2) + 1).c_str()))
                                {
                                    mSubdatasets = CSLSetNameValue(mSubdatasets, descKey.c_str(), pszDescValue);
                                    CPLFree(pszDescKey);
                                    break;
                                }
                                CPLFree(pszDescKey);
                            }

                            nSubDSCount++;
                        }
                    }
                    CPLFree(pszKey);
                }
                return mSubdatasets;
            }
        }
        // No subdatasets found
        return nullptr;
    }

    // For other domains, handle as before
    return GDALDataset::GetMetadata(pszDomain);
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