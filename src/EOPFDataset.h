// src/EOPFDataset.h
#ifndef EOPFDATASET_H
#define EOPFDATASET_H

#include "gdal_pam.h"

class EOPFRasterBand;

class EOPFDataset final : public GDALPamDataset
{
public:
    static int Identify(struct GDALOpenInfo* poOpenInfo);
    static GDALDataset* Open(struct GDALOpenInfo* poOpenInfo);

    EOPFDataset();
    ~EOPFDataset() override;

    // For skeleton, might store a file pointer or nothing
};

#endif
