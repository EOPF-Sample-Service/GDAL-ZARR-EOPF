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
#include <algorithm>
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

// helper function to create consistently formatted paths for QGIS
static std::string CreateQGISCompatiblePath(const std::string &path)
{
    std::string qgisPath = path;

#ifdef _WIN32
    // Replace forward slashes with backslashes for consistent Windows paths
    std::replace(qgisPath.begin(), qgisPath.end(), '/', '\\');

    // Handle network paths consistently
    if (qgisPath.length() >= 2 && qgisPath[0] == '\\' && qgisPath[1] != '\\')
    {
        // If it starts with a single backslash but not a UNC path
        if (qgisPath.length() > 2 && qgisPath[2] == ':')
        {
            // This is a path with leading slash before drive letter - remove it
            qgisPath = qgisPath.substr(1);
        }
    }

    // Ensure UNC paths have double backslashes
    if (qgisPath.length() >= 2 && qgisPath[0] == '\\' && qgisPath[1] == '\\')
    {
        // This is already a proper UNC path - keep it
    }

    // Handle trailing backslash consistently (remove unless root drive)
    if (qgisPath.length() > 3 && qgisPath.back() == '\\')
    {
        qgisPath.pop_back();
    }
#endif

    return qgisPath;
}

// Helper function to detect if a path is a URL or virtual file system path
static bool IsUrlOrVirtualPath(const std::string& path)
{
    CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Checking path: %s", path.c_str());
    
    // Check for GDAL virtual file systems - these should NEVER be normalized on Windows
    if (STARTS_WITH_CI(path.c_str(), "/vsi")) {
        CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Detected virtual file system");
        return true;
    }
    
    // Check for URL schemes
    if (path.find("://") != std::string::npos) {
        size_t schemeEnd = path.find("://");
        std::string scheme = path.substr(0, schemeEnd);
        CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Detected URL scheme: %s", scheme.c_str());
        return true;
    }
    
    return false;
}

static bool ParseSubdatasetPath(const std::string &fullPath,
                                std::string       &mainPath,
                                std::string       &subdatasetPath)
{
    CPLDebug("EOPFZARR", "ParseSubdatasetPath: in=\"%s\"", fullPath.c_str());

    // 1. Strip prefix
    const char *pszPrefix = "EOPFZARR:";
    std::string withoutPrefix = STARTS_WITH_CI(fullPath.c_str(), pszPrefix)
                                ? fullPath.substr(strlen(pszPrefix))
                                : fullPath;

    // 2. Handle quoted form or VFS paths
    if (!withoutPrefix.empty() && withoutPrefix[0] == '\"')
    {
        size_t quoteColon = withoutPrefix.find("\":");
        size_t endQuote   = withoutPrefix.find('\"', 1);

        if (quoteColon != std::string::npos)     /* Form: "main":subds */
        {
            mainPath       = withoutPrefix.substr(1, quoteColon - 1);
            subdatasetPath = withoutPrefix.substr(quoteColon + 2);
            CPLDebug("EOPFZARR", "  ➜ quoted+subds  main=\"%s\"  sub=\"%s\"",
                     mainPath.c_str(), subdatasetPath.c_str());
        }
        else if (endQuote != std::string::npos)  /* Form: "main" */
        {
            mainPath       = withoutPrefix.substr(1, endQuote - 1);
            subdatasetPath.clear();
            CPLDebug("EOPFZARR", "  ➜ quoted root   main=\"%s\"", mainPath.c_str());
        }
        else
        {
            mainPath = withoutPrefix;
            subdatasetPath.clear();
        }
    }
    else if (IsUrlOrVirtualPath(withoutPrefix))
    {
        // For URLs/VFS, split at .zarr if subdataset is present
        size_t zarrPos = withoutPrefix.find(".zarr");
        if (zarrPos != std::string::npos)
        {
            size_t slashPos = withoutPrefix.find('/', zarrPos + 5); // Next '/' after ".zarr"
            if (slashPos != std::string::npos)
            {
                mainPath = withoutPrefix.substr(0, slashPos);
                subdatasetPath = withoutPrefix.substr(slashPos + 1);
                CPLDebug("EOPFZARR", "  ➜ URL/VFS with subds  main=\"%s\"  sub=\"%s\"",
                         mainPath.c_str(), subdatasetPath.c_str());
            }
            else
            {
                mainPath = withoutPrefix;
                subdatasetPath.clear();
                CPLDebug("EOPFZARR", "  ➜ URL/VFS root   main=\"%s\"", mainPath.c_str());
            }
        }
        else
        {
            mainPath = withoutPrefix;
            subdatasetPath.clear();
            CPLDebug("EOPFZARR", "  ➜ URL/VFS path   main=\"%s\"", mainPath.c_str());
        }
    }
    else
    {
        // Unquoted local style; split at the first *real* ':'
        size_t delim = withoutPrefix.find(':');
    #ifdef _WIN32
        if (delim == 1)                 // C: drive letter – skip it
            delim = withoutPrefix.find(':', 2);
    #endif
        if (delim != std::string::npos)
        {
            mainPath       = withoutPrefix.substr(0, delim);
            subdatasetPath = withoutPrefix.substr(delim + 1);
            CPLDebug("EOPFZARR", "  ➜ local split   main=\"%s\"  sub=\"%s\"",
                     mainPath.c_str(), subdatasetPath.c_str());
        }
        else
        {
            mainPath       = withoutPrefix;
            subdatasetPath.clear();
        }
    }

    // 3. Decide if URL / VFS
    const bool isVirtual = IsUrlOrVirtualPath(mainPath);

    // 4. WINDOWS normalisation (local paths **only**)
    #ifdef _WIN32
    if (!isVirtual)
    {
        std::replace(mainPath.begin(), mainPath.end(), '/', '\\');
        if (mainPath.size() > 2 && mainPath[0] == '\\' &&
            mainPath[2] == ':' && mainPath[1] != '\\')
            mainPath.erase(0, 1);
        if (mainPath.size() > 3 && mainPath.back() == '\\')
            mainPath.pop_back();
    }
    #endif

    CPLDebug("EOPFZARR", "ParseSubdatasetPath: out  main=\"%s\"  sub=\"%s\"  url=%s",
             mainPath.c_str(), subdatasetPath.c_str(),
             isVirtual ? "YES" : "NO");

    return !subdatasetPath.empty();
}

