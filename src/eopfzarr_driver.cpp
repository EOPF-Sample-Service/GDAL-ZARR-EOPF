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

// helper function to create consistently formatted paths for QGIS
static std::string CreateQGISCompatiblePath(const std::string& path) {
    std::string qgisPath = path;
    
#ifdef _WIN32
    // Replace forward slashes with backslashes for consistent Windows paths
    for (char& c : qgisPath) {
        if (c == '/') c = '\\';
    }
    
    // Handle network paths consistently
    if (qgisPath.length() >= 2 && qgisPath[0] == '\\' && qgisPath[1] != '\\') {
        // If it starts with a single backslash but not a UNC path
        if (qgisPath.length() > 2 && qgisPath[2] == ':') {
            // This is a path with leading slash before drive letter - remove it
            qgisPath = qgisPath.substr(1);
        }
    }
    
    // Ensure UNC paths have double backslashes
    if (qgisPath.length() >= 2 && qgisPath[0] == '\\' && qgisPath[1] == '\\') {
        // This is already a proper UNC path - keep it
    }
    
    // Handle trailing backslash consistently (remove unless root drive)
    if (qgisPath.length() > 3 && qgisPath.back() == '\\') {
        qgisPath.pop_back();
    }
#endif

    return qgisPath;
}

static bool IsUrlOrVirtualPath(const std::string& path) {
    if (path.empty()) return false;
    
    CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Checking path: %s", path.c_str());
    
    // Check for GDAL virtual file systems
    if (STARTS_WITH_CI(path.c_str(), "/vsicurl/") ||
        STARTS_WITH_CI(path.c_str(), "/vsis3/") ||
        STARTS_WITH_CI(path.c_str(), "/vsigs/") ||
        STARTS_WITH_CI(path.c_str(), "/vsiaz/") ||
        STARTS_WITH_CI(path.c_str(), "/vsioss/") ||
        STARTS_WITH_CI(path.c_str(), "/vsiswift/") ||
        STARTS_WITH_CI(path.c_str(), "/vsihdfs/") ||
        STARTS_WITH_CI(path.c_str(), "/vsimem/") ||
        STARTS_WITH_CI(path.c_str(), "/vsizip/") ||
        STARTS_WITH_CI(path.c_str(), "/vsigzip/") ||
        STARTS_WITH_CI(path.c_str(), "/vsitar/")) {
        CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Detected virtual file system");
        return true;
    }
    
    // Check for URL schemes - more comprehensive check
    size_t colonPos = path.find(':');
    if (colonPos != std::string::npos && colonPos > 0 && colonPos <= 10) {
        std::string scheme = path.substr(0, colonPos);
        if (scheme == "http" || scheme == "https" || 
            scheme == "ftp" || scheme == "ftps" ||
            scheme == "s3" || scheme == "gs" || scheme == "azure" ||
            scheme == "file") {
            // Additional check: verify it looks like a real URL with ://
            if (colonPos + 2 < path.length() && 
                path[colonPos + 1] == '/' && path[colonPos + 2] == '/') {
                CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Detected URL scheme: %s", scheme.c_str());
                return true;
            }
        }
    }
    
    CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Not detected as URL/Virtual path");
    return false;
}

