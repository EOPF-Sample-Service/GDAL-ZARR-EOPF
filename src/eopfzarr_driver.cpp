/**********************************************************************
 *  EOPF‑Zarr GDAL driver — registration, Identify(), Open()
 *
 *  This file contains only the pieces that interact directly with
 *  GDAL's Driver Manager.  All wrapper‑dataset logic lives in
 *  eopfzarr_dataset.*.
 **********************************************************************/
#include <algorithm>
#include <string>

#include "cpl_string.h"  // Added for CSL functions (CSLRemove, CSLDuplicate, etc.)
#include "cpl_vsi.h"
#include "eopf_metadata.h"
#include "eopfzarr_config.h"
#include "eopfzarr_dataset.h"
#include "gdal_priv.h"

// Add Windows-specific export declarations without redefining CPL macros
#ifdef _WIN32
#define EOPFZARR_DLL __declspec(dllexport)
#else
#define EOPFZARR_DLL
#endif

static GDALDriver* gEOPFDriver = nullptr; /* global ptr for reuse */

/* -------------------------------------------------------------------- */
/*  Tiny helper: does a file exist (works for /vsicurl/, /vsis3/)        */
/* -------------------------------------------------------------------- */
static bool HasFile(const std::string& path)
{
    VSIStatBufL sStat;  // Use VSIStatBufL for VSIStatL
    return VSIStatL(path.c_str(), &sStat) == 0;
}

// helper function to create consistently formatted paths for QGIS
static std::string CreateQGISCompatiblePath(const std::string& path)
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

    // Check for URL schemes
    if (path.find("://") != std::string::npos)
    {
        // Extract scheme
        size_t schemeEnd = path.find("://");
        std::string scheme = path.substr(0, schemeEnd);
        CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Detected URL scheme: %s", scheme.c_str());
        return true;
    }

    // Check for GDAL virtual file systems
    if (STARTS_WITH_CI(path.c_str(), "/vsi"))
    {
        CPLDebug("EOPFZARR", "IsUrlOrVirtualPath: Detected virtual file system");
        return true;
    }

    return false;
}

