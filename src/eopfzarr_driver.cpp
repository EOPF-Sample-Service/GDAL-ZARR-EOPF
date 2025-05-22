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
#include "cpl_string.h" // Added for CSL functions (CSLRemove, CSLDuplicate, etc.)
#include <string>

 // Add Windows-specific export declarations without redefining CPL macros
#ifdef _WIN32
#define EOPFZARR_DLL __declspec(dllexport)
#else
#define EOPFZARR_DLL
#endif

static GDALDriver* gEOPFDriver = nullptr;   /* global ptr for reuse */

/* -------------------------------------------------------------------- */
/*  Tiny helper: does a file exist (works for /vsicurl/, /vsis3/)        */
/* -------------------------------------------------------------------- */
static bool HasFile(const std::string& path)
{
    VSIStatBufL sStat; // Use VSIStatBufL for VSIStatL
    return VSIStatL(path.c_str(), &sStat) == 0;
}

/* -------------------------------------------------------------------- */
/*      Identify — accept any Zarr‑V2 root (.zgroup|.zarray|.zmetadata)  */
/* -------------------------------------------------------------------- */

// Helper function to remove the i-th element from a CSL array.
static char** RemoveString(char** papszOptions, int iIndex)
{
    if (papszOptions == nullptr)
        return nullptr;
    int nCount = CSLCount(papszOptions);
    if (iIndex < 0 || iIndex >= nCount)
        return papszOptions;
    for (int i = iIndex; i < nCount - 1; i++)
    {
        papszOptions[i] = papszOptions[i + 1];
    }
    papszOptions[nCount - 1] = nullptr;
    return papszOptions;
}

