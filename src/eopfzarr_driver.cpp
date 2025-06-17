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

    // Check for quoted path format: "path":subds
    size_t startQuote = pathWithoutPrefix.find('\"');
    if (startQuote != std::string::npos) {
        size_t endQuote = pathWithoutPrefix.find('\"', startQuote + 1);
        if (endQuote != std::string::npos && endQuote > startQuote + 1) {
            // We have a quoted path - extract it
            mainPath = pathWithoutPrefix.substr(startQuote + 1, endQuote - startQuote - 1);

            // Now check if there's a subdataset part after the quoted path
            if (endQuote + 1 < pathWithoutPrefix.length() && pathWithoutPrefix[endQuote + 1] == ':') {
                // We have a subdataset part - everything after the colon
                subdatasetPath = pathWithoutPrefix.substr(endQuote + 2);
                CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found quoted path with subdataset - Main: %s, Subds: %s",
                    mainPath.c_str(), subdatasetPath.c_str());

                // Fix Windows paths - replace forward slashes with backward slashes
#ifdef _WIN32
                // Replace forward slashes with backslashes
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
#endif
                return true;
            }
            // No subdataset part, just a quoted path
            else {
                CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found quoted path without subdataset - Main: %s", mainPath.c_str());
                subdatasetPath = "";
#ifdef _WIN32
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
#endif
                return false;
            }
        }
    }

    // Check for simple path with subdataset separator (e.g., EOPFZARR:path:subds)
    // This is complicated on Windows due to drive letters (C:)
    std::string tmpPath = pathWithoutPrefix;
    size_t colonPos = tmpPath.find(':');

#ifdef _WIN32
    // On Windows, the first colon might be the drive letter
    if (colonPos != std::string::npos && colonPos == 1) {
        // This is likely a drive letter - look for another colon
        colonPos = tmpPath.find(':', colonPos + 1);
    }
#endif

    if (colonPos != std::string::npos) {
        mainPath = tmpPath.substr(0, colonPos);
        subdatasetPath = tmpPath.substr(colonPos + 1);
        CPLDebug("EOPFZARR", "ParseSubdatasetPath: Found simple path with subdataset - Main: %s, Subds: %s",
            mainPath.c_str(), subdatasetPath.c_str());

#ifdef _WIN32
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
#endif
        return true;
    }

    // Not a subdataset path
    mainPath = pathWithoutPrefix;
    subdatasetPath = "";