static bool ParseSubdatasetPath2(const std::string &fullPath,
                                std::string       &mainPath,
                                std::string       &subdatasetPath)
{
    CPLDebug("EOPFZARR", "ParseSubdatasetPath: in=\"%s\"", fullPath.c_str());

    // 1. strip prefix
    const char *pszPrefix = "EOPFZARR:";
    std::string withoutPrefix = STARTS_WITH_CI(fullPath.c_str(), pszPrefix)
                                ? fullPath.substr(strlen(pszPrefix))
                                : fullPath;

    // 2. QUOTED form
    if (!withoutPrefix.empty() && withoutPrefix[0] == '\"')
    {
        size_t quoteColon = withoutPrefix.find("\":");   // look for "\":"
        size_t endQuote   = withoutPrefix.find('\"', 1); // plain root form

        if (quoteColon != std::string::npos)     /* form 1 */
        {
            mainPath       = withoutPrefix.substr(1, quoteColon - 1);
            subdatasetPath = withoutPrefix.substr(quoteColon + 2); // skip ":
            CPLDebug("EOPFZARR","  ➜ quoted+subds  main=\"%s\"  sub=\"%s\"",
                     mainPath.c_str(), subdatasetPath.c_str());
        }
        else if (endQuote != std::string::npos)  /* form 2 */
        {
            mainPath       = withoutPrefix.substr(1, endQuote - 1);
            subdatasetPath.clear();
            CPLDebug("EOPFZARR","  ➜ quoted root   main=\"%s\"", mainPath.c_str());
        }
        else
        {
            mainPath = withoutPrefix;
        }
    }
    else if (IsUrlOrVirtualPath(withoutPrefix))
    {
        // For URLs and GDAL VFS paths, do NOT split at colon
        mainPath = withoutPrefix;
        subdatasetPath.clear();
        CPLDebug("EOPFZARR","  ➜ URL/VFS path   main=\"%s\"", mainPath.c_str());
    }
    else
    {
        // Unquoted local style; split at the first *real* ':'
        size_t delim = withoutPrefix.find(':');
    #ifdef _WIN32
        if (delim == 1)                 // C: drive letter – skip it
            delim = withoutPrefix.find(':', 2);
    #endif
        if (delim != std::string::npos)
        {
            mainPath       = withoutPrefix.substr(0, delim);
            subdatasetPath = withoutPrefix.substr(delim + 1);
            CPLDebug("EOPFZARR","  ➜ local split   main=\"%s\"  sub=\"%s\"",
                     mainPath.c_str(), subdatasetPath.c_str());
        }
        else
        {
            mainPath       = withoutPrefix;
            subdatasetPath.clear();
        }
    }

    // 3. decide if URL / VFS
    const bool isVirtual = IsUrlOrVirtualPath(mainPath);

    // 4. WINDOWS normalisation (local paths **only**)
    #ifdef _WIN32
    if (!isVirtual)
    {
        std::replace(mainPath.begin(), mainPath.end(), '/', '\\');
        if (mainPath.size() > 2 && mainPath[0] == '\\' &&
            mainPath[2] == ':' && mainPath[1] != '\\')
            mainPath.erase(0, 1);
        if (mainPath.size() > 3 && mainPath.back() == '\\')
            mainPath.pop_back();
    }
    #endif

    CPLDebug("EOPFZARR", "ParseSubdatasetPath: out  main=\"%s\"  sub=\"%s\"  url=%s",
             mainPath.c_str(), subdatasetPath.c_str(),
             isVirtual ? "YES" : "NO");

    return !subdatasetPath.empty();
}

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
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
static GDALDataset *OpenSubdataset(const std::string &mainPath,
                                   const std::string &subdatasetPath,
                                   unsigned int nOpenFlags,
                                   char **papszOpenOptions)
{
    char *const azDrvList[] = {(char *)"Zarr", nullptr};

    // Try direct path with Zarr syntax if subdataset is specified
    GDALDataset *poDS = nullptr;
    if (!subdatasetPath.empty())
    {
        std::string zarrPath = "ZARR:\"" + mainPath + "\":" + subdatasetPath;
        CPLDebug("EOPFZARR", "Attempting to open subdataset with Zarr syntax: %s", zarrPath.c_str());
        poDS = static_cast<GDALDataset *>(GDALOpenEx(zarrPath.c_str(),
                                                     nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                                     azDrvList, papszOpenOptions, nullptr));
        if (poDS)
            return poDS;
    }

    // Fallback to direct path concatenation
    std::string directPath = mainPath;
    if (!directPath.empty() && directPath.back() != '/' && directPath.back() != '\\')
#ifdef _WIN32
        directPath += '\\';
#else
        directPath += '/';
#endif
    directPath += subdatasetPath;

    CPLDebug("EOPFZARR", "Attempting to open subdataset directly: %s", directPath.c_str());
    poDS = static_cast<GDALDataset *>(GDALOpenEx(directPath.c_str(),
                                                 nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                                 azDrvList, papszOpenOptions, nullptr));
    if (poDS)
        return poDS;

    // If direct access fails, try opening parent and finding the subdataset
    CPLDebug("EOPFZARR", "Direct access failed, trying through parent dataset");

    // Open parent dataset
    GDALDataset *poParentDS = static_cast<GDALDataset *>(GDALOpenEx(mainPath.c_str(),
                                                                    nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                                                    azDrvList, papszOpenOptions, nullptr));
    if (!poParentDS)
    {
        CPLDebug("EOPFZARR", "Failed to open parent dataset: %s", mainPath.c_str());
        return nullptr;
    }

    std::unique_ptr<GDALDataset> parentGuard(poParentDS);

    // Find the subdataset
    char **papszSubdatasets = poParentDS->GetMetadata("SUBDATASETS");
    if (!papszSubdatasets)
    {
        CPLDebug("EOPFZARR", "No subdatasets found in parent dataset");
        return nullptr;
    }

    // Prepare subdataset path for matching - normalize for comparison
    std::string cleanSubdsPath = subdatasetPath;
    if (!cleanSubdsPath.empty() &&
        (cleanSubdsPath.front() == '/' || cleanSubdsPath.front() == '\\'))
    {
        cleanSubdsPath = cleanSubdsPath.substr(1);
    }

    // Search for matching subdataset
    for (int i = 0; papszSubdatasets[i]; i++)
    {
        if (strstr(papszSubdatasets[i], "_NAME=") == nullptr)
            continue;

        char *pszKey = nullptr;
        const char *pszValue = CPLParseNameValue(papszSubdatasets[i], &pszKey);
        if (pszKey && pszValue && STARTS_WITH_CI(pszValue, "ZARR:"))
        {
            // Extract subdataset path from ZARR path
            std::string extractedPath = pszValue;
            size_t pathEndPos = extractedPath.find("\":", 5); // Look for ": after ZARR:

            if (pathEndPos != std::string::npos)
            {
                std::string subdsComponent = extractedPath.substr(pathEndPos + 2);

                // Normalize subdataset paths for comparison
                if (!subdsComponent.empty() &&
                    (subdsComponent.front() == '/' || subdsComponent.front() == '\\'))
                {
                    subdsComponent = subdsComponent.substr(1);
                }

                if (subdsComponent == cleanSubdsPath)
                {
                    CPLDebug("EOPFZARR", "Found matching subdataset: %s", pszValue);
                    CPLErrorHandler oldHandler = CPLSetErrorHandler(CPLQuietErrorHandler);
                    poDS = static_cast<GDALDataset *>(GDALOpenEx(pszValue,
                                                                 nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                                                 azDrvList, papszOpenOptions, nullptr));
                    CPLSetErrorHandler(oldHandler);
                    if (poDS)
                        return poDS;
                }
            }
        }
        CPLFree(pszKey);
    }

    CPLDebug("EOPFZARR", "No matching subdataset found for: %s", subdatasetPath.c_str());
    return nullptr;
}


