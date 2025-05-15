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
        // Set default geotransform for European region
        mGeoTransform[0] = 10.0;       // Origin X
        mGeoTransform[1] = 0.01;       // Pixel width
        mGeoTransform[2] = 0.0;        // Rotation
        mGeoTransform[3] = 45.0;       // Origin Y
        mGeoTransform[4] = 0.0;        // Rotation
        mGeoTransform[5] = -0.01;      // Pixel height

        CPLDebug("EOPFZARR", "Set default geotransform: [%f,%f,%f,%f,%f,%f]",
            mGeoTransform[0], mGeoTransform[1], mGeoTransform[2],
            mGeoTransform[3], mGeoTransform[4], mGeoTransform[5]);
    }
}


const OGRSpatialReference* EOPFZarrDataset::GetSpatialRef() const
{
    if (mProjectionRef && strlen(mProjectionRef) > 0)
    {
        OGRSpatialReference* poSRS = new OGRSpatialReference();
        if (poSRS->importFromWkt(mProjectionRef) == OGRERR_NONE)
        {
            // Return the spatial reference directly without modifications
            return poSRS;
        }
        delete poSRS;
    }

    // Fall back to WGS84 if no valid projection reference found
    OGRSpatialReference* poSRS = new OGRSpatialReference();
    poSRS->SetWellKnownGeogCS("WGS84");
    poSRS->SetAuthority("GEOGCS", "EPSG", 4326);
    return poSRS;
}



CPLErr EOPFZarrDataset::GetGeoTransform(double* padfTransform)
{
    if (padfTransform)
    {
        memcpy(padfTransform, mGeoTransform, sizeof(double) * 6);
        return CE_None;
    }
    return CE_Failure;
}

char** EOPFZarrDataset::GetMetadata(const char* pszDomain)
{
    if (pszDomain != nullptr && EQUAL(pszDomain, "SUBDATASETS")) {
        // Get subdatasets from the inner Zarr dataset
        char** papszInnerSubdatasets = mInner->GetMetadata("SUBDATASETS");
        if (papszInnerSubdatasets == nullptr) {
            return nullptr;
        }

        // Clean up our previous domain if it exists
        if (mSubdatasets) {
            CSLDestroy(mSubdatasets);
            mSubdatasets = nullptr;
        }

        // Copy the subdatasets, replacing "ZARR:" with "EOPFZARR:"
        mSubdatasets = CSLDuplicate(papszInnerSubdatasets);
        for (int i = 0; mSubdatasets[i] != nullptr; i++) {
            if (strstr(mSubdatasets[i], "_NAME=ZARR:")) {
                char* pszValue = strstr(mSubdatasets[i], "=ZARR:");
                if (pszValue) {
                    pszValue[1] = 'E'; // =E...
                    pszValue[2] = 'O'; // =EO...
                    pszValue[3] = 'P'; // =EOP...
                    pszValue[4] = 'F'; // =EOPF...
                    pszValue[5] = 'Z'; // =EOPFZ...
                }
            }
        }
        return mSubdatasets;
    }

    // For other domains, delegate to the parent class
    return GDALDataset::GetMetadata(pszDomain);
}