static bool ParseSubdatasetPath(const std::string& fullPath,
                                std::string& mainPath,
                                std::string& subdatasetPath)
{
    // Debug the original path
    CPLDebug("EOPFZARR", "ParseSubdatasetPath: Parsing path: %s", fullPath.c_str());

    // First, check for EOPFZARR: prefix
    const char* pszPrefix = "EOPFZARR:";
    std::string pathWithoutPrefix = fullPath;
    if (STARTS_WITH_CI(fullPath.c_str(), pszPrefix))
    {
        pathWithoutPrefix = fullPath.substr(strlen(pszPrefix));
        CPLDebug(
            "EOPFZARR", "ParseSubdatasetPath: Removed prefix, now: %s", pathWithoutPrefix.c_str());
    }

    // Check for quoted content first to handle quoted URLs properly
    char quoteChar = '\0';
    size_t startQuote = std::string::npos;

    // Look for either double quotes or single quotes
    size_t doubleQuotePos = pathWithoutPrefix.find('\"');
    size_t singleQuotePos = pathWithoutPrefix.find('\'');

    if (doubleQuotePos != std::string::npos &&
        (singleQuotePos == std::string::npos || doubleQuotePos < singleQuotePos))
    {
        startQuote = doubleQuotePos;
        quoteChar = '\"';
    }
    else if (singleQuotePos != std::string::npos)
    {
        startQuote = singleQuotePos;
        quoteChar = '\'';
    }

    if (startQuote != std::string::npos)
    {
        size_t endQuote = pathWithoutPrefix.find(quoteChar, startQuote + 1);
        if (endQuote != std::string::npos && endQuote > startQuote + 1)
        {
            // We have a quoted path - extract it
            mainPath = pathWithoutPrefix.substr(startQuote + 1, endQuote - startQuote - 1);

            // Now check if there's a subdataset part after the quoted path
            if (endQuote + 1 < pathWithoutPrefix.length() && pathWithoutPrefix[endQuote + 1] == ':')
            {
                // We have a subdataset part - everything after the colon
                subdatasetPath = pathWithoutPrefix.substr(endQuote + 2);
                CPLDebug(
                    "EOPFZARR",
                    "ParseSubdatasetPath: Found quoted path with subdataset - Main: %s, Subds: %s",
                    mainPath.c_str(),
                    subdatasetPath.c_str());

                // Only apply Windows path transformations to local paths, not URLs
                bool isUrl = IsUrlOrVirtualPath(mainPath);
                CPLDebug("EOPFZARR",
                         "ParseSubdatasetPath: Path is URL/Virtual: %s",
                         isUrl ? "YES" : "NO");

                if (!isUrl)
                {
                    // Fix Windows paths - replace forward slashes with backward slashes
#ifdef _WIN32
                    // Replace forward slashes with backslashes
                    for (size_t i = 0; i < mainPath.length(); ++i)
                    {
                        if (mainPath[i] == '/')
                        {
                            mainPath[i] = '\\';
                        }
                    }

                    // Remove leading slash if present in Windows paths (e.g., /C:/...)
                    if (!mainPath.empty() && mainPath[0] == '\\' && mainPath.length() > 2 &&
                        mainPath[1] != '\\' && mainPath[2] == ':')
                    {
                        mainPath = mainPath.substr(1);
                    }

                    // Remove trailing slash if present
                    if (!mainPath.empty() && mainPath.back() == '\\')
                    {
                        mainPath.pop_back();
                    }
#endif
                }
                return true;
            }
            // No subdataset part after quote - but check if subdataset is embedded in path
            // Example: '/path/to/file.zarr/measurements/reflectance/r60m/b01'
            else
            {
                // Look for .zarr/ pattern which indicates a subdataset within the path
                size_t zarrPos = mainPath.rfind(".zarr/");
                if (zarrPos != std::string::npos)
                {
                    // Found .zarr/ - split the path there
                    subdatasetPath = mainPath.substr(zarrPos + 6);  // Everything after ".zarr/"
                    mainPath =
                        mainPath.substr(0, zarrPos + 5);  // Everything up to and including ".zarr"

                    CPLDebug("EOPFZARR",
                             "ParseSubdatasetPath: Found embedded subdataset in quoted path - "
                             "Main: %s, Subds: %s",
                             mainPath.c_str(),
                             subdatasetPath.c_str());

                    // Only apply Windows path transformations to local paths, not URLs
                    bool isUrl = IsUrlOrVirtualPath(mainPath);
                    CPLDebug("EOPFZARR",
                             "ParseSubdatasetPath: Path is URL/Virtual: %s",
                             isUrl ? "YES" : "NO");

                    if (!isUrl)
                    {
#ifdef _WIN32
                        // Fix Windows paths
                        for (size_t i = 0; i < mainPath.length(); ++i)
                        {
                            if (mainPath[i] == '/')
                            {
                                mainPath[i] = '\\';
                            }
                        }

                        if (!mainPath.empty() && mainPath[0] == '\\' && mainPath.length() > 2 &&
                            mainPath[1] != '\\' && mainPath[2] == ':')
                        {
                            mainPath = mainPath.substr(1);
                        }

                        if (!mainPath.empty() && mainPath.back() == '\\')
                        {
                            mainPath.pop_back();
                        }
#endif
                    }
                    return true;  // This IS a subdataset
                }

                // No .zarr/ found - this is just a plain zarr path without subdataset
                CPLDebug("EOPFZARR",
                         "ParseSubdatasetPath: Found quoted path without subdataset - Main: %s",
                         mainPath.c_str());
                subdatasetPath = "";

                // Only apply Windows path transformations to local paths, not URLs
                bool isUrl = IsUrlOrVirtualPath(mainPath);
                CPLDebug("EOPFZARR",
                         "ParseSubdatasetPath: Path is URL/Virtual: %s",
                         isUrl ? "YES" : "NO");

                if (!isUrl)
                {
#ifdef _WIN32
                    // Fix Windows paths - same as above
                    for (size_t i = 0; i < mainPath.length(); ++i)
                    {
                        if (mainPath[i] == '/')
                        {
                            mainPath[i] = '\\';
                        }
                    }

                    if (!mainPath.empty() && mainPath[0] == '\\' && mainPath.length() > 2 &&
                        mainPath[1] != '\\' && mainPath[2] == ':')
                    {
                        mainPath = mainPath.substr(1);
                    }

                    if (!mainPath.empty() && mainPath.back() == '\\')
                    {
                        mainPath.pop_back();
                    }
#endif
                }
                return false;
            }
        }
    }

    // Check for simple path with subdataset separator (e.g., EOPFZARR:path:subds)
    // This is complicated on Windows due to drive letters (C:) and URLs (https://)
    std::string tmpPath = pathWithoutPrefix;

    // For URL/VSI paths, we need to handle them differently - look for .zarr/ or .zarr:
    // instead of splitting on colons at the start (which would break URLs like https://)
    if (IsUrlOrVirtualPath(tmpPath))
    {
        // Check for .zarr/ pattern (embedded path format)
        size_t zarrSlashPos = tmpPath.rfind(".zarr/");
        // Check for .zarr: pattern (colon separator format)
        size_t zarrColonPos = tmpPath.rfind(".zarr:");

        // Use whichever pattern we find (prefer slash if both exist)
        size_t zarrPos = std::string::npos;
        char separator = '\0';

        if (zarrSlashPos != std::string::npos &&
            (zarrColonPos == std::string::npos || zarrSlashPos > zarrColonPos))
        {
            zarrPos = zarrSlashPos;
            separator = '/';
        }
        else if (zarrColonPos != std::string::npos)
        {
            zarrPos = zarrColonPos;
            separator = ':';
        }

        if (zarrPos != std::string::npos)
        {
            // Found .zarr/ or .zarr: - split the path there
            mainPath = tmpPath.substr(0, zarrPos + 5);     // Everything up to and including ".zarr"
            subdatasetPath = tmpPath.substr(zarrPos + 6);  // Everything after ".zarr/" or ".zarr:"

            CPLDebug("EOPFZARR",
                     "ParseSubdatasetPath: Found subdataset in URL/VSI path (separator='%c') - "
                     "Main: %s, Subds: %s",
                     separator,
                     mainPath.c_str(),
                     subdatasetPath.c_str());
            return true;  // This IS a subdataset
        }

        // No .zarr/ or .zarr: found - this is a plain zarr path without subdataset
        mainPath = tmpPath;
        subdatasetPath = "";
        CPLDebug("EOPFZARR",
                 "ParseSubdatasetPath: URL/VSI path without subdataset - Main: %s",
                 mainPath.c_str());
        return false;
    }

    // For non-URL paths, look for colon separator
    size_t colonPos = tmpPath.find(':');

    // Check if this looks like a URL scheme first
    if (colonPos != std::string::npos)
    {
        std::string potential_scheme = tmpPath.substr(0, colonPos);

        // Common URL schemes that should not be treated as subdataset separators
        if (potential_scheme == "http" || potential_scheme == "https" ||
            potential_scheme == "ftp" || potential_scheme == "ftps" || potential_scheme == "s3" ||
            potential_scheme == "gs" || potential_scheme == "az" || potential_scheme == "azure")
        {
            // This is a URL - don't parse for subdatasets in the simple case
            colonPos = std::string::npos;
        }
    }

#ifdef _WIN32
    // On Windows, the first colon might be the drive letter
    if (colonPos != std::string::npos && colonPos == 1)
    {
        // This is likely a drive letter - look for another colon
        colonPos = tmpPath.find(':', colonPos + 1);

        // But check again if the next part looks like a URL scheme
        if (colonPos != std::string::npos && colonPos > 2)
        {
            std::string after_drive = tmpPath.substr(2, colonPos - 2);
            if (after_drive == "http" || after_drive == "https" || after_drive == "ftp" ||
                after_drive == "ftps" || after_drive == "s3" || after_drive == "gs" ||
                after_drive == "az" || after_drive == "azure")
            {
                colonPos = std::string::npos;
            }
        }
    }
#endif

    if (colonPos != std::string::npos)
    {
        mainPath = tmpPath.substr(0, colonPos);
        subdatasetPath = tmpPath.substr(colonPos + 1);
        CPLDebug("EOPFZARR",
                 "ParseSubdatasetPath: Found simple path with subdataset - Main: %s, Subds: %s",
                 mainPath.c_str(),
                 subdatasetPath.c_str());

        // Only apply Windows path transformations to local paths, not URLs
        bool isUrl = IsUrlOrVirtualPath(mainPath);
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: Path is URL/Virtual: %s", isUrl ? "YES" : "NO");

        if (!isUrl)
        {
#ifdef _WIN32
            // Fix Windows paths - same as above
            for (size_t i = 0; i < mainPath.length(); ++i)
            {
                if (mainPath[i] == '/')
                {
                    mainPath[i] = '\\';
                }
            }

            if (!mainPath.empty() && mainPath[0] == '\\' && mainPath.length() > 2 &&
                mainPath[1] != '\\' && mainPath[2] == ':')
            {
                mainPath = mainPath.substr(1);
            }

            if (!mainPath.empty() && mainPath.back() == '\\')
            {
                mainPath.pop_back();
            }
#endif
        }
        return true;
    }

    // Not a subdataset path
    mainPath = pathWithoutPrefix;
    subdatasetPath = "";

    // Only apply Windows path transformations to local paths, not URLs
    bool isUrl = IsUrlOrVirtualPath(mainPath);
    CPLDebug("EOPFZARR", "ParseSubdatasetPath: Path is URL/Virtual: %s", isUrl ? "YES" : "NO");

    if (!isUrl)
    {
#ifdef _WIN32
        // Fix Windows paths - same as above
        for (size_t i = 0; i < mainPath.length(); ++i)
        {
            if (mainPath[i] == '/')
            {
                mainPath[i] = '\\';
            }
        }

        if (!mainPath.empty() && mainPath[0] == '\\' && mainPath.length() > 2 &&
            mainPath[1] != '\\' && mainPath[2] == ':')
        {
            mainPath = mainPath.substr(1);
        }

        if (!mainPath.empty() && mainPath.back() == '\\')
        {
            mainPath.pop_back();
        }
#endif
    }

    CPLDebug("EOPFZARR", "ParseSubdatasetPath: No subdataset found - Main: %s", mainPath.c_str());
    return false;
}