static GDALDataset *EOPFOpen(GDALOpenInfo *poOpenInfo)
{
    const char *pszFilename = poOpenInfo->pszFilename;
    CPLDebug("EOPFZARR", "EOPFOpen: Opening file: %s", pszFilename);

    // Parse the filename
    std::string mainPath, subdatasetPath;
    bool isSubdataset = ParseSubdatasetPath(pszFilename, mainPath, subdatasetPath);

    // Strip the EOPFZARR: prefix if present
    if (STARTS_WITH_CI(mainPath.c_str(), "EOPFZARR:"))
        mainPath = mainPath.substr(9);

    // Skip file existence check for URLs and virtual paths
    if (IsUrlOrVirtualPath(mainPath))
    {
        CPLDebug("EOPFZARR", "Skipping existence check for URL/Virtual path: %s", mainPath.c_str());
    }
    else
    {
        // Check file existence for local files only
        VSIStatBufL sStat;
        if (VSIStatL(mainPath.c_str(), &sStat) != 0)
        {
            CPLError(CE_Failure, CPLE_OpenFailed, "EOPFZARR driver: Main path '%s' does not exist", mainPath.c_str());
            return nullptr;
        }
    }

    // Create option list without EOPF_PROCESS
    char **papszOpenOptions = nullptr;
    for (char **papszIter = poOpenInfo->papszOpenOptions; papszIter && *papszIter; ++papszIter)
    {
        char *pszKey = nullptr;
        const char *pszValue = CPLParseNameValue(*papszIter, &pszKey);
        if (pszKey && !EQUAL(pszKey, "EOPF_PROCESS"))
        {
            papszOpenOptions = CSLSetNameValue(papszOpenOptions, pszKey, pszValue);
        }
        CPLFree(pszKey);
    }

    // Use GDALOpenEx to open the dataset
    GDALDataset *poUnderlyingDS = nullptr;

    // For subdatasets, we need special handling
    if (isSubdataset)
    {
        // Use our helper function that handles subdatasets
        poUnderlyingDS = OpenSubdataset(mainPath, subdatasetPath, poOpenInfo->nOpenFlags, papszOpenOptions);
    }
    else
    {
        // Prepare the path for the underlying Zarr driver
        std::string zarrPath = mainPath;

        // If this is a virtual file system path, format it properly for the Zarr driver
        if (STARTS_WITH_CI(mainPath.c_str(), "/vsi"))
        {
            // For URLs, we need to be careful about path formatting
            // The Zarr driver expects virtual file system paths to be quoted and prefixed with ZARR:
            CPLDebug("EOPFZARR", "Detected virtual file system path, formatting for Zarr driver");

            // Ensure the path uses forward slashes (important for URLs even on Windows)
            std::string normalizedPath = mainPath;
            // No need to change slashes in URLs - they should already be correct

            zarrPath = "ZARR:\"" + normalizedPath + "\"";
            CPLDebug("EOPFZARR", "Formatted Zarr path: %s", zarrPath.c_str());
        }

        // Use safer GDALOpenEx API with explicit driver list
        char *const azDrvList[] = {(char *)"Zarr", nullptr};

        CPLDebug("EOPFZARR", "Attempting to open with Zarr driver: %s", zarrPath.c_str());

        poUnderlyingDS = static_cast<GDALDataset *>(
            GDALOpenEx(zarrPath.c_str(),
                       poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                       azDrvList, // Explicitly use Zarr driver
                       papszOpenOptions,
                       nullptr));

        // If the formatted version failed, try the original path
        if (!poUnderlyingDS)
        {
            CPLDebug("EOPFZARR", "Formatted path failed, trying original path: %s", mainPath.c_str());
            poUnderlyingDS = static_cast<GDALDataset *>(
                GDALOpenEx(mainPath.c_str(),
                           poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                           azDrvList,
                           papszOpenOptions,
                           nullptr));
        }

        // If both approaches failed and this is a URL, try without /vsicurl/ prefix
        if (!poUnderlyingDS && STARTS_WITH_CI(mainPath.c_str(), "/vsicurl/"))
        {
            std::string directUrl = mainPath.substr(9); // Remove "/vsicurl/" prefix
            CPLDebug("EOPFZARR", "VSI path failed, trying direct URL: %s", directUrl.c_str());

            std::string directZarrPath = "ZARR:\"" + directUrl + "\"";
            poUnderlyingDS = static_cast<GDALDataset *>(
                GDALOpenEx(directZarrPath.c_str(),
                           poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                           azDrvList,
                           papszOpenOptions,
                           nullptr));
        }
    }

    CSLDestroy(papszOpenOptions);

    if (!poUnderlyingDS)
    {
        CPLError(CE_Failure, CPLE_OpenFailed, "Zarr driver could not open %s", mainPath.c_str());
        return nullptr;
    }

    // Create our wrapper dataset
    EOPFZarrDataset *poDS = EOPFZarrDataset::Create(poUnderlyingDS, gEOPFDriver);
    if (poDS)
    {
        poDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");
        if (isSubdataset && !subdatasetPath.empty())
            poDS->SetMetadataItem("SUBDATASET_PATH", subdatasetPath.c_str());
    }
    else
    {
        // If we failed to create the wrapper, close the underlying dataset
        GDALClose(poUnderlyingDS);
    }

    return poDS;
}