static int EOPFIdentify(GDALOpenInfo* poOpenInfo)
{
    if (poOpenInfo->eAccess == GA_Update)
        return FALSE;

    // If EOPF_PROCESS is explicitly NO, this driver should not identify.
    const char* pszProcessEOPFOption = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");
    if (pszProcessEOPFOption != nullptr && EQUAL(pszProcessEOPFOption, "NO"))
    {
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS=NO, declining to identify.");
        return FALSE;
    }

    std::string rootPath(poOpenInfo->pszFilename);
    VSIStatBufL sStat;
    bool bIsDirectory = false;
    if (VSIStatL(rootPath.c_str(), &sStat) == 0 && VSI_ISDIR(sStat.st_mode))
    {
        bIsDirectory = true;
        if (!rootPath.empty() && rootPath.back() != '/')
        {
            rootPath += '/';
        }
    }

    // Only attempt to identify if it's a directory (typical for Zarr stores)
    // or if EOPF_PROCESS=YES (allowing more flexibility if user forces it)
    if (!bIsDirectory && (pszProcessEOPFOption == nullptr || !EQUAL(pszProcessEOPFOption, "YES"))) {
        // If not a directory and not explicitly told to process,
        // let the standard Zarr driver handle potentially more complex path resolutions.
        return FALSE;
    }

    // If EOPF_PROCESS=YES, we are more lenient with identification,
    // otherwise, we are stricter (e.g. requiring a directory with markers).
    if (pszProcessEOPFOption != nullptr && EQUAL(pszProcessEOPFOption, "YES")) {
        // If user forces with EOPF_PROCESS=YES, be more optimistic.
        // Check for markers or .zarr extension.
        const char* markers[] = { ".zgroup", ".zarray", ".zmetadata", ".zattrs" };
        for (const char* m : markers) {
            // Ensure rootPath has a trailing slash if it's a directory before appending marker
            std::string fullMarkerPath = rootPath;
            if (bIsDirectory && !fullMarkerPath.empty() && fullMarkerPath.back() != '/') {
                fullMarkerPath += '/';
            }
            else if (!bIsDirectory && !fullMarkerPath.empty() && fullMarkerPath.back() == '/') {
                // If it's not a directory but ends with a slash (e.g. user provided "path/to/store/"),
                // it's fine. If it doesn't end with a slash, CPLGetDirname might be needed if we
                // were to treat it like a directory. But for HasFile, direct concatenation is okay.
            }
            fullMarkerPath += m;
            if (HasFile(fullMarkerPath)) {
                CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS=YES and found marker %s in %s", m, rootPath.c_str());
                return TRUE;
            }
        }
        if (EQUAL(CPLGetExtension(poOpenInfo->pszFilename), "zarr")) {
            CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS=YES and path ends with .zarr: %s", poOpenInfo->pszFilename);
            return TRUE;
        }
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS=YES, assuming it's a Zarr store for %s", poOpenInfo->pszFilename);
        return TRUE; // Be optimistic if EOPF_PROCESS=YES
    }


    // Default identification logic (if EOPF_PROCESS is not YES or NO)
    // Requires it to be a directory containing Zarr markers.
    if (bIsDirectory) {
        const char* markers[] = { ".zgroup", ".zarray", ".zmetadata", ".zattrs" };
        for (const char* m : markers) {
            if (HasFile(rootPath + m)) { // rootPath already has trailing slash
                CPLDebug("EOPFZARR", "EOPFIdentify: Found marker %s in directory %s", m, rootPath.c_str());
                return TRUE;
            }
        }
    }

    CPLDebug("EOPFZARR", "EOPFIdentify: Conditions not met for EOPF Zarr identification of %s", poOpenInfo->pszFilename);
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
static GDALDataset* EOPFOpen(GDALOpenInfo* poOpenInfo)
{
    const char* pszProcessEOPFOption = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");

    if (pszProcessEOPFOption == nullptr || !EQUAL(pszProcessEOPFOption, "YES"))
    {
        CPLDebug("EOPFZARR", "EOPFOpen: EOPF_PROCESS option not set to YES. EOPFZarr driver will not open this dataset. Letting standard Zarr driver proceed if available.");
        return nullptr; // Default behavior: let other drivers (like standard Zarr) handle it.
    }

    // Re-check identify, now that EOPF_PROCESS=YES is confirmed.
    // This ensures that if open options were not available to Identify initially,
    // it gets a chance to re-evaluate with them.
    char** papszIdentifyOpenOptions = CSLSetNameValue(nullptr, "EOPF_PROCESS", "YES");
    GDALOpenInfo oIdentifyOpenInfo(poOpenInfo->pszFilename, poOpenInfo->nOpenFlags, papszIdentifyOpenOptions);
    // Keep other open options if they were relevant for Identify, though EOPF_PROCESS is primary here.
    // For simplicity, we are only explicitly setting EOPF_PROCESS for this re-check.
    // A more robust way would be to pass all original open options to EOPFIdentify.
    // However, EOPFIdentify itself fetches from poOpenInfo->papszOpenOptions.
    // The main check is that EOPF_PROCESS=YES is now active.

    if (!EOPFIdentify(&oIdentifyOpenInfo)) { // Pass the GDALOpenInfo with EOPF_PROCESS=YES
        CSLDestroy(papszIdentifyOpenOptions);
        CPLDebug("EOPFZARR", "EOPFOpen: EOPF_PROCESS=YES, but EOPFIdentify still returned FALSE for %s even with option explicitly set. This is unexpected.", poOpenInfo->pszFilename);
        return nullptr;
    }
    CSLDestroy(papszIdentifyOpenOptions);


    CPLDebug("EOPFZARR", "EOPFOpen: EOPF_PROCESS=YES. Attempting to open %s with EOPF wrapper.", poOpenInfo->pszFilename);

    char* const azDrvList[] = { (char*)"Zarr", nullptr };
    GDALDataset* poUnderlyingDataset = nullptr;

    // Filter out EOPF_PROCESS from open options passed to the underlying Zarr driver
    char** papszFilteredOpenOptions = CSLDuplicate(poOpenInfo->papszOpenOptions);
    int nIdx = CSLFindName(papszFilteredOpenOptions, "EOPF_PROCESS");
    if (nIdx != -1) {
        papszFilteredOpenOptions = RemoveString(papszFilteredOpenOptions, nIdx);
    }

    nIdx = CSLFindName(papszFilteredOpenOptions, "GDAL_SKIP");
    if (nIdx != -1) {
        char** papszSkippedDrivers = CSLTokenizeString2(CSLFetchNameValue(papszFilteredOpenOptions, "GDAL_SKIP"), ",", CSLT_HONOURSTRINGS);
        bool bZarrSkipped = false;
        for (int i = 0; papszSkippedDrivers && papszSkippedDrivers[i]; ++i) {
            if (EQUAL(papszSkippedDrivers[i], "Zarr")) {
                bZarrSkipped = true;
                break;
            }
        }
        CSLDestroy(papszSkippedDrivers);
        if (bZarrSkipped) {
            CPLDebug("EOPFZARR", "EOPFOpen: GDAL_SKIP contains Zarr, temporarily removing it for underlying GDALOpenEx call.");
            // Create a new list without GDAL_SKIP=Zarr or modify existing one carefully
            // For simplicity, if Zarr is in GDAL_SKIP, we remove the GDAL_SKIP option entirely for this call.
            papszFilteredOpenOptions = CSLSetNameValue(papszFilteredOpenOptions, "GDAL_SKIP", nullptr);
        }
    }


    poUnderlyingDataset = static_cast<GDALDataset*>(
        GDALOpenEx(poOpenInfo->pszFilename,
            poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
            azDrvList,
            papszFilteredOpenOptions,
            nullptr));

    CSLDestroy(papszFilteredOpenOptions);

    if (!poUnderlyingDataset)
    {
        CPLDebug("EOPFZARR", "EOPFOpen: Core Zarr driver could not open %s even with EOPF_PROCESS=YES.", poOpenInfo->pszFilename);
        return nullptr;
    }

    CPLDebug("EOPFZARR", "EOPFOpen: Successfully opened with core Zarr driver. Now creating EOPF wrapper for %s.", poOpenInfo->pszFilename);

    if (!gEOPFDriver) {
        CPLDebug("EOPFZARR", "EOPFOpen: gEOPFDriver is null. Cannot create EOPFZarrDataset.");
        GDALClose(poUnderlyingDataset);
        return nullptr;
    }

    return EOPFZarrDataset::Create(poUnderlyingDataset, gEOPFDriver);
}

/* -------------------------------------------------------------------- */
/*      GDALRegister_EOPFZarr — entry point called by Driver Manager     */
/* -------------------------------------------------------------------- */
extern "C" EOPFZARR_DLL void GDALRegister_EOPFZarr()
{
    if (GDALGetDriverByName("EOPFZARR") != nullptr)
        return;

    if (!GDAL_CHECK_VERSION("EOPFZARR"))
        return;

    gEOPFDriver = new GDALDriver();

    gEOPFDriver->SetDescription("EOPFZARR");
    gEOPFDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Zarr Wrapper Driver 4");
    gEOPFDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    gEOPFDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    gEOPFDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");

    gEOPFDriver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST,
        "<OpenOptionList>"
        "  <Option name='EOPF_PROCESS' type='boolean' description='Process as EOPF Zarr. Set to YES to activate this EOPF wrapper driver. Default: NO.' default='NO'/>"
        "</OpenOptionList>");

    gEOPFDriver->pfnIdentify = EOPFIdentify;
    gEOPFDriver->pfnOpen = EOPFOpen;

    GetGDALDriverManager()->RegisterDriver(gEOPFDriver);
    CPLDebug("EOPFZARR", "EOPFZarr driver registered with EOPF_PROCESS open option.");
}

extern "C" EOPFZARR_DLL void GDALRegisterMe()
{
    GDALRegister_EOPFZarr();
}
