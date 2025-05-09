#include "eopfzarr_dataset.h"


EOPFZarrDataset::EOPFZarrDataset(std::unique_ptr<GDALDataset> inner,
    GDALDriver* selfDrv)
    : mInner(std::move(inner))
{
    
    poDriver = selfDrv;        // <-- NEW : make GetDriver() report us
    nRasterXSize = mInner->GetRasterXSize();
    nRasterYSize = mInner->GetRasterYSize();
    nBands = mInner->GetRasterCount();
    for (int i = 1; i <= nBands; ++i)
        SetBand(i, mInner->GetRasterBand(i));
}

EOPFZarrDataset* EOPFZarrDataset::Create(GDALDataset* inner, GDALDriver* drv)
{
    return new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv);
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    
}
