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
#include "cpl_string.h"
#include <string>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>
#include "eopf_metadata.h"
#include "eopfzarr_config.h"


#ifdef _WIN32
#define EOPFZARR_DLL __declspec(dllexport)
#else
#define EOPFZARR_DLL
#endif

// Thread-safe global variables
static std::atomic<GDALDriver*> gEOPFDriver{nullptr};
static std::once_flag gEOPFDriverInitFlag;
static std::mutex gCacheMutex;
static std::unordered_map<std::string, bool> gIsEOPFZarrCache;
static std::mutex gSkipMutex;

// Thread-safe GDAL_SKIP guard
class GDALSkipGuard {
private:
    CPLString m_osOldSkip;
    std::lock_guard<std::mutex> m_lock;

public:
    explicit GDALSkipGuard(const char* pszSkip) 
#ifdef _MSC_VER
        __pragma(warning(suppress: 26115)) // Suppress the warning about lock not being released
#endif
        : m_lock(gSkipMutex)
    {
        const char* pszOldSkip = CPLGetConfigOption("GDAL_SKIP", nullptr);
        m_osOldSkip = pszOldSkip ? pszOldSkip : "";
        
        CPLString osNewSkip = m_osOldSkip;
        if (!osNewSkip.empty()) {
            osNewSkip += ",";
        }
        osNewSkip += pszSkip;
        
        CPLSetConfigOption("GDAL_SKIP", osNewSkip.c_str());
    }

    ~GDALSkipGuard() {
        CPLSetConfigOption("GDAL_SKIP", m_osOldSkip.empty() ? nullptr : m_osOldSkip.c_str());
    }
    
    // Prevent copying
    GDALSkipGuard(const GDALSkipGuard&) = delete;
    GDALSkipGuard& operator=(const GDALSkipGuard&) = delete;
};

static bool HasFile(const std::string &path)
{
    VSIStatBufL sStat;
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
    // Thread-safe cache check
    {
        std::lock_guard<std::mutex> lock(gCacheMutex);
        auto it = gIsEOPFZarrCache.find(path);
        if (it != gIsEOPFZarrCache.end()) {
            return it->second;
        }
    }

    // Extract main path if this is a subdataset reference
    std::string mainPath, subdatasetPath;
    ParseSubdatasetPath(path, mainPath, subdatasetPath);

    // Use the main path for detection
    std::string pathToCheck = mainPath;
    bool isEOPF = false;

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
                    isEOPF = true;
                }
            }
        }
    }

    // 2. Check if it's a subdataset path with EOPFZARR prefix
    if (!isEOPF && STARTS_WITH_CI(path.c_str(), "EOPFZARR:")) {
        CPLDebug("EOPFZARR", "Path starts with EOPFZARR: prefix, accepting");
        isEOPF = true;
    }

    // Cache the result with mutex protection
    {
        std::lock_guard<std::mutex> lock(gCacheMutex);
        gIsEOPFZarrCache[path] = isEOPF;
    }

    return isEOPF;
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

    // Only identify in two very specific cases:
    
    // Case 1: Explicit EOPFZARR prefix (when someone explicitly wants this driver)
    if (STARTS_WITH_CI(pszFilename, "EOPFZARR:")) {
        CPLDebug("EOPFZARR", "EOPFIdentify: Found EOPFZARR prefix, accepting");
        return TRUE;
    }

    // Case 2: Explicit EOPF_PROCESS=YES option is provided
    const char* pszEOPFProcess = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "EOPF_PROCESS");
    if (pszEOPFProcess && 
        (EQUAL(pszEOPFProcess, "YES") || EQUAL(pszEOPFProcess, "TRUE") || EQUAL(pszEOPFProcess, "1")))
    {
        CPLDebug("EOPFZARR", "EOPFIdentify: EOPF_PROCESS option is set to YES, accepting");
        return TRUE;
    }
    
    // For all other cases, decline - let other drivers (including Zarr) handle them
    CPLDebug("EOPFZARR", "EOPFIdentify: No explicit EOPF selection, declining");
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

    // Parse the filename to check if it's a subdataset reference
    std::string mainPath, subdatasetPath;
    bool isSubdataset = ParseSubdatasetPath(pszFilename, mainPath, subdatasetPath);

    // If this is a subdataset path with our prefix, strip the prefix
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

    // Use thread-safe guard for GDAL_SKIP
    GDALDataset* poUnderlyingDataset = nullptr;
    {
        // This RAII guard handles the GDAL_SKIP setting and restoration in a thread-safe way
        GDALSkipGuard skipGuard("EOPFZARR");
        
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
            // For subdatasets, use the helper function for better handling
            poUnderlyingDataset = OpenSubdataset(mainPath, subdatasetPath, poOpenInfo->nOpenFlags, papszOpenOptionsCopy);
        }
        // skipGuard destructor will restore GDAL_SKIP here
    }

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

    // Get driver in thread-safe way
    GDALDriver* driver = gEOPFDriver.load();
    if (!driver)
    {
        CPLDebug("EOPFZARR", "EOPFOpen: gEOPFDriver is null. Cannot create EOPFZarrDataset.");
        CPLError(CE_Failure, CPLE_AppDefined, "EOPFZARR driver not properly initialized");
        GDALClose(poUnderlyingDataset);
        return nullptr;
    }

    // Create the wrapper dataset
    EOPFZarrDataset* poDS = EOPFZarrDataset::Create(poUnderlyingDataset, driver);

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
    // Add protection for the entire function, not just the internal code
    static std::mutex registrationMutex;
    std::lock_guard<std::mutex> lock(registrationMutex);

    // Already registered?
    if (GDALGetDriverByName("EOPFZARR") != nullptr)
        return;

    // Thread-safe driver registration using std::call_once
    // Note: We're keeping call_once as an additional safety measure
    std::call_once(gEOPFDriverInitFlag, []() {
        // Version check
        if (!GDAL_CHECK_VERSION("EOPFZARR"))
            return;

        // Make sure the Zarr driver is registered first
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
        
        // ==== INJECT THE OPTION DIRECTLY INTO THE ZARR DRIVER ====
        // Only add if the driver exists (extra safety against crashes)
        if (poZarrDriver) {
            const char* pszExistingOptions = poZarrDriver->GetMetadataItem(GDAL_DMD_OPENOPTIONLIST);
            
            // Only add if EOPF_PROCESS doesn't already exist
            if (pszExistingOptions == nullptr || strstr(pszExistingOptions, "EOPF_PROCESS") == nullptr) {
                const char* pszAddOption = 
                    "  <Option name='EOPF_PROCESS' type='boolean' default='NO' description='Enable EOPF features'>"
                    "    <Value>YES</Value>"
                    "    <Value>NO</Value>"
                    "  </Option>";
                    
                if (pszExistingOptions && strlen(pszExistingOptions) > 20) {
                    // Find where to insert our option
                    const char* pszInsertPoint = strstr(pszExistingOptions, "</OpenOptionList>");
                    if (pszInsertPoint) {
                        size_t insertPos = pszInsertPoint - pszExistingOptions;
                        std::string newOptions(pszExistingOptions);
                        newOptions.insert(insertPos, pszAddOption);
                        poZarrDriver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, newOptions.c_str());
                        CPLDebug("EOPFZARR", "Injected EOPF_PROCESS option into Zarr driver options");
                    }
                } else {
                    CPLString osOptions;
                    osOptions.Printf("<OpenOptionList>%s</OpenOptionList>", pszAddOption);
                    poZarrDriver->SetMetadataItem(GDAL_DMD_OPENOPTIONLIST, osOptions.c_str());
                    CPLDebug("EOPFZARR", "Created new options list with EOPF_PROCESS for Zarr driver");
                }
            }
            
            // Force this option to be visible in all UIs
            poZarrDriver->SetMetadataItem("EOPF_PROCESS_OPTION_AVAILABLE", "YES");
        }

        // ==== REGISTER OUR OWN DRIVER AS USUAL ====
        GDALDriver* driver = new GDALDriver();
        driver->SetDescription("EOPFZARR");
        driver->SetMetadataItem(GDAL_DMD_LONGNAME, "EOPF Zarr Wrapper Driver 3");
        driver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
        driver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
        driver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/eopfzarr.html");
        driver->SetMetadataItem(GDAL_DMD_SUBDATASETS, "YES");

        driver->pfnIdentify = EOPFIdentify;
        driver->pfnOpen = EOPFOpen;

        // Do the registration and store atomically
        if (poDM) {
            poDM->RegisterDriver(driver);
            gEOPFDriver.store(driver);
            CPLDebug("EOPFZARR", "EOPFZarr driver registered successfully");
        } else {
            // Emergency deletion if no driver manager available
            delete driver;
            CPLDebug("EOPFZARR", "Failed to get GDALDriverManager");
        }
    });
}

