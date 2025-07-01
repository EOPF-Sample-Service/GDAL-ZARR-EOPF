#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "eopfzarr_config.h" // Add this include
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
      m_bPamInitialized(false)
{
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

EOPFZarrDataset *EOPFZarrDataset::Create(GDALDataset *inner, GDALDriver *drv)
{
    if (!inner)
        return nullptr;

    try
    {
        std::unique_ptr<EOPFZarrDataset> ds(new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv));

        // Load metadata if this is the root dataset (no subdataset specified)
        const char *subdsPath = inner->GetMetadataItem("SUBDATASET_PATH");
        if (!subdsPath || !subdsPath[0])
        {
            ds->LoadEOPFMetadata();
        }

        // Return the dataset
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
    // Get the directory path where the dataset is stored
    std::string rootPath = mInner->GetDescription();

    // Use the EOPF AttachMetadata function to load all metadata
    EOPF::AttachMetadata(*this, rootPath);

    // After metadata is attached, make sure geospatial info is processed

    // Check for geotransform in metadata
    const char *pszGeoTransform = GetMetadataItem("geo_transform");
    if (pszGeoTransform)
    {
        CPLDebug("EOPFZARR", "Found geo_transform metadata: %s", pszGeoTransform);
        double adfGeoTransform[6] = {0};
        char **papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
        if (CSLCount(papszTokens) == 6)
        {
            for (int i = 0; i < 6; i++)
                adfGeoTransform[i] = CPLAtof(papszTokens[i]);

            // Set the geotransform
            GDALPamDataset::SetGeoTransform(adfGeoTransform);
            CPLDebug("EOPFZARR", "Set geotransform: [%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]",
                     adfGeoTransform[0], adfGeoTransform[1], adfGeoTransform[2],
                     adfGeoTransform[3], adfGeoTransform[4], adfGeoTransform[5]);
        }
        else
        {
            CPLDebug("EOPFZARR", "Invalid geo_transform format, expected 6 elements");
        }
        CSLDestroy(papszTokens);
    }

    // Check for spatial reference (projection) in metadata
    const char *pszSpatialRef = GetMetadataItem("spatial_ref");
    if (pszSpatialRef && strlen(pszSpatialRef) > 0)
    {
        CPLDebug("EOPFZARR", "Found spatial_ref metadata: %s", pszSpatialRef);

        // Create OGR spatial reference from WKT
        OGRSpatialReference oSRS;
        if (oSRS.importFromWkt(pszSpatialRef) == OGRERR_NONE)
        {
            // Set the projection
            GDALPamDataset::SetSpatialRef(&oSRS);
            CPLDebug("EOPFZARR", "Set spatial reference from WKT");
        }
        else
        {
            CPLDebug("EOPFZARR", "Failed to import WKT: %s", pszSpatialRef);
        }
    }

    // Check for EPSG code
    const char *pszEPSG = GetMetadataItem("EPSG");
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
                GDALPamDataset::SetSpatialRef(&oSRS);
                CPLDebug("EOPFZARR", "Set spatial reference from EPSG: %d", nEPSG);
            }
        }
    }

    // Check for corner coordinates that might be available in various metadata items

    // UTM coordinates
    bool hasUtmCorners = false;
    double utmMinX = 0, utmMaxX = 0, utmMinY = 0, utmMaxY = 0;

    const char *pszUtmMinX = GetMetadataItem("utm_easting_min");
    const char *pszUtmMaxX = GetMetadataItem("utm_easting_max");
    const char *pszUtmMinY = GetMetadataItem("utm_northing_min");
    const char *pszUtmMaxY = GetMetadataItem("utm_northing_max");

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

    const char *pszLonMin = GetMetadataItem("geospatial_lon_min");
    const char *pszLonMax = GetMetadataItem("geospatial_lon_max");
    const char *pszLatMin = GetMetadataItem("geospatial_lat_min");
    const char *pszLatMax = GetMetadataItem("geospatial_lat_max");

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
    if (!pszGeoTransform && (hasUtmCorners || hasGeographicCorners))
    {
        double adfGeoTransform[6] = {0};

        // Use UTM corners if available, otherwise use geographic corners
        double minX = hasUtmCorners ? utmMinX : lonMin;
        double maxX = hasUtmCorners ? utmMaxX : lonMax;
        double minY = hasUtmCorners ? utmMinY : latMin;
        double maxY = hasUtmCorners ? utmMaxY : latMax;

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
            CPLDebug("EOPFZARR", "Created geotransform from corner coordinates: [%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]",
                     adfGeoTransform[0], adfGeoTransform[1], adfGeoTransform[2],
                     adfGeoTransform[3], adfGeoTransform[4], adfGeoTransform[5]);
        }
    }

    // Ensure there's valid georeference info for OSGeo4W shell
    double adfGeoTransform[6];
    if (GDALPamDataset::GetGeoTransform(adfGeoTransform) != CE_None && !GetSpatialRef())
    {
        // Log that no georeference info was found
        CPLDebug("EOPFZARR", "No georeference information found in metadata");
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
    // For subdatasets, return the subdataset metadata
    if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS"))
    {
        // First try from GDALDataset - this would be the subdatasets set by EOPFOpen
        char **papszSubdatasets = GDALDataset::GetMetadata(pszDomain);
        if (papszSubdatasets && CSLCount(papszSubdatasets) > 0)
        {
            CPLDebug("EOPFZARR", "GetMetadata(SUBDATASETS): Returning %d subdataset items from base dataset",
                     CSLCount(papszSubdatasets));
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
                        if (pszValue && strstr(pszKey, "_NAME") && STARTS_WITH_CI(pszValue, "ZARR:"))
                        {
                            CPLString eopfValue("EOPFZARR:");
                            eopfValue += (pszValue + 5); // Skip "ZARR:"
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

                return mSubdatasets;
            }
        }
        // No subdatasets found
        return nullptr;
    }

    else if (pszDomain == nullptr || EQUAL(pszDomain, ""))
    {
        // If not already cached, get inner metadata and merge with our own
        if (m_papszDefaultDomainFilteredMetadata == nullptr)
        {
            // Get our metadata first
            m_papszDefaultDomainFilteredMetadata = CSLDuplicate(GDALPamDataset::GetMetadata());

            // Get metadata from inner dataset
            char **papszInnerMeta = mInner->GetMetadata();
            for (char **papszIter = papszInnerMeta; papszIter && *papszIter; ++papszIter)
            {
                char *pszKey = nullptr;
                const char *pszValue = CPLParseNameValue(*papszIter, &pszKey);
                if (pszKey && pszValue)
                {
                    m_papszDefaultDomainFilteredMetadata =
                        CSLSetNameValue(m_papszDefaultDomainFilteredMetadata, pszKey, pszValue);
                }
                CPLFree(pszKey);
            }
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

// XMLInit implementation to match base class signature
CPLErr EOPFZarrDataset::XMLInit(CPLXMLNode* psTree, const char* pszUnused) {
    return GDALPamDataset::XMLInit(psTree, pszUnused);
}

CPLXMLNode *EOPFZarrDataset::SerializeToXML(const char *pszUnused)
{
    return GDALPamDataset::SerializeToXML(pszUnused);
}

// Implementation of EOPFZarrRasterBand

EOPFZarrRasterBand::EOPFZarrRasterBand(EOPFZarrDataset* poDS, GDALRasterBand* poUnderlyingBand, int nBand)
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

#if GDAL_VERSION_NUM >= 3060000
GDALRasterBand* EOPFZarrRasterBand::RefUnderlyingRasterBand(bool /*bForceOpen*/) const {
    return m_poUnderlyingBand;
}
#else
GDALRasterBand* EOPFZarrRasterBand::RefUnderlyingRasterBand() {
    return m_poUnderlyingBand;
}
#endif
CPLErr EOPFZarrRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    // Simply delegate to the underlying band
    if (m_poUnderlyingBand)
        return m_poUnderlyingBand->ReadBlock(nBlockXOff, nBlockYOff, pImage);

    // Return failure if there's no underlying band
    CPLError(CE_Failure, CPLE_AppDefined,
        "EOPFZarrRasterBand::IReadBlock: No underlying raster band");
    return CE_Failure;
}