static bool ParseSubdatasetPath(const std::string& fullPath, std::string& mainPath, std::string& subdatasetPath)
{
    // Debug the original path
    CPLDebug("EOPFZARR", "ParseSubdatasetPath: Parsing path: %s", fullPath.c_str());

    // First, check for EOPFZARR: prefix
    const char* pszPrefix = "EOPFZARR:";
    std::string pathWithoutPrefix = fullPath;
    if (STARTS_WITH_CI(fullPath.c_str(), pszPrefix)) {
        pathWithoutPrefix = fullPath.substr(strlen(pszPrefix));
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: Removed prefix, now: %s", pathWithoutPrefix.c_str());
    }

    // EARLY CHECK: For URLs and virtual file systems, don't try to parse subdatasets from colons
    // This must happen before any other parsing logic
    if (IsUrlOrVirtualPath(pathWithoutPrefix)) {
        // For URLs and virtual paths, no subdataset parsing - treat the whole thing as the main path
        mainPath = pathWithoutPrefix;
        subdatasetPath = "";
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: URL/Virtual path detected early, no subdataset parsing - Main: %s", mainPath.c_str());
        return false;
    }

    // Check for quoted path format: "path":subds
    size_t startQuote = pathWithoutPrefix.find('\"');
    if (startQuote != std::string::npos) {
        size_t endQuote = pathWithoutPrefix.find('\"', startQuote + 1);
        if (endQuote != std::string::npos && endQuote > startQuote + 1) {
            // We have a quoted path - extract it
            mainPath = pathWithoutPrefix.substr(startQuote + 1, endQuote - startQuote - 1);

            // Check if the quoted content is a URL/virtual path
            if (IsUrlOrVirtualPath(mainPath)) {
                // Even if there's a subdataset part after the quote, ignore it for URLs
                subdatasetPath = "";
                CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found quoted URL/Virtual path, no subdataset parsing - Main: %s", mainPath.c_str());
                return false;
            }

            // Now check if there's a subdataset part after the quoted path
            if (endQuote + 1 < pathWithoutPrefix.length() && pathWithoutPrefix[endQuote + 1] == ':') {
                // We have a subdataset part - everything after the colon
                subdatasetPath = pathWithoutPrefix.substr(endQuote + 2);
                CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found quoted path with subdataset - Main: %s, Subds: %s",
                    mainPath.c_str(), subdatasetPath.c_str());

                // Don't modify URLs or virtual file system paths on Windows
#ifdef _WIN32
                if (!IsUrlOrVirtualPath(mainPath)) {
                    // Replace forward slashes with backslashes for local Windows paths only
                    for (size_t i = 0; i < mainPath.length(); ++i) {
                        if (mainPath[i] == '/') {
                            mainPath[i] = '\\';
                        }
                    }

                    // Remove leading slash if present in Windows paths (e.g., /C:/...)
                    if (!mainPath.empty() && mainPath[0] == '\\' &&
                        mainPath.length() > 2 && mainPath[1] != '\\' && mainPath[2] == ':') {
                        mainPath = mainPath.substr(1);
                    }

                    // Remove trailing slash if present
                    if (!mainPath.empty() && mainPath.back() == '\\') {
                        mainPath.pop_back();
                    }
                }
#endif
                return true;
            }
            // No subdataset part, just a quoted path
            else {
                CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found quoted path without subdataset - Main: %s", mainPath.c_str());
                subdatasetPath = "";
#ifdef _WIN32
                if (!IsUrlOrVirtualPath(mainPath)) {
                    // Fix Windows paths - same as above
                    for (size_t i = 0; i < mainPath.length(); ++i) {
                        if (mainPath[i] == '/') {
                            mainPath[i] = '\\';
                        }
                    }

                    if (!mainPath.empty() && mainPath[0] == '\\' &&
                        mainPath.length() > 2 && mainPath[1] != '\\' && mainPath[2] == ':') {
                        mainPath = mainPath.substr(1);
                    }

                    if (!mainPath.empty() && mainPath.back() == '\\') {
                        mainPath.pop_back();
                    }
                }
#endif
                return false;
            }
        }
    }

    // Check for simple path with subdataset separator (e.g., EOPFZARR:path:subds)
    // This is complicated on Windows due to drive letters (C:) and URLs (https:)
    std::string tmpPath = pathWithoutPrefix;
    
    // Double-check: URLs should have been caught earlier, but just in case
    if (IsUrlOrVirtualPath(tmpPath)) {
        mainPath = tmpPath;
        subdatasetPath = "";
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: URL/Virtual path detected in fallback check - Main: %s", mainPath.c_str());
        return false;
    } else {
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: Not detected as URL/Virtual path, checking for subdatasets: %s", tmpPath.c_str());
    }
    
    size_t colonPos = tmpPath.find(':');

    // Skip Windows drive letters
    if (colonPos != std::string::npos) {
#ifdef _WIN32
        // On Windows, the first colon might be the drive letter
        if (colonPos == 1) {
            // This is likely a drive letter - look for another colon
            colonPos = tmpPath.find(':', colonPos + 1);
        }
#endif
    }

    if (colonPos != std::string::npos) {
        mainPath = tmpPath.substr(0, colonPos);
        subdatasetPath = tmpPath.substr(colonPos + 1);
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found simple path with subdataset - Main: %s, Subds: %s",
            mainPath.c_str(), subdatasetPath.c_str());

