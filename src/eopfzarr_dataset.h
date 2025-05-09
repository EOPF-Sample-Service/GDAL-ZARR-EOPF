#pragma once
#include "gdal_priv.h"
#include <memory>

class EOPFZarrDataset final : public GDALDataset
{
    std::unique_ptr<GDALDataset> mInner;

    explicit EOPFZarrDataset(std::unique_ptr<GDALDataset> inner, GDALDriver* selfDriver);
    void LoadEOPFMetadata();          

public:
    static EOPFZarrDataset* Create(GDALDataset* inner,
        GDALDriver* selfDriver);

};