static bool IsEOPFZarr(const std::string& path)
{
    // Extract main path if this is a subdataset reference
    std::string mainPath, subdatasetPath;
    ParseSubdatasetPath(path, mainPath, subdatasetPath);

    // Use the main path for detection
    std::string pathToCheck = mainPath;

    // 1. Check for .zmetadata file
    std::string zmetaPath = CPLFormFilename(pathToCheck.c_str(), ".zmetadata", nullptr);
    if (HasFile(zmetaPath))
    {
        CPLJSONDocument doc;
        if (doc.Load(zmetaPath))
        {
            const CPLJSONObject& root = doc.GetRoot();
            const CPLJSONObject& metadata = root.GetObj("metadata");
            if (metadata.IsValid())
            {
                const CPLJSONObject& zattrs = metadata.GetObj(".zattrs");
                if (zattrs.IsValid() && (zattrs.GetObj("stac_discovery").IsValid() ||
                                         !zattrs.GetString("eopf_category").empty() ||
                                         !zattrs.GetString("eopf:resolutions").empty()))
                {
                    CPLDebug("EOPFZARR",
                             "Dataset at %s identified as EOPF by .zmetadata markers",
                             pathToCheck.c_str());
                    return true;
                }
            }
        }
    }

    // 2. Check if it's a subdataset path with EOPFZARR prefix
    if (STARTS_WITH_CI(path.c_str(), "EOPFZARR:"))
    {
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

    // Only identify with explicit prefixes or options
    if (STARTS_WITH_CI(pszFilename, "EOPFZARR:"))
        return TRUE;

    // Handle quoted strings that contain EOPFZARR: prefix
    // Check for patterns like 'EOPFZARR:...' or "EOPFZARR:..."
    size_t len = strlen(pszFilename);
    if (len > 10)  // Minimum: 'EOPFZARR:'
    {
        if ((pszFilename[0] == '\'' && pszFilename[len - 1] == '\'') ||
            (pszFilename[0] == '"' && pszFilename[len - 1] == '"'))
        {
            // Extract content between quotes
            std::string quoted_content(pszFilename + 1, len - 2);
            if (STARTS_WITH_CI(quoted_content.c_str(), "EOPFZARR:"))
                return TRUE;
        }
    }

    // Check EOPF_PROCESS option
    const char* pszEOPFProcess = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");
    if (pszEOPFProcess && (EQUAL(pszEOPFProcess, "YES") || EQUAL(pszEOPFProcess, "TRUE") ||
                           EQUAL(pszEOPFProcess, "1")))
        return TRUE;

    // Decline all other files
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
    char* const azDrvList[] = {(char*) "Zarr", nullptr};

    // Check if we should quiet errors (default is to quiet them unless EOPF_SHOW_ZARR_ERRORS=1)
    bool quietErrors = !CPLTestBool(CPLGetConfigOption("EOPF_SHOW_ZARR_ERRORS", "NO"));
    CPLErrorHandler oldHandler = nullptr;

    if (quietErrors)
    {
        oldHandler = CPLSetErrorHandler(CPLQuietErrorHandler);
    }

    // Try direct Zarr formatted path first - correct format for subdatasets
    std::string zarrFormattedPath = "ZARR:\"" + mainPath + "\":" + subdatasetPath;

    CPLDebug("EOPFZARR",
             "Attempting to open subdataset with Zarr format: %s",
             zarrFormattedPath.c_str());
    GDALDataset* poDS =
        static_cast<GDALDataset*>(GDALOpenEx(zarrFormattedPath.c_str(),
                                             nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                             azDrvList,
                                             papszOpenOptions,
                                             nullptr));

    if (poDS)
    {
        // Restore original error handler before returning
        if (quietErrors && oldHandler)
        {
            CPLSetErrorHandler(oldHandler);
        }
        return poDS;
    }

    // If direct access fails, try opening parent and then finding the subdataset
    CPLDebug("EOPFZARR", "Direct access failed, trying through parent dataset");

    // Format parent path correctly for Zarr driver
    std::string parentZarrPath = "ZARR:\"" + mainPath + "\"";

    // Open parent dataset
    GDALDataset* poParentDS =
        static_cast<GDALDataset*>(GDALOpenEx(parentZarrPath.c_str(),
                                             nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                             azDrvList,
                                             papszOpenOptions,
                                             nullptr));

    if (!poParentDS)
    {
        // Restore original error handler before returning
        if (quietErrors && oldHandler)
        {
            CPLSetErrorHandler(oldHandler);
        }
        CPLDebug("EOPFZARR", "Failed to open parent dataset: %s", mainPath.c_str());
        return nullptr;
    }

    std::unique_ptr<GDALDataset> parentGuard(poParentDS);

    // Find the subdataset
    char** papszSubdatasets = poParentDS->GetMetadata("SUBDATASETS");
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

        char* pszKey = nullptr;
        const char* pszValue = CPLParseNameValue(papszSubdatasets[i], &pszKey);
        if (pszKey && pszValue && STARTS_WITH_CI(pszValue, "ZARR:"))
        {
            // Try to extract subdataset path from ZARR path
            std::string extractedPath = pszValue;
            size_t pathEndPos = extractedPath.find("\":", 5);  // Look for ": after ZARR:

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
                    // Found a matching subdataset, open it directly
                    CPLDebug("EOPFZARR", "Found matching subdataset: %s", pszValue);

                    poDS = static_cast<GDALDataset*>(
                        GDALOpenEx(pszValue,
                                   nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                   azDrvList,
                                   papszOpenOptions,
                                   nullptr));

                    if (poDS)
                    {
                        // Restore original error handler before returning
                        if (quietErrors && oldHandler)
                        {
                            CPLSetErrorHandler(oldHandler);
                        }
                        return poDS;
                    }
                }
            }
        }
        CPLFree(pszKey);
    }

    // Restore original error handler before final return
    if (quietErrors && oldHandler)
    {
        CPLSetErrorHandler(oldHandler);
    }

    CPLDebug("EOPFZARR", "No matching subdataset found for: %s", subdatasetPath.c_str());
    return nullptr;
}