/* -------------------------------------------------------------------- */
/*      GDALRegister_EOPFZarr — entry point called by Driver Manager     */
/* -------------------------------------------------------------------- */
extern "C" EOPFZARR_DLL void GDALRegister_EOPFZarr()
{
    // Check if already registered
    if (GDALGetDriverByName("EOPFZARR") != nullptr)
        return;

    if (!GDAL_CHECK_VERSION("EOPFZARR"))
        return;

    // Create our own driver without modifying Zarr driver
    GDALDriver *driver = new GDALDriver();
    driver->SetDescription("EOPFZARR");
    driver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Zarr Wrapper Driver 1");
    driver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");
    driver->SetMetadataItem(GDAL_DMD_SUBDATASETS, "YES");

    // Add our own EOPF_PROCESS option
    const char *pszOptions =
        "<OpenOptionList>"
        "  <Option name='EOPF_PROCESS' type='boolean' default='NO' description='Enable EOPF features'>"
        "    <Value>YES</Value>"
        "    <Value>NO</Value>"
        "  </Option>"
        "</OpenOptionList>";
    driver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, pszOptions);

    driver->pfnIdentify = EOPFIdentify;
    driver->pfnOpen = EOPFOpen;

    GetGDALDriverManager()->RegisterDriver(driver);
    gEOPFDriver = driver;

    CPLDebug("EOPFZARR", "EOPF Zarr driver registered");
}

// Add a cleanup function
extern "C" EOPFZARR_DLL void GDALDeregisterEOPFZarr()
{
    if (gEOPFDriver)
    {
        // Get the driver manager - do not delete the driver itself
        GDALDriverManager *poDM = GetGDALDriverManager();
        if (poDM)
        {
            poDM->DeregisterDriver(gEOPFDriver);
        }
        gEOPFDriver = nullptr;
    }
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
