#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "cpl_json.h"
#include "cpl_vsi.h"
#include "cpl_string.h"
#include <cstring>

EOPFZarrDataset::EOPFZarrDataset(std::unique_ptr<GDALDataset> inner,
    GDALDriver* selfDrv)
    : mInner(std::move(inner)), mProjectionRef(nullptr)
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
}

EOPFZarrDataset* EOPFZarrDataset::Create(GDALDataset* inner, GDALDriver* drv)
{
    return new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv);
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    CPLDebug("EOPFZARR", "Loading EOPF metadata...");
    EOPF::AttachMetadata(*this, mInner->GetDescription());

    // After metadata is attached, parse and store geotransform and projection
    const char* pszGeoTransform = GetMetadataItem("geo_transform");
    if (pszGeoTransform)
    {
        CPLDebug("EOPFZARR", "Parsing geo_transform: %s", pszGeoTransform);
        char** papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
        if (CSLCount(papszTokens) == 6)
        {
            for (int i = 0; i < 6; i++)
                mGeoTransform[i] = CPLAtof(papszTokens[i]);

            CPLDebug("EOPFZARR", "Parsed geotransform: [%f,%f,%f,%f,%f,%f]",
                mGeoTransform[0], mGeoTransform[1], mGeoTransform[2],
                mGeoTransform[3], mGeoTransform[4], mGeoTransform[5]);
        }
        CSLDestroy(papszTokens);
    }

    const char* pszSpatialRef = GetMetadataItem("spatial_ref");
    if (pszSpatialRef)
    {
        CPLDebug("EOPFZARR", "Setting projection ref: %s", pszSpatialRef);
        mProjectionRef = CPLStrdup(pszSpatialRef);
    }
}

const OGRSpatialReference* EOPFZarrDataset::GetSpatialRef() const
{
    if (mProjectionRef && strlen(mProjectionRef) > 0)
    {
        OGRSpatialReference* poSRS = new OGRSpatialReference();
        if (poSRS->importFromWkt(mProjectionRef) == OGRERR_NONE)
            return poSRS;
        delete poSRS;
    }
    return nullptr;
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