// Add an unload function for cleaner shutdown
// This is crucial to prevent crashes during shutdown
extern "C" EOPFZARR_DLL void GDALDeregisterDriverEOPF()
{
    static std::mutex deregistrationMutex;
    std::lock_guard<std::mutex> lock(deregistrationMutex);

    // Get our driver pointer safely
    GDALDriver* driver = gEOPFDriver.exchange(nullptr);
    if (driver) {
        // Already nulled out our global pointer, now remove from registry
        GDALDriverManager* poDM = GetGDALDriverManager();
        if (poDM) {
            CPLDebug("EOPFZARR", "Deregistering EOPFZarr driver");
            poDM->DeregisterDriver(driver);
        }
        // Note: GDAL will delete the driver, don't delete it here
    }

    // Clean up caches
    std::lock_guard<std::mutex> cacheLock(gCacheMutex);
    gIsEOPFZarrCache.clear();
    
    // Clean up GDAL finder resources that might be used by our driver
    CPLFinderClean();
}

#ifdef _WIN32
#ifdef _WIN32  
#include <windows.h> // Include this to define BOOL and other Windows types  
#endif  

static BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {  
    switch (fdwReason) {  
        case DLL_PROCESS_ATTACH:  
            // Perform actions needed when the DLL is loaded  
            break;  
        case DLL_THREAD_ATTACH:  
        case DLL_THREAD_DETACH:  
            // Perform actions needed when threads are created or destroyed  
            break;  
        case DLL_PROCESS_DETACH:  
            // Perform cleanup when the DLL is unloaded  
            break;  
    }  
    return TRUE;  
}
#endif

// Register the cleanup function for Unix/Linux platforms
class EOPFZarrCleanupManager {
public:
    EOPFZarrCleanupManager() {
        CPLDebug("EOPFZARR", "Registering EOPF driver cleanup");
    }
    
    ~EOPFZarrCleanupManager() {
        CPLDebug("EOPFZARR", "Running EOPF driver cleanup");
        GDALDeregisterDriverEOPF();
    }
};

// Create a singleton instance to handle cleanup
static EOPFZarrCleanupManager gEOPFZarrCleanupManager;

extern "C" EOPFZARR_DLL void GDALRegisterMe()
{
    GDALRegister_EOPFZarr();
}
