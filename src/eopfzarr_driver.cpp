/**********************************************************************
 *  EOPF‑Zarr GDAL driver — registration, Identify(), Open()
 *
 *  This file contains only the pieces that interact directly with
 *  GDAL's Driver Manager.  All wrapper‑dataset logic lives in
 *  eopfzarr_dataset.*.
 **********************************************************************/
#include "eopfzarr_dataset.h"
#include "gdal_priv.h"
#include "cpl_vsi.h"
#include "cpl_string.h" // Added for CSL functions (CSLRemove, CSLDuplicate, etc.)
#include <string>
#include "eopf_metadata.h"
#include "eopfzarr_config.h"

// Add Windows-specific export declarations without redefining CPL macros
#ifdef _WIN32
#define EOPFZARR_DLL __declspec(dllexport)
#else
#define EOPFZARR_DLL
#endif

static GDALDriver *gEOPFDriver = nullptr; /* global ptr for reuse */

/* -------------------------------------------------------------------- */
/*  Tiny helper: does a file exist (works for /vsicurl/, /vsis3/)        */
/* -------------------------------------------------------------------- */
static bool HasFile(const std::string &path)
{
    VSIStatBufL sStat; // Use VSIStatBufL for VSIStatL
    return VSIStatL(path.c_str(), &sStat) == 0;
}

/* -------------------------------------------------------------------- */
/*      Identify — only accept Zarr files with the EOPF prefix           */
/* -------------------------------------------------------------------- */
static int EOPFIdentify(GDALOpenInfo *poOpenInfo)
{
    if (poOpenInfo->eAccess == GA_Update)
        return FALSE;

    // Check for filename prefix if enabled - ONLY accept this case
#if USE_FILENAME_PREFIX
    const char *pszPrefix = EOPF_FILENAME_PREFIX;
    const size_t nPrefixLen = strlen(pszPrefix);

    // Only accept files explicitly starting with our prefix
    if (EQUALN(poOpenInfo->pszFilename, pszPrefix, nPrefixLen))
    {
        // Also verify there's something after the prefix
        if (strlen(poOpenInfo->pszFilename) > nPrefixLen)
        {
            CPLDebug("EOPFZARR", "EOPFIdentify: Prefix %s detected in filename %s, accepting dataset.",
                    pszPrefix, poOpenInfo->pszFilename);
            return TRUE;
        }
        else
        {
            CPLDebug("EOPFZARR", "EOPFIdentify: Prefix only with no path detected, declining: %s", 
                    poOpenInfo->pszFilename);
            return FALSE;
        }
    }
#endif

    // Everything else is rejected
    CPLDebug("EOPFZARR", "EOPFIdentify: No valid prefix detected, declining to handle: %s", 
             poOpenInfo->pszFilename);
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
static GDALDataset *EOPFOpen(GDALOpenInfo *poOpenInfo)
{
    // Double check that we're only handling prefixed paths
    const char *pszFilename = poOpenInfo->pszFilename;
    
    // If the path doesn't have our prefix, immediately return nullptr
    const char *pszPrefix = EOPF_FILENAME_PREFIX;
    const size_t nPrefixLen = strlen(pszPrefix);
    
    if (!EQUALN(pszFilename, pszPrefix, nPrefixLen))
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "EOPFZARR driver should only be used with %s prefix", pszPrefix);
        CPLDebug("EOPFZARR", "EOPFOpen: Path doesn't start with %s, rejecting: %s", 
                 pszPrefix, pszFilename);
        return nullptr;
    }

    // Process filename prefix if enabled
#if USE_FILENAME_PREFIX
    CPLDebug("EOPFZARR", "EOPFOpen: Prefix %s detected in filename %s",
             pszPrefix, pszFilename);

    // Extract the actual path after the prefix
    const char *pszActualFilename = pszFilename + nPrefixLen;
    
    if (strlen(pszActualFilename) == 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "No path specified after %s prefix", pszPrefix);
        return nullptr;
    }

    CPLDebug("EOPFZARR", "Extracted path after prefix: %s", pszActualFilename);

    // For Windows paths, check if it's a valid path
    // If the path doesn't have proper drive letter format, it might need fixing
#ifdef _WIN32
    // If path starts with single backslash, it's likely a Windows path with escaped backslashes
    // We'll need to convert it properly
    std::string fixedPath = pszActualFilename;

    // Check if GDAL already fixed the path
    VSIStatBufL sStat;
    if (VSIStatL(pszActualFilename, &sStat) != 0)
    {
        CPLDebug("EOPFZARR", "Path after prefix doesn't exist directly: %s", pszActualFilename);

        // Try standard VSI path
        std::string vsiPath = "/vsifs/";
        vsiPath += pszActualFilename;
        if (VSIStatL(vsiPath.c_str(), &sStat) == 0)
        {
            fixedPath = vsiPath;
            CPLDebug("EOPFZARR", "Using VSI filesystem path: %s", fixedPath.c_str());
        }
    }
    pszActualFilename = CPLStrdup(fixedPath.c_str());
