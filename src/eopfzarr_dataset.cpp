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

    // Initialize the PAM info
    TryLoadXML();
    m_bPamInitialized = true;
}

EOPFZarrDataset::~EOPFZarrDataset()
{
    if (mProjectionRef)
        CPLFree(mProjectionRef);

    if (mSubdatasets)
        CSLDestroy(mSubdatasets);

    if (mSubdatasets)
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
    // In LoadEOPFMetadata
    const char *pszGeoTransform = GetMetadataItem("geo_transform");
    if (pszGeoTransform)
    {
        double adfGeoTransform[6] = {0};
        char **papszTokens = CSLTokenizeString2(pszGeoTransform, ",", 0);
        if (CSLCount(papszTokens) == 6)
        {
            for (int i = 0; i < 6; i++)
                adfGeoTransform[i] = CPLAtof(papszTokens[i]);

            // Use the parent class method to set the geotransform
            GDALPamDataset::SetGeoTransform(adfGeoTransform);
        }
        CSLDestroy(papszTokens);
    }

    // Similarly for projection
    const char *pszSpatialRef = GetMetadataItem("spatial_ref");
    if (pszSpatialRef && strlen(pszSpatialRef) > 0)
    {
        OGRSpatialReference oSRS;
        if (oSRS.importFromWkt(pszSpatialRef) == OGRERR_NONE)
        {
            GDALPamDataset::SetSpatialRef(&oSRS);
        }
    }
}

CPLErr EOPFZarrDataset::GetGeoTransform(double* padfTransform)
{
    return GDALPamDataset::GetGeoTransform(padfTransform);
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

const OGRSpatialReference* EOPFZarrDataset::GetSpatialRef() const
{
    return GDALPamDataset::GetSpatialRef();
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

CPLErr EOPFZarrDataset::XMLInit(const CPLXMLNode *psTree, const char *pszUnused)
{
    return GDALPamDataset::XMLInit(psTree, pszUnused);
}

CPLXMLNode *EOPFZarrDataset::SerializeToXML(const char *pszUnused)
{
    return GDALPamDataset::SerializeToXML(pszUnused);
}