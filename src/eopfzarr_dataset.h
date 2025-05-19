#pragma once
#include "gdal_priv.h"
#include <memory>

namespace EOPF {
    void AttachMetadata(GDALDataset& ds, const std::string& rootPath);
}

class EOPFZarrDataset : public GDALDataset
{
private:
    std::unique_ptr<GDALDataset> mInner;
    double mGeoTransform[6];
    char* mProjectionRef;
    char** mSubdatasets;

public:
    EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDrv);
    ~EOPFZarrDataset();

    // Factory method
    static EOPFZarrDataset* Create(GDALDataset* inner, GDALDriver* drv);

    // Load metadata from Zarr file
    void LoadEOPFMetadata();

    // Override metadata method to handle subdatasets
    char** GetMetadata(const char* pszDomain = nullptr) override;
    CPLErr SetSpatialRef(const OGRSpatialReference* poSRS) override;
    CPLErr SetGeoTransform(double* padfTransform) override;
    
    // Override geospatial methods to correctly handle coordinate transformations
    const OGRSpatialReference* GetSpatialRef() const override;
    CPLErr GetGeoTransform(double* padfTransform) override;
   
};