#endif

    CPLDebug("EOPFZARR", "Using actual path: %s", pszActualFilename);

    // Open the Zarr dataset directly, explicitly requiring the Zarr driver
    char *const azDrvList[] = {(char *)"Zarr", nullptr};

    // Create option list for zarr driver
    char **papszOpenOptionsCopy = CSLDuplicate(poOpenInfo->papszOpenOptions);
    
    // Add an option to ensure the EOPFZARR driver isn't recursively called
    papszOpenOptionsCopy = CSLSetNameValue(papszOpenOptionsCopy, "GDAL_SKIP", "EOPFZARR");

    GDALDataset *poUnderlyingDataset = static_cast<GDALDataset *>(
        GDALOpenEx(pszActualFilename,
                   poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                   azDrvList,
                   papszOpenOptionsCopy,
                   nullptr));

    CSLDestroy(papszOpenOptionsCopy);
#ifdef _WIN32
    CPLFree((void *)pszActualFilename);
#endif

    if (!poUnderlyingDataset)
    {
        CPLDebug("EOPFZARR", "EOPFOpen: Core Zarr driver could not open %s.", pszActualFilename);
        CPLError(CE_Failure, CPLE_OpenFailed, 
                 "EOPFZARR driver: Core Zarr driver could not open %s.", pszActualFilename);
        return nullptr;
    }

    CPLDebug("EOPFZARR", "EOPFOpen: Successfully opened with core Zarr driver. Creating EOPF wrapper.");

    if (!gEOPFDriver)
    {
        CPLDebug("EOPFZARR", "EOPFOpen: gEOPFDriver is null. Cannot create EOPFZarrDataset.");
        CPLError(CE_Failure, CPLE_AppDefined, "EOPFZARR driver not properly initialized");
        GDALClose(poUnderlyingDataset);
        return nullptr;
    }

    // Create the wrapper dataset
    EOPFZarrDataset* poDS = EOPFZarrDataset::Create(poUnderlyingDataset, gEOPFDriver);
    
    // Set a property on the dataset to mark it as explicitly created by the EOPF driver
    // This can help debugging and identification
    if (poDS != nullptr)
    {
        poDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");
    }
    
    return poDS;
#else
    // If prefix support is disabled, this driver should not be used
    CPLError(CE_Failure, CPLE_AppDefined, 
             "EOPFZARR driver requires filename prefix support but it is disabled");
    CPLDebug("EOPFZARR", "EOPFOpen: Filename prefix support is disabled, cannot open file: %s", 
             pszFilename);
    return nullptr;
#endif
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

    // Make sure the Zarr driver is registered first - try to register it if not present
    GDALDriverManager *poDM = GetGDALDriverManager();
    GDALDriver *poZarrDriver = poDM->GetDriverByName("Zarr");
    if (poZarrDriver == nullptr)
    {
        CPLDebug("EOPFZARR", "Core Zarr driver not registered. Trying to register it.");
        // Try to register the Zarr driver
        GDALRegister_Zarr();
        
        // Check again
        poZarrDriver = poDM->GetDriverByName("Zarr");
        if (poZarrDriver == nullptr)
        {
            CPLError(CE_Warning, CPLE_AppDefined, 
                     "Core Zarr driver could not be registered. EOPFZARR may not work correctly.");
        }
    }

    gEOPFDriver = new GDALDriver();

    gEOPFDriver->SetDescription("EOPFZARR");
    gEOPFDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Zarr Wrapper Driver");
    gEOPFDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    gEOPFDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    gEOPFDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");

#if USE_FILENAME_PREFIX
    // Register the connection prefix for our driver
    gEOPFDriver->SetMetadataItem(GDAL_DMD_CONNECTION_PREFIX, EOPF_FILENAME_PREFIX);

    // Add description about the prefix usage
    CPLString prefixDescription;
    prefixDescription.Printf("EOPF datasets can be accessed using the '%s' prefix, e.g.: %sdata.zarr",
                             EOPF_FILENAME_PREFIX, EOPF_FILENAME_PREFIX);
    gEOPFDriver->SetMetadataItem("PREFIX_USAGE", prefixDescription.c_str());
#endif

    gEOPFDriver->pfnIdentify = EOPFIdentify;
    gEOPFDriver->pfnOpen = EOPFOpen;

    // Register the driver after the core Zarr driver
    // Note: RegisterDriver appends to the end of the list, ensuring it's after Zarr
    poDM->RegisterDriver(gEOPFDriver);

    CPLDebug("EOPFZARR", "EOPFZarr driver registered with filename prefix support: %s",
             USE_FILENAME_PREFIX ? EOPF_FILENAME_PREFIX : "(disabled)");
}

extern "C" EOPFZARR_DLL void GDALRegisterMe()
{
    GDALRegister_EOPFZarr();
}
