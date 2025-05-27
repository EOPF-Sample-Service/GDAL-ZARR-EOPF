#pragma once
#include "gdal_priv.h"
#include <memory>

namespace EOPF {
    enum class Mode;
    void AttachMetadata(GDALDataset& ds, const std::string& rootPath, Mode mode);
}

class EOPFZarrDataset : public GDALDataset
{
private:
    std::unique_ptr<GDALDataset> mInner;
    double mGeoTransform[6];
    char* mProjectionRef;
    char** mSubdatasets;
    mutable OGRSpatialReference* mCachedSpatialRef = nullptr;
    char** m_papszDefaultDomainFilteredMetadata = nullptr;
    EOPF::Mode m_eMode;  // Store the mode

public:
    EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDrv);
    ~EOPFZarrDataset();

    // Factory method
    static EOPFZarrDataset* Create(GDALDataset* inner, GDALDriver* drv);

    // Load metadata from Zarr file
    void LoadEOPFMetadata();
    
    // Set/Get the mode
    void SetMode(EOPF::Mode eMode) { m_eMode = eMode; }
    EOPF::Mode GetMode() const { return m_eMode; }

    // Override metadata method to handle subdatasets
    char** GetMetadata(const char* pszDomain = nullptr) override;
    CPLErr SetSpatialRef(const OGRSpatialReference* poSRS) override;
    CPLErr SetGeoTransform(double* padfTransform) override;
   
    // Override geospatial methods to correctly handle coordinate transformations
    const OGRSpatialReference* GetSpatialRef() const override;
    CPLErr GetGeoTransform(double* padfTransform) override;
};