static GDALDataset* EOPFOpen(GDALOpenInfo* poOpenInfo)
{
    const char* pszFilename = poOpenInfo->pszFilename;
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
            CPLError(CE_Failure,
                     CPLE_OpenFailed,
                     "EOPFZARR driver: Main path '%s' does not exist",
                     mainPath.c_str());
            return nullptr;
        }
    }

    // Create option list without EOPF_PROCESS and SUPPRESS_AUX_WARNING
    // (we handle these internally)
    char** papszOpenOptions = nullptr;
    const char* pszSuppressAuxWarning = nullptr;
    for (char** papszIter = poOpenInfo->papszOpenOptions; papszIter && *papszIter; ++papszIter)
    {
        char* pszKey = nullptr;
        const char* pszValue = CPLParseNameValue(*papszIter, &pszKey);
        if (pszKey)
        {
            if (EQUAL(pszKey, "SUPPRESS_AUX_WARNING"))
            {
                pszSuppressAuxWarning = pszValue;
            }
            else if (!EQUAL(pszKey, "EOPF_PROCESS"))
            {
                papszOpenOptions = CSLSetNameValue(papszOpenOptions, pszKey, pszValue);
            }
        }
        CPLFree(pszKey);
    }

    // If SUPPRESS_AUX_WARNING was provided as open option, set it as config option
    // so that EOPFZarrDataset::Create can pick it up
    if (pszSuppressAuxWarning)
    {
        CPLSetThreadLocalConfigOption("EOPFZARR_SUPPRESS_AUX_WARNING", pszSuppressAuxWarning);
    }

    // Use GDALOpenEx to open the dataset
    GDALDataset* poUnderlyingDS = nullptr;

    // For subdatasets, we need special handling
    if (isSubdataset)
    {
        // Use our helper function that handles subdatasets
        poUnderlyingDS =
            OpenSubdataset(mainPath, subdatasetPath, poOpenInfo->nOpenFlags, papszOpenOptions);
    }
    else
    {
        // Prepare the path for the underlying Zarr driver
        std::string zarrPath = mainPath;

        // If this is a virtual file system path, format it properly for the Zarr driver
        if (STARTS_WITH_CI(mainPath.c_str(), "/vsi"))
        {
            // For URLs, we need to be careful about path formatting
            // The Zarr driver expects virtual file system paths to be quoted and prefixed with
            // ZARR:
            CPLDebug("EOPFZARR", "Detected virtual file system path, formatting for Zarr driver");

            // Ensure the path uses forward slashes (important for URLs even on Windows)
            std::string normalizedPath = mainPath;
            // No need to change slashes in URLs - they should already be correct

            zarrPath = "ZARR:\"" + normalizedPath + "\"";
            CPLDebug("EOPFZARR", "Formatted Zarr path: %s", zarrPath.c_str());
        }

        // Use safer GDALOpenEx API with explicit driver list
        char* const azDrvList[] = {(char*) "Zarr", nullptr};

        CPLDebug("EOPFZARR", "Attempting to open with Zarr driver: %s", zarrPath.c_str());

        poUnderlyingDS = static_cast<GDALDataset*>(
            GDALOpenEx(zarrPath.c_str(),
                       poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                       azDrvList,  // Explicitly use Zarr driver
                       papszOpenOptions,
                       nullptr));

        // If the formatted version failed, try the original path
        if (!poUnderlyingDS)
        {
            CPLDebug(
                "EOPFZARR", "Formatted path failed, trying original path: %s", mainPath.c_str());
            poUnderlyingDS = static_cast<GDALDataset*>(
                GDALOpenEx(mainPath.c_str(),
                           poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                           azDrvList,
                           papszOpenOptions,
                           nullptr));
        }

        // If both approaches failed and this is a URL, try without /vsicurl/ prefix
        if (!poUnderlyingDS && STARTS_WITH_CI(mainPath.c_str(), "/vsicurl/"))
        {
            std::string directUrl = mainPath.substr(9);  // Remove "/vsicurl/" prefix
            CPLDebug("EOPFZARR", "VSI path failed, trying direct URL: %s", directUrl.c_str());

            std::string directZarrPath = "ZARR:\"" + directUrl + "\"";
            poUnderlyingDS = static_cast<GDALDataset*>(
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

    // Detect if this is a remote dataset (for PAM save warning suppression)
    bool bIsRemoteDataset = IsUrlOrVirtualPath(mainPath);
    CPLDebug("EOPFZARR", "Dataset is %s", bIsRemoteDataset ? "REMOTE" : "LOCAL");

    // Check if this is a GRD product and multi-band mode is enabled
    // Default is YES for GRD products unless explicitly disabled
    const char* pszGrdMultiband =
        CSLFetchNameValueDef(poOpenInfo->papszOpenOptions, "GRD_MULTIBAND", "YES");
    bool bGrdMultiband = CPLTestBool(pszGrdMultiband);

    // For root datasets (not subdatasets), check if this is a GRD product
    if (!isSubdataset && bGrdMultiband && IsGRDProduct(mainPath))
    {
        CPLDebug("EOPFZARR", "Detected GRD product, attempting multi-band mode");

        // Create a temporary wrapper to get subdatasets
        std::unique_ptr<EOPFZarrDataset> tempDS(
            EOPFZarrDataset::Create(poUnderlyingDS, gEOPFDriver, nullptr, bIsRemoteDataset));

        if (tempDS)
        {
            // Find GRD polarization subdatasets
            auto polPaths = FindGRDPolarizations(tempDS.get(), mainPath);

            if (polPaths.size() >= 2)
            {
                CPLDebug("EOPFZARR",
                         "Found %d polarizations, creating multi-band dataset",
                         (int) polPaths.size());

                // Release the temporary dataset (it owns poUnderlyingDS)
                tempDS.reset();

                // Create multi-band dataset
                EOPFZarrMultiBandDataset* poMultiDS =
                    EOPFZarrMultiBandDataset::CreateFromPolarizations(
                        mainPath, polPaths, gEOPFDriver, bIsRemoteDataset);

                if (poMultiDS)
                {
                    poMultiDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");
                    return poMultiDS;
                }
                else
                {
                    CPLDebug("EOPFZARR",
                             "Failed to create multi-band dataset, falling back to standard mode");
                    // Re-open the underlying dataset since tempDS closed it
                    char* const azDrvList[] = {(char*) "Zarr", nullptr};
                    std::string zarrPath = "ZARR:\"" + mainPath + "\"";
                    poUnderlyingDS = static_cast<GDALDataset*>(
                        GDALOpenEx(zarrPath.c_str(),
                                   poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                   azDrvList,
                                   nullptr,
                                   nullptr));
                    if (!poUnderlyingDS)
                    {
                        CPLError(CE_Failure,
                                 CPLE_OpenFailed,
                                 "Failed to re-open dataset after multi-band attempt");
                        return nullptr;
                    }
                }
            }
            else
            {
                CPLDebug("EOPFZARR",
                         "Found only %d polarization(s), using standard mode",
                         (int) polPaths.size());
                // tempDS will be destroyed, need to get the underlying dataset back
                // Actually, we need to keep tempDS and return it
                tempDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");
                return tempDS.release();
            }
        }
    }

    // Create our wrapper dataset (standard mode)
    // Pass subdataset path so it can be set before metadata loading
    // Pass remote flag to control PAM save behavior
    EOPFZarrDataset* poDS = EOPFZarrDataset::Create(poUnderlyingDS,
                                                    gEOPFDriver,
                                                    isSubdataset ? subdatasetPath.c_str() : nullptr,
                                                    bIsRemoteDataset);
    if (poDS)
    {
        poDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");
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

    // Use .eopfzarr as primary extension to avoid conflicts with built-in Zarr
    // This ensures rasterio can discover our driver without interfering with original Zarr
    driver->SetMetadataItem(GDAL_DMD_EXTENSION, "eopfzarr");

    // Support both .eopfzarr and .zarr extensions, but prioritize .eopfzarr
    driver->SetMetadataItem(GDAL_DMD_EXTENSIONS, "eopfzarr zarr");

    // Add connection prefix for explicit driver usage
    driver->SetMetadataItem(GDAL_DMD_CONNECTION_PREFIX, "EOPFZARR:");

    driver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");
    driver->SetMetadataItem(GDAL_DMD_SUBDATASETS, "YES");

    // Add our own EOPF_PROCESS option, SUPPRESS_AUX_WARNING, and GRD_MULTIBAND options
    const char* pszOptions =
        "<OpenOptionList>"
        "  <Option name='EOPF_PROCESS' type='boolean' default='NO' description='Enable EOPF "
        "features'>"
        "    <Value>YES</Value>"
        "    <Value>NO</Value>"
        "  </Option>"
        "  <Option name='SUPPRESS_AUX_WARNING' type='boolean' default='YES' description='Suppress "
        "auxiliary file (.aux.xml) save warnings for remote datasets'>"
        "    <Value>YES</Value>"
        "    <Value>NO</Value>"
        "  </Option>"
        "  <Option name='GRD_MULTIBAND' type='boolean' default='YES' description='For Sentinel-1 "
        "GRD products, combine polarization bands (VV/VH or HH/HV) into a single multi-band "
        "dataset'>"
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
