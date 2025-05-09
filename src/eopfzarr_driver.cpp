/**********************************************************************
 *  EOPF‑Zarr GDAL driver — registration, Identify(), Open()
 *
 *  This file contains only the pieces that interact directly with
 *  GDAL’s Driver Manager.  All wrapper‑dataset logic lives in
 *  eopfzarr_dataset.*.
 **********************************************************************/

#include "eopfzarr_dataset.h"
#include "gdal_priv.h"
#include "cpl_vsi.h"
#include <string>

static GDALDriver* gEOPFDriver = nullptr;   /* global ptr for reuse */

/* -------------------------------------------------------------------- */
/*  Tiny helper: does a file exist (works for /vsicurl/, /vsis3/)        */
/* -------------------------------------------------------------------- */
static bool HasFile(const std::string& path)
{
    return VSIStatL(path.c_str(), nullptr) == 0;
}

/* -------------------------------------------------------------------- */
/*      Identify — accept any Zarr‑V2 root (.zgroup|.zarray|.zmetadata)  */
/* -------------------------------------------------------------------- */


static int EOPFIdentify(GDALOpenInfo* info)

{
    CPLDebug("EOPFZARR", "Identify called for %s", info->pszFilename);
    if (info->eAccess == GA_Update)
        return FALSE;

    std::string root(info->pszFilename);
    if (!root.empty() && root.back() != '/') root.push_back('/');

    const char* markers[] = { ".zgroup", ".zarray", ".zmetadata", ".zattrs" };
    for (const char* m : markers)
        if (VSIStatL((root + m).c_str(), nullptr) == 0)
            return TRUE;                         // marker exists

    /* extra cheap check: directory name ends in .zarr */
    return EQUAL(CPLGetExtension(info->pszFilename), "zarr");
    
    /* let other drivers try */
    /* Build <root>/filename  
    std::string root(poOpenInfo->pszFilename);
    if (!root.empty() && root.back() != '/')
        root.push_back('/');

    return  HasFile(root + ".zgroup") ||
        HasFile(root + ".zarray") ||
        HasFile(root + ".zmetadata") ||
        HasFile(root + ".zattrs"); */
}

/* -------------------------------------------------------------------- */
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
static GDALDataset* EOPFOpen(GDALOpenInfo* poOpenInfo)
{
    if (!EOPFIdentify(poOpenInfo))
        return nullptr;

    /* Force GDAL to use only the Zarr driver for the inner open          */
    char* const azDrvList[] = { (char*)"Zarr", nullptr };

    GDALDataset* inner = static_cast<GDALDataset*>(
        GDALOpenEx(poOpenInfo->pszFilename,
            GDAL_OF_RASTER | GDAL_OF_READONLY,
            azDrvList, nullptr, nullptr));

    if (!inner)                       // inner driver failed
    {
        CPLDebug("EOPFZARR", "Core Zarr driver could not open %s – failing",
            poOpenInfo->pszFilename);
        return nullptr;              // let GDAL emit an error
    }
    return EOPFZarrDataset::Create(inner, gEOPFDriver);
}

/* -------------------------------------------------------------------- */
/*      GDALRegister_EOPFZarr — entry point called by Driver Manager     */
/* -------------------------------------------------------------------- */
extern "C" void GDALRegister_EOPFZarr()
{
    /* Avoid duplicate registration (may happen if plugins reload) */
    if (GDALGetDriverByName("EOPFZARR") != nullptr)
        return;

    /* ABI safety check */
    if (!GDAL_CHECK_VERSION("EOPFZARR"))
        return;

    gEOPFDriver = new GDALDriver();

    gEOPFDriver->SetDescription("EOPFZARR");          // short name
    gEOPFDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
        "EOPF‑zarr (Sentinel‑2) format-1");
    gEOPFDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    gEOPFDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

    gEOPFDriver->pfnIdentify = EOPFIdentify;
    gEOPFDriver->pfnOpen = EOPFOpen;

    GetGDALDriverManager()->RegisterDriver(gEOPFDriver);
}