#ifdef _WIN32
        if (!IsUrlOrVirtualPath(mainPath)) {
            // Fix Windows paths - same as above
            for (size_t i = 0; i < mainPath.length(); ++i) {
                if (mainPath[i] == '/') {
                    mainPath[i] = '\\';
                }
            }

            if (!mainPath.empty() && mainPath[0] == '\\' &&
                mainPath.length() > 2 && mainPath[1] != '\\' && mainPath[2] == ':') {
                mainPath = mainPath.substr(1);
            }

            if (!mainPath.empty() && mainPath.back() == '\\') {
                mainPath.pop_back();
            }
        }
#endif
        return true;
    }

    // Not a subdataset path
    mainPath = pathWithoutPrefix;
    subdatasetPath = "";

#ifdef _WIN32
    if (!IsUrlOrVirtualPath(mainPath)) {
        // Fix Windows paths - same as above
        for (size_t i = 0; i < mainPath.length(); ++i) {
            if (mainPath[i] == '/') {
                mainPath[i] = '\\';
            }
        }

        if (!mainPath.empty() && mainPath[0] == '\\' &&
            mainPath.length() > 2 && mainPath[1] != '\\' && mainPath[2] == ':') {
            mainPath = mainPath.substr(1);
        }

        if (!mainPath.empty() && mainPath.back() == '\\') {
            mainPath.pop_back();
        }
    }
#endif

    CPLDebug("EOPFZARR", "ParseSubdatasetPath: No subdataset found - Main: %s", mainPath.c_str());
    return false;
}