#ifdef _WIN32
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
    if (poOpenInfo->eAccess == GA_Update)
    {
        CPLDebug("EOPFZARR", "EOPFIdentify: Update mode not supported");
        return FALSE;
    }

    const char* pszFilename = poOpenInfo->pszFilename;

    // Check if it's a subdataset reference with our driver prefix
    if (STARTS_WITH_CI(pszFilename, "EOPFZARR:")) {
        CPLDebug("EOPFZARR", "EOPFIdentify: Found EOPFZARR prefix, accepting");
        return TRUE;
    }

    // Check if the open option EOPF_PROCESS = YES is explicitly set
    bool bEOPFProcessFlag = false;
    const char* pszEOPFProcess = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");

    if (pszEOPFProcess && EQUAL(pszEOPFProcess, "YES"))
    {
        bEOPFProcessFlag = true;
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS flag is set to YES, processing file %s", pszFilename);
        return TRUE;
    }
    else if (pszEOPFProcess && EQUAL(pszEOPFProcess, "NO"))
    {
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS=NO option explicitly disables EOPF processing");
        return FALSE;
    }

    // If the path has specific patterns that indicate EOPF content, check them
    if (IsEOPFZarr(pszFilename))
    {
        CPLDebug("EOPFZARR", "EOPFIdentify: Dataset identified as EOPF Zarr by content");
        return TRUE;
    }

    // No EOPF markers found
    CPLDebug("EOPFZARR", "EOPFIdentify: Not identified as an EOPF dataset");
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      Open — delegate to the core Zarr driver, then wrap it            */
/* -------------------------------------------------------------------- */
static GDALDataset* EOPFOpen(GDALOpenInfo* poOpenInfo)
{
    const char* pszFilename = poOpenInfo->pszFilename;
    CPLDebug("EOPFZARR", "EOPFOpen: Opening file: %s", pszFilename);

    // Parse the filename to check if it's a subdataset reference
    std::string mainPath, subdatasetPath;
    bool isSubdataset = ParseSubdatasetPath(pszFilename, mainPath, subdatasetPath);

    // If this is a subdataset path with our prefix, strip the prefix - this should already be handled in ParseSubdatasetPath
    if (STARTS_WITH_CI(mainPath.c_str(), "EOPFZARR:")) {
        mainPath = mainPath.substr(9); // Skip "EOPFZARR:"
        CPLDebug("EOPFZARR", "EOPFOpen: Removed EOPFZARR prefix, main path: %s", mainPath.c_str());
    }

    CPLDebug("EOPFZARR", "EOPFOpen: After parsing - Main path: %s, Subdataset path: %s",
        mainPath.c_str(), subdatasetPath.c_str());

    // Verify that the main path exists
    VSIStatBufL sStat;
    if (VSIStatL(mainPath.c_str(), &sStat) != 0) {
        CPLDebug("EOPFZARR", "EOPFOpen: Main path does not exist: %s", mainPath.c_str());
        CPLError(CE_Failure, CPLE_OpenFailed, "EOPFZARR driver: Main path '%s' does not exist", mainPath.c_str());
        return nullptr;
    }

    // Create option list for zarr driver - DO NOT pass EOPF_PROCESS to underlying driver
    char** papszOpenOptionsCopy = nullptr;
    for (char** papszIter = poOpenInfo->papszOpenOptions; papszIter && *papszIter; ++papszIter)
    {
        char* pszKey = nullptr;
        const char* pszValue = CPLParseNameValue(*papszIter, &pszKey);
        if (pszKey && !EQUAL(pszKey, "EOPF_PROCESS"))
        {
            papszOpenOptionsCopy = CSLSetNameValue(papszOpenOptionsCopy, pszKey, pszValue);
        }
        CPLFree(pszKey);
    }

    // Use a config option (not open option) to temporarily disable EOPFZARR driver
    // Store the previous value first
    const char* pszOldSkip = CPLGetConfigOption("GDAL_SKIP", nullptr);
    CPLString osOldSkip = pszOldSkip ? pszOldSkip : "";

    // Add EOPFZARR to the list of skipped drivers
    CPLString osNewSkip = osOldSkip;
    if (!osNewSkip.empty()) {
        osNewSkip += ",";
    }
    osNewSkip += "EOPFZARR";

    // Set the new GDAL_SKIP config option
    CPLSetConfigOption("GDAL_SKIP", osNewSkip.c_str());

    // Different handling for main dataset vs subdataset
    GDALDataset* poUnderlyingDataset = nullptr;

    if (!isSubdataset || subdatasetPath.empty()) {
        // Open the Zarr dataset directly, explicitly requiring the Zarr driver
        char* const azDrvList[] = { (char*)"Zarr", nullptr };

        CPLDebug("EOPFZARR", "EOPFOpen: Opening main dataset with core Zarr driver: %s", mainPath.c_str());
        poUnderlyingDataset = static_cast<GDALDataset*>(
            GDALOpenEx(mainPath.c_str(),
                poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                azDrvList,
                papszOpenOptionsCopy,
                nullptr));
    }
    else {
        // For subdatasets, we need to:
        // 1. Open the main dataset with Zarr driver
        // 2. Get the subdataset path from it
        // 3. Open the subdataset

        // First open main dataset
        char* const azDrvList[] = { (char*)"Zarr", nullptr };
        CPLDebug("EOPFZARR", "EOPFOpen: Opening parent dataset for subdataset: %s", mainPath.c_str());

        GDALDataset* poParentDS = static_cast<GDALDataset*>(
            GDALOpenEx(mainPath.c_str(),
                poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                azDrvList,
                papszOpenOptionsCopy,
                nullptr));

        if (poParentDS) {
            // Find the subdataset with matching path
            char** papszSubdatasets = poParentDS->GetMetadata("SUBDATASETS");
            if (papszSubdatasets) {
                CPLString zarrSubdsPath;
                bool foundSubds = false;

                CPLDebug("EOPFZARR", "EOPFOpen: Looking for subdataset path: %s", subdatasetPath.c_str());

                // Loop through all subdatasets to find the one we want
                for (int i = 0; papszSubdatasets[i] != nullptr; i++) {
                    if (strstr(papszSubdatasets[i], "_NAME=") == nullptr) {
                        continue;  // Skip DESC entries
                    }

                    // Extract subdataset path
                    char* pszKey = nullptr;
                    const char* pszSubdsPath = CPLParseNameValue(papszSubdatasets[i], &pszKey);
                    if (pszKey && pszSubdsPath) {
                        CPLDebug("EOPFZARR", "EOPFOpen: Checking subdataset %s = %s", pszKey, pszSubdsPath);

                        // If it's a ZARR: path, extract the subdataset portion
                        CPLString testPath(pszSubdsPath);
                        if (STARTS_WITH_CI(testPath, "ZARR:")) {
                            // Extract the subdataset path from ZARR:"path":subdspath
                            size_t pathEndPos = testPath.find("\":", 5); // Look for end of quoted path
                            if (pathEndPos != std::string::npos) {
                                CPLString extractedSubdsPath = testPath.substr(pathEndPos + 2); // +2 to skip ":

                                CPLDebug("EOPFZARR", "EOPFOpen: Extracted subdataset path: %s", extractedSubdsPath.c_str());

                                // Check if this matches what we're looking for
                                if (extractedSubdsPath == subdatasetPath) {
                                    zarrSubdsPath = pszSubdsPath;
                                    foundSubds = true;
                                    CPLDebug("EOPFZARR", "Found matching subdataset: %s", zarrSubdsPath.c_str());
                                    break;
                                }
                            }
                        }
                    }
                    CPLFree(pszKey);
                }

                // If we found the subdataset, create a direct access to it
                if (foundSubds) {
                    CPLDebug("EOPFZARR", "Opening subdataset with Zarr driver: %s", zarrSubdsPath.c_str());

                    // IMPORTANT: Here's the fix - don't call GDALOpen directly which can cause recursion issues
                    // Instead use GDALOpenEx with explicit Zarr driver and additional options to prevent recursion

                    // Extract the actual subdataset path without ZARR: prefix for direct access
                    CPLString directSubdsPath = mainPath + subdatasetPath;
                    CPLDebug("EOPFZARR", "Attempting direct access to subdataset at: %s", directSubdsPath.c_str());

                    // Open the subdataset directly
                    poUnderlyingDataset = static_cast<GDALDataset*>(
                        GDALOpenEx(directSubdsPath.c_str(),
                            poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                            azDrvList,
                            papszOpenOptionsCopy,
                            nullptr));

                    // If direct access fails, try using the original Zarr subdataset path
                    if (!poUnderlyingDataset) {
                        CPLDebug("EOPFZARR", "Direct access failed, trying with original Zarr path");

                        // Save the current error handler and temporarily disable error output
                        CPLErrorHandler oldHandler = CPLSetErrorHandler(CPLQuietErrorHandler);

                        // Try opening with the original Zarr subdataset path
                        poUnderlyingDataset = static_cast<GDALDataset*>(
                            GDALOpenEx(zarrSubdsPath.c_str(),
                                poOpenInfo->nOpenFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                azDrvList,
                                nullptr, // No open options to avoid recursion issues
                                nullptr));

                        // Restore the original error handler
                        CPLSetErrorHandler(oldHandler);
                    }
                }
                else {
                    CPLDebug("EOPFZARR", "No matching subdataset found for path: %s", subdatasetPath.c_str());
                }
            }
            else {
                CPLDebug("EOPFZARR", "No subdatasets found in parent dataset");
            }

            // Clean up parent dataset if we're done with it
            GDALClose(poParentDS);
        }
        else {
            CPLDebug("EOPFZARR", "Failed to open parent dataset: %s", mainPath.c_str());
        }
    }

    // Restore the original GDAL_SKIP config option
    CPLSetConfigOption("GDAL_SKIP", osOldSkip.empty() ? nullptr : osOldSkip.c_str());

    // Clean up
    CSLDestroy(papszOpenOptionsCopy);

    if (!poUnderlyingDataset)
    {
        CPLDebug("EOPFZARR", "EOPFOpen: Core Zarr driver could not open %s %s",
            mainPath.c_str(), isSubdataset ? "or subdataset not found" : "");
        CPLError(CE_Failure, CPLE_OpenFailed,
            "EOPFZARR driver: Core Zarr driver could not open %s%s%s",
            mainPath.c_str(),
            isSubdataset ? " or subdataset " : "",
            isSubdataset ? subdatasetPath.c_str() : "");
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
    if (poDS != nullptr)
    {
        poDS->SetMetadataItem("EOPFZARR_WRAPPER", "YES", "EOPF");

        // If this was a subdataset, store the subdataset path for reference
        if (isSubdataset && !subdatasetPath.empty()) {
            poDS->SetMetadataItem("SUBDATASET_PATH", subdatasetPath.c_str());
        }
    }

    return poDS;
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
    GDALDriverManager* poDM = GetGDALDriverManager();
    GDALDriver* poZarrDriver = poDM->GetDriverByName("Zarr");
    if (poZarrDriver == nullptr)
    {
        CPLDebug("EOPFZARR", "Core Zarr driver not registered. Trying to register it.");
        GDALRegister_Zarr();

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
    gEOPFDriver->SetMetadataItem(GDAL_DMD_SUBDATASETS, "YES");

    // Register the open options with the correct format
    const char* pszOpenOptList =
        "<OpenOptionList>"
        "  <Option name='EOPF_PROCESS' type='string-select' default='AUTO' description='Force EOPF processing'>"
        "    <Value>YES</Value>"
        "    <Value>NO</Value>"
        "    <Value>AUTO</Value>"
        "  </Option>"
        "</OpenOptionList>";

    gEOPFDriver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, pszOpenOptList);

    gEOPFDriver->pfnIdentify = EOPFIdentify;
    gEOPFDriver->pfnOpen = EOPFOpen;

    // Register the driver after the core Zarr driver
    // Note: RegisterDriver appends to the end of the list, ensuring it's after Zarr
    poDM->RegisterDriver(gEOPFDriver);
    CPLDebug("EOPFZARR", "EOPFZarr driver registered with subdataset support");
}

extern "C" EOPFZARR_DLL void GDALRegisterMe()
{
    GDALRegister_EOPFZarr();
}
