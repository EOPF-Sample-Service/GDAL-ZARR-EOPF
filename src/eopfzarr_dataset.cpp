#include "eopfzarr_dataset.h"
#include "eopf_metadata.h"
#include "cpl_json.h"
#include "cpl_vsi.h"

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

    LoadEOPFMetadata();
}

EOPFZarrDataset* EOPFZarrDataset::Create(GDALDataset* inner, GDALDriver* drv)
{
    return new EOPFZarrDataset(std::unique_ptr<GDALDataset>(inner), drv);
}

void EOPFZarrDataset::LoadEOPFMetadata()
{
    CPLDebug("EOPFZARR", "Load EOPFMetadata OK");
    EOPF::AttachMetadata(*this, mInner->GetDescription());
}