static bool IsEOPFZarr(const std::string& path) {
    // Extract main path if this is a subdataset reference
    std::string mainPath, subdatasetPath;
    ParseSubdatasetPath(path, mainPath, subdatasetPath);

    // Use the main path for detection
    std::string pathToCheck = mainPath;

    // 1. Check for .zmetadata file
    std::string zmetaPath = CPLFormFilename(pathToCheck.c_str(), ".zmetadata", nullptr);
    if (HasFile(zmetaPath)) {
        CPLJSONDocument doc;
        if (doc.Load(zmetaPath)) {
            const CPLJSONObject& root = doc.GetRoot();
            const CPLJSONObject& metadata = root.GetObj("metadata");
            if (metadata.IsValid()) {
                const CPLJSONObject& zattrs = metadata.GetObj(".zattrs");
                if (zattrs.IsValid() &&
                    (zattrs.GetObj("stac_discovery").IsValid() ||
                        !zattrs.GetString("eopf_category").empty() ||
                        !zattrs.GetString("eopf:resolutions").empty())) {
                    CPLDebug("EOPFZARR", "Dataset at %s identified as EOPF by .zmetadata markers", pathToCheck.c_str());
                    return true;
                }
            }
        }
    }

    // 2. Check if it's a subdataset path with EOPFZARR prefix
    if (STARTS_WITH_CI(path.c_str(), "EOPFZARR:")) {
        CPLDebug("EOPFZARR", "Path starts with EOPFZARR: prefix, accepting");
        return true;
    }

    return false;
}
/* -------------------------------------------------------------------- */
/*      Identify — only accept Zarr files with the EOPF prefix           */
/* -------------------------------------------------------------------- */
static int EOPFIdentify(GDALOpenInfo* poOpenInfo)
{
    // Don't handle update mode
    if (poOpenInfo->eAccess == GA_Update)
        return FALSE;

    const char* pszFilename = poOpenInfo->pszFilename;
    CPLDebug("EOPFZARR", "EOPFIdentify called with filename: %s", pszFilename);

    // Only identify with explicit prefixes or options
    if (STARTS_WITH_CI(pszFilename, "EOPFZARR:")) {
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPFZARR prefix detected, returning TRUE");
        return TRUE;
    }

    // Check EOPF_PROCESS option
    const char* pszEOPFProcess = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");
    if (pszEOPFProcess && 
        (EQUAL(pszEOPFProcess, "YES") || EQUAL(pszEOPFProcess, "TRUE") || EQUAL(pszEOPFProcess, "1"))) {
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS option detected, returning TRUE");
        return TRUE;
    }

    // Decline all other files
    CPLDebug("EOPFZARR", "EOPFIdentify: No match, returning FALSE");
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
static GDALDataset* OpenSubdataset(const std::string& mainPath, 
                                 const std::string& subdatasetPath,
                                 unsigned int nOpenFlags, 
                                 char** papszOpenOptions)
{
    char* const azDrvList[] = { (char*)"Zarr", nullptr };
    
    // Try direct path first - most reliable for QGIS
    std::string directPath = mainPath;
    if (!directPath.empty() && directPath.back() != '/' && directPath.back() != '\\')
#ifdef _WIN32
        directPath += '\\';
#else
        directPath += '/';
#endif
    directPath += subdatasetPath;
    
    CPLDebug("EOPFZARR", "Attempting to open subdataset directly: %s", directPath.c_str());
    GDALDataset* poDS = static_cast<GDALDataset*>(GDALOpenEx(directPath.c_str(), 
        nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY, azDrvList, papszOpenOptions, nullptr));
        
    if (poDS)
        return poDS;
        
    // If direct access fails, try opening parent and then finding the subdataset
    CPLDebug("EOPFZARR", "Direct access failed, trying through parent dataset");
    
    // Open parent dataset
    GDALDataset* poParentDS = static_cast<GDALDataset*>(GDALOpenEx(mainPath.c_str(), 
        nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY, azDrvList, papszOpenOptions, nullptr));
    
    if (!poParentDS) {
        CPLDebug("EOPFZARR", "Failed to open parent dataset: %s", mainPath.c_str());
        return nullptr;
    }
    
    std::unique_ptr<GDALDataset> parentGuard(poParentDS);
    
    // Find the subdataset
    char** papszSubdatasets = poParentDS->GetMetadata("SUBDATASETS");
    if (!papszSubdatasets) {
        CPLDebug("EOPFZARR", "No subdatasets found in parent dataset");
        return nullptr;
    }
    
    // Prepare subdataset path for matching - normalize for comparison
    std::string cleanSubdsPath = subdatasetPath;
    if (!cleanSubdsPath.empty() && 
        (cleanSubdsPath.front() == '/' || cleanSubdsPath.front() == '\\')) {
        cleanSubdsPath = cleanSubdsPath.substr(1);
    }
    
    // Search for matching subdataset
    for (int i = 0; papszSubdatasets[i]; i++) {
        if (strstr(papszSubdatasets[i], "_NAME=") == nullptr)
            continue;
            
        char* pszKey = nullptr;
        const char* pszValue = CPLParseNameValue(papszSubdatasets[i], &pszKey);
        if (pszKey && pszValue && STARTS_WITH_CI(pszValue, "ZARR:")) {
            // Try to extract subdataset path from ZARR path
            std::string extractedPath = pszValue;
            size_t pathEndPos = extractedPath.find("\":", 5);  // Look for ": after ZARR:
            
            if (pathEndPos != std::string::npos) {
                std::string subdsComponent = extractedPath.substr(pathEndPos + 2);
                
                // Normalize subdataset paths for comparison
                if (!subdsComponent.empty() && 
                    (subdsComponent.front() == '/' || subdsComponent.front() == '\\')) {
                    subdsComponent = subdsComponent.substr(1);
                }
                
                if (subdsComponent == cleanSubdsPath) {
                    // Found a matching subdataset, open it directly
                    CPLDebug("EOPFZARR", "Found matching subdataset: %s", pszValue);
                    
                    CPLErrorHandler oldHandler = CPLSetErrorHandler(CPLQuietErrorHandler);
                    poDS = static_cast<GDALDataset*>(GDALOpenEx(pszValue, 
                        nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY, 
                        azDrvList, papszOpenOptions, nullptr));
                    CPLSetErrorHandler(oldHandler);
                    
                    if (poDS) {
                        return poDS;
                    }
                }
            }
        }
        CPLFree(pszKey);
    }
    
    CPLDebug("EOPFZARR", "No matching subdataset found for: %s", subdatasetPath.c_str());
    return nullptr;
}

static GDALDataset* EOPFOpen(GDALOpenInfo* poOpenInfo)
{
    const char* pszFilename = poOpenInfo->pszFilename;
    CPLDebug("EOPFZARR", "EOPFOpen: Opening file: %s", pszFilename);
    CPLDebug("EOPFZARR", "EOPFOpen: ENTRY POINT REACHED - Driver is working!");

    // Parse the filename
    std::string mainPath, subdatasetPath;
    bool isSubdataset = ParseSubdatasetPath(pszFilename, mainPath, subdatasetPath);

    // Strip the EOPFZARR: prefix if present
    if (STARTS_WITH_CI(mainPath.c_str(), "EOPFZARR:"))
        mainPath = mainPath.substr(9);

    // For URLs and virtual file systems, skip existence check as it may not work reliably
    // The underlying Zarr driver will handle the actual validation
    if (!IsUrlOrVirtualPath(mainPath)) {
        // Check file existence only for local paths
        VSIStatBufL sStat;
        if (VSIStatL(mainPath.c_str(), &sStat) != 0)
        {
            CPLError(CE_Failure, CPLE_OpenFailed, "EOPFZARR driver: Main path '%s' does not exist", mainPath.c_str());
            return nullptr;
        }
    } else {
        CPLDebug("EOPFZARR", "Skipping existence check for URL/Virtual path: %s", mainPath.c_str());
    }

    // Create option list without EOPF_PROCESS
    char** papszOpenOptions = nullptr;
    for (char** papszIter = poOpenInfo->papszOpenOptions; papszIter && *papszIter; ++papszIter)
    {
        char* pszKey = nullptr;
        const char* pszValue = CPLParseNameValue(*papszIter, &pszKey);
        if (pszKey && !EQUAL(pszKey, "EOPF_PROCESS"))
        {
            papszOpenOptions = CSLSetNameValue(papszOpenOptions, pszKey, pszValue);
        }
        CPLFree(pszKey);
    }

    // Use GDALOpenEx to open the dataset
    GDALDataset* poUnderlyingDS = nullptr;

    // For subdatasets, we need special handling
    if (isSubdataset)
    {
        // Use our helper function that handles subdatasets
        poUnderlyingDS = OpenSubdataset(mainPath, subdatasetPath, poOpenInfo->nOpenFlags, papszOpenOptions);
    }
    else
    {
        // Use safer GDALOpenEx API with explicit driver list
        char* const azDrvList[] = { (char*)"Zarr", nullptr };
        poUnderlyingDS = static_cast<GDALDataset*>(
            GDALOpenEx(mainPath.c_str(),
                poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                azDrvList,  // Explicitly use Zarr driver
                papszOpenOptions,
                nullptr));
    }

    CSLDestroy(papszOpenOptions);

    if (!poUnderlyingDS)
    {
        CPLError(CE_Failure, CPLE_OpenFailed, "Zarr driver could not open %s", mainPath.c_str());
        return nullptr;
    }

    // Create our wrapper dataset
    EOPFZarrDataset* poDS = EOPFZarrDataset::Create(poUnderlyingDS, gEOPFDriver);
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
    GDALDriver* driver = new GDALDriver();
    driver->SetDescription("EOPFZARR");
    driver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Zarr Wrapper Driver");
    driver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");
    driver->SetMetadataItem(GDAL_DMD_SUBDATASETS, "YES");
    
    // Add our own EOPF_PROCESS option
    const char* pszOptions = 
        "<OpenOptionList>"
        "  <Option name='EOPF_PROCESS' type='boolean' default='NO' description='Enable EOPF features'>"
        "    <Value>YES</Value>"
        "    <Value>NO</Value>"
        "  </Option>"
        "</OpenOptionList>";
    driver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, pszOptions);

    driver->pfnIdentify = EOPFIdentify;
    driver->pfnOpen = EOPFOpen;

    // Insert at the beginning to get higher priority than other drivers
    GetGDALDriverManager()->RegisterDriver(driver);
    gEOPFDriver = driver;
    
    CPLDebug("EOPFZARR", "EOPF Zarr driver registered with high priority");
}

// Add a cleanup function
extern "C" EOPFZARR_DLL void GDALDeregisterEOPFZarr()
{
    if (gEOPFDriver)
    {
        // Get the driver manager - do not delete the driver itself
        GDALDriverManager* poDM = GetGDALDriverManager();
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
