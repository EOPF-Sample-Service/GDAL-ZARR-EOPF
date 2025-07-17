#include "eopfzarr_opener.h"
#include "eopfzarr_errors.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include <memory>

namespace EOPFZarrOpener
{
    GDALDataset* DatasetOpener::OpenMainDataset(
        const std::string& path,
        unsigned int openFlags,
        char** originalOptions)
    {
        // Filter open options
        char** openOptions = FilterOpenOptions(originalOptions);
        
        // Prepare the path for the underlying Zarr driver
        std::string zarrPath = path;

        // If this is a virtual file system path, format it properly for the Zarr driver
        if (STARTS_WITH_CI(path.c_str(), "/vsi"))
        {
            // For URLs, we need to be careful about path formatting
            // The Zarr driver expects virtual file system paths to be quoted and prefixed with ZARR:
            EOPFErrorUtils::ErrorHandler::Debug("Detected virtual file system path, formatting for Zarr driver");

            // Ensure the path uses forward slashes (important for URLs even on Windows)
            std::string normalizedPath = path;
            // No need to change slashes in URLs - they should already be correct

            zarrPath = "ZARR:\"" + normalizedPath + "\"";
            EOPFErrorUtils::ErrorHandler::Debug(std::string("Formatted Zarr path: ") + zarrPath);
        }

        // Use safer GDALOpenEx API with explicit driver list
        char* const azDrvList[] = {(char*)"Zarr", nullptr};

        EOPFErrorUtils::ErrorHandler::Debug(std::string("Attempting to open with Zarr driver: ") + zarrPath);

        GDALDataset* poUnderlyingDS = static_cast<GDALDataset*>(
            GDALOpenEx(zarrPath.c_str(),
                       openFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                       azDrvList, // Explicitly use Zarr driver
                       openOptions,
                       nullptr));

        // If the formatted version failed, try the original path
        if (!poUnderlyingDS)
        {
            EOPFErrorUtils::ErrorHandler::Debug(std::string("Formatted path failed, trying original path: ") + path);
            poUnderlyingDS = static_cast<GDALDataset*>(
                GDALOpenEx(path.c_str(),
                           openFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                           azDrvList,
                           openOptions,
                           nullptr));
        }

        // If both approaches failed and this is a URL, try without /vsicurl/ prefix
        if (!poUnderlyingDS && STARTS_WITH_CI(path.c_str(), "/vsicurl/"))
        {
            std::string directUrl = path.substr(9); // Remove "/vsicurl/" prefix
            EOPFErrorUtils::ErrorHandler::Debug(std::string("VSI path failed, trying direct URL: ") + directUrl);

            std::string directZarrPath = "ZARR:\"" + directUrl + "\"";
            poUnderlyingDS = static_cast<GDALDataset*>(
                GDALOpenEx(directZarrPath.c_str(),
                           openFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                           azDrvList,
                           openOptions,
                           nullptr));
        }

        CSLDestroy(openOptions);
        return poUnderlyingDS;
    }

    GDALDataset* DatasetOpener::OpenSubdataset(
        const std::string& mainPath,
        const std::string& subdatasetPath,
        unsigned int openFlags,
        char** originalOptions)
    {
        char** openOptions = FilterOpenOptions(originalOptions);
        char* const azDrvList[] = {(char*)"Zarr", nullptr};

        // Try direct path first - most reliable for QGIS
        std::string directPath = mainPath;
        if (!directPath.empty() && directPath.back() != '/' && directPath.back() != '\\')
#ifdef _WIN32
            directPath += '\\';
#else
            directPath += '/';
#endif
        directPath += subdatasetPath;

        EOPFErrorUtils::ErrorHandler::Debug(std::string("Attempting to open subdataset directly: ") + directPath);
        GDALDataset* poDS = static_cast<GDALDataset*>(GDALOpenEx(directPath.c_str(),
                                                                  openFlags | GDAL_OF_RASTER | GDAL_OF_READONLY, 
                                                                  azDrvList, openOptions, nullptr));

        if (poDS)
        {
            CSLDestroy(openOptions);
            return poDS;
        }

        // If direct access fails, try opening parent and then finding the subdataset
        EOPFErrorUtils::ErrorHandler::Debug("Direct access failed, trying through parent dataset");

        // Open parent dataset
        GDALDataset* poParentDS = static_cast<GDALDataset*>(GDALOpenEx(mainPath.c_str(),
                                                                        openFlags | GDAL_OF_RASTER | GDAL_OF_READONLY, 
                                                                        azDrvList, openOptions, nullptr));

        if (!poParentDS)
        {
            EOPFErrorUtils::ErrorHandler::Debug(std::string("Failed to open parent dataset: ") + mainPath);
            CSLDestroy(openOptions);
            return nullptr;
        }

        std::unique_ptr<GDALDataset> parentGuard(poParentDS);

        // Find the subdataset
        char** papszSubdatasets = poParentDS->GetMetadata("SUBDATASETS");
        if (!papszSubdatasets)
        {
            EOPFErrorUtils::ErrorHandler::Debug("No subdatasets found in parent dataset");
            CSLDestroy(openOptions);
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
                        // Found a matching subdataset, open it directly
                        EOPFErrorUtils::ErrorHandler::Debug(std::string("Found matching subdataset: ") + pszValue);

                        CPLErrorHandler oldHandler = CPLSetErrorHandler(CPLQuietErrorHandler);
                        poDS = static_cast<GDALDataset*>(GDALOpenEx(pszValue,
                                                                     openFlags | GDAL_OF_RASTER | GDAL_OF_READONLY,
                                                                     azDrvList, openOptions, nullptr));
                        CPLSetErrorHandler(oldHandler);

                        if (poDS)
                        {
                            CPLFree(pszKey);
                            CSLDestroy(openOptions);
                            return poDS;
                        }
                    }
                }
            }
            CPLFree(pszKey);
        }

        EOPFErrorUtils::ErrorHandler::ReportSubdatasetNotFound(subdatasetPath);
        CSLDestroy(openOptions);
        return nullptr;
    }

    std::string DatasetOpener::FormatZarrPath(const std::string& path)
    {
        if (STARTS_WITH_CI(path.c_str(), "/vsi"))
        {
            return "ZARR:\"" + path + "\"";
        }
        return path;
    }

    char** DatasetOpener::FilterOpenOptions(char** originalOptions)
    {
        char** filteredOptions = nullptr;
        for (char** papszIter = originalOptions; papszIter && *papszIter; ++papszIter)
        {
            char* pszKey = nullptr;
            const char* pszValue = CPLParseNameValue(*papszIter, &pszKey);
            if (pszKey && !EQUAL(pszKey, "EOPF_PROCESS"))
            {
                filteredOptions = CSLSetNameValue(filteredOptions, pszKey, pszValue);
            }
            CPLFree(pszKey);
        }
        return filteredOptions;
    }
}
