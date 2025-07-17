/**********************************************************************
 *  EOPF‑Zarr GDAL driver — Refactored main driver file
 *
 *  This file contains only the core GDAL driver interface functions.
 *  All implementation details are delegated to specialized classes.
 **********************************************************************/
#include "eopfzarr_dataset.h"
#include "eopfzarr_path_utils.h"
#include "eopfzarr_opener.h"
#include "eopfzarr_registry.h"
#include "eopfzarr_errors.h"
#include "gdal_priv.h"
#include "cpl_vsi.h"
#include "cpl_string.h"

// Export declarations
#ifdef _WIN32
#define EOPFZARR_DLL __declspec(dllexport)
#else
#define EOPFZARR_DLL
#endif

using namespace EOPFPathUtils;
using namespace EOPFZarrOpener;
using namespace EOPFZarrRegistry;
using namespace EOPFErrorUtils;

/* -------------------------------------------------------------------- */
/*      Identify — only accept Zarr files with the EOPF prefix           */
/* -------------------------------------------------------------------- */
static int EOPFIdentify(GDALOpenInfo *poOpenInfo)
{
    // Don't handle update mode
    if (poOpenInfo->eAccess == GA_Update)
        return FALSE;

    const char *pszFilename = poOpenInfo->pszFilename;

    // Only identify with explicit prefixes or options
    if (STARTS_WITH_CI(pszFilename, "EOPFZARR:"))
        return TRUE;

    // Check EOPF_PROCESS option
    const char *pszEOPFProcess = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");
    if (pszEOPFProcess && 
        (EQUAL(pszEOPFProcess, "YES") || EQUAL(pszEOPFProcess, "TRUE") || EQUAL(pszEOPFProcess, "1")))
        return TRUE;

    // Decline all other files
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*      Open — delegate to specialized opener classes                    */
/* -------------------------------------------------------------------- */
static GDALDataset *EOPFOpen(GDALOpenInfo *poOpenInfo)
{
    const char *pszFilename = poOpenInfo->pszFilename;
    ErrorHandler::Debug(std::string("Opening file: ") + pszFilename);

    // Parse the filename using dedicated parser
    auto parsedPath = PathParser::Parse(pszFilename);

    // Skip file existence check for URLs and virtual paths
    if (!parsedPath.isUrl && !parsedPath.isVirtualPath)
    {
        // Check file existence for local files only
        VSIStatBufL sStat;
        if (VSIStatL(parsedPath.mainPath.c_str(), &sStat) != 0)
        {
            ErrorHandler::ReportFileNotFound(parsedPath.mainPath);
            return nullptr;
        }
    }

    // Open the underlying dataset using specialized opener
    GDALDataset *poUnderlyingDS = nullptr;
    
    if (parsedPath.isSubdataset)
    {
        poUnderlyingDS = DatasetOpener::OpenSubdataset(
            parsedPath.mainPath, 
            parsedPath.subdatasetPath,
            poOpenInfo->nOpenFlags, 
            poOpenInfo->papszOpenOptions);
    }
    else
    {
        poUnderlyingDS = DatasetOpener::OpenMainDataset(
            parsedPath.mainPath,
            poOpenInfo->nOpenFlags,
            poOpenInfo->papszOpenOptions);
    }

    if (!poUnderlyingDS)
    {
        ErrorHandler::ReportOpenFailure(parsedPath.mainPath, "Zarr driver could not open path");
        return nullptr;
    }

    // Create our wrapper dataset
    EOPFZarrDataset *poDS = EOPFZarrDataset::Create(poUnderlyingDS, DriverRegistry::GetDriver());
    if (poDS)
    {
        poDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");
        if (parsedPath.isSubdataset && !parsedPath.subdatasetPath.empty())
            poDS->SetMetadataItem("SUBDATASET_PATH", parsedPath.subdatasetPath.c_str());
    }
    else
    {
        // If we failed to create the wrapper, close the underlying dataset
        GDALClose(poUnderlyingDS);
        ErrorHandler::ReportWrapperFailure("Failed to create EOPF wrapper dataset");
    }

    return poDS;
}

/* -------------------------------------------------------------------- */
/*      GDALRegister_EOPFZarr — entry point called by Driver Manager     */
/* -------------------------------------------------------------------- */
extern "C" EOPFZARR_DLL void GDALRegister_EOPFZarr()
{
    if (!DriverRegistry::RegisterDriver())
    {
        ErrorHandler::ReportWrapperFailure("Failed to register EOPF Zarr driver");
        return;
    }

    // Set up function pointers
    GDALDriver* driver = DriverRegistry::GetDriver();
    if (driver)
    {
        driver->pfnIdentify = EOPFIdentify;
        driver->pfnOpen = EOPFOpen;
    }

    ErrorHandler::Debug("EOPF Zarr driver registered successfully");
}

// Add a cleanup function
extern "C" EOPFZARR_DLL void GDALDeregisterEOPFZarr()
{
    DriverRegistry::DeregisterDriver();
}

#ifdef _WIN32
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        // Only if not during process termination
        if (lpvReserved == NULL)
        {
            GDALDeregisterEOPFZarr();
        }
    }
    return TRUE;
}
#endif

extern "C" EOPFZARR_DLL void GDALRegisterMe()
{
    GDALRegister_EOPFZarr();
}
