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

public:
    EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDrv);
    ~EOPFZarrDataset();

    // Factory method
    static EOPFZarrDataset* Create(GDALDataset* inner, GDALDriver* drv);

    // Load metadata from Zarr file
    void LoadEOPFMetadata();

    // Override geospatial methods to correctly handle coordinate transformations
    // In newer GDAL versions, these are the correct methods:
    const OGRSpatialReference* GetSpatialRef() const override;
    CPLErr GetGeoTransform(double* padfTransform) override;

    // Additional methods if needed (e.g., for subdatasets or other functionality)
};
