#include "eopfzarr_path_utils.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include <algorithm>

namespace EOPFPathUtils
{
    PathParser::ParsedPath PathParser::Parse(const std::string& fullPath)
    {
        ParsedPath result;
        result.isSubdataset = false;
        result.isUrl = false;
        result.isVirtualPath = false;

        // First, check for EOPFZARR: prefix
        std::string pathWithoutPrefix = StripEOPFPrefix(fullPath);
        
        // Handle quoted paths first - extract the main path from quotes
        std::string mainPathForChecking = pathWithoutPrefix;
        
        // Check for quoted path format: "path":subds or just "path"
        size_t startQuote = pathWithoutPrefix.find('\"');
        if (startQuote != std::string::npos)
        {
            size_t endQuote = pathWithoutPrefix.find('\"', startQuote + 1);
            if (endQuote != std::string::npos && endQuote > startQuote + 1)
            {
                // Extract the main path from quotes for URL/virtual path checking
                mainPathForChecking = pathWithoutPrefix.substr(startQuote + 1, endQuote - startQuote - 1);
            }
        }
        
        // Check if this is a URL or virtual path using the unquoted path
        result.isUrl = IsUrlOrVirtualPath(mainPathForChecking);
        result.isVirtualPath = result.isUrl || STARTS_WITH_CI(mainPathForChecking.c_str(), "/vsi");

        if (result.isUrl || result.isVirtualPath)
        {
            result.mainPath = mainPathForChecking;  // Use unquoted path
            result.subdatasetPath = "";
            
            // For quoted URLs with subdatasets, combine the paths
            if (startQuote != std::string::npos)
            {
                size_t endQuote = pathWithoutPrefix.find('\"', startQuote + 1);
                if (endQuote != std::string::npos && endQuote + 1 < pathWithoutPrefix.length() && pathWithoutPrefix[endQuote + 1] == ':')
                {
                    std::string subdatasetPart = pathWithoutPrefix.substr(endQuote + 2);
                    
                    // For URLs, combine main path and subdataset path directly
                    // Remove leading slash from subdataset part if present
                    if (!subdatasetPart.empty() && subdatasetPart[0] == '/')
                    {
                        subdatasetPart = subdatasetPart.substr(1);
                    }
                    
                    // Ensure main path ends with slash
                    if (!result.mainPath.empty() && result.mainPath.back() != '/')
                    {
                        result.mainPath += '/';
                    }
                    
                    // Combine the paths
                    result.mainPath += subdatasetPart;
                    result.isSubdataset = false;  // Treat as single main path for URLs
                }
            }
            return result;
        }

        // Handle remaining quoted paths (local files with potential subdatasets)
        if (startQuote != std::string::npos)
        {
            size_t endQuote = pathWithoutPrefix.find('\"', startQuote + 1);
            if (endQuote != std::string::npos && endQuote > startQuote + 1)
            {
                // We have a quoted path - extract it
                result.mainPath = pathWithoutPrefix.substr(startQuote + 1, endQuote - startQuote - 1);

                // Check if there's a subdataset part after the quoted path
                if (endQuote + 1 < pathWithoutPrefix.length() && pathWithoutPrefix[endQuote + 1] == ':')
                {
                    // We have a subdataset part
                    result.subdatasetPath = pathWithoutPrefix.substr(endQuote + 2);
                    result.isSubdataset = true;
                    
                    NormalizeWindowsPath(result.mainPath);
                    return result;
                }
                else
                {
                    // No subdataset part, just a quoted path
                    result.subdatasetPath = "";
                    NormalizeWindowsPath(result.mainPath);
                    return result;
                }
            }
        }

        // Check for simple path with subdataset separator (e.g., EOPFZARR:path:subds)
        // This is complicated on Windows due to drive letters (C:)
        std::string tmpPath = pathWithoutPrefix;
        size_t colonPos = tmpPath.find(':');

#ifdef _WIN32
        // On Windows, the first colon might be the drive letter
        if (colonPos != std::string::npos && colonPos == 1)
        {
            // This is likely a drive letter - look for another colon
            colonPos = tmpPath.find(':', colonPos + 1);
        }
#endif

        if (colonPos != std::string::npos)
        {
            result.mainPath = tmpPath.substr(0, colonPos);
            result.subdatasetPath = tmpPath.substr(colonPos + 1);
            result.isSubdataset = true;
            
            NormalizeWindowsPath(result.mainPath);
            return result;
        }

        // Not a subdataset path
        result.mainPath = pathWithoutPrefix;
        result.subdatasetPath = "";
        
        NormalizeWindowsPath(result.mainPath);
        return result;
    }

    bool PathParser::IsUrlOrVirtualPath(const std::string& path)
    {
        // Check for URL schemes
        if (path.find("://") != std::string::npos)
        {
            return true;
        }

        // Check for GDAL virtual file systems
        if (STARTS_WITH_CI(path.c_str(), "/vsi"))
        {
            return true;
        }

        return false;
    }

    std::string PathParser::CreateQGISCompatiblePath(const std::string& path)
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

    std::string PathParser::StripEOPFPrefix(const std::string& path)
    {
        const char* pszPrefix = "EOPFZARR:";
        if (STARTS_WITH_CI(path.c_str(), pszPrefix))
        {
            return path.substr(strlen(pszPrefix));
        }
        return path;
    }

    void PathParser::NormalizeWindowsPath(std::string& path)
    {
#ifdef _WIN32
        // Replace forward slashes with backslashes
        for (size_t i = 0; i < path.length(); ++i)
        {
            if (path[i] == '/')
            {
                path[i] = '\\';
            }
        }

        // Remove leading slash if present in Windows paths (e.g., /C:/...)
        if (!path.empty() && path[0] == '\\' &&
            path.length() > 2 && path[1] != '\\' && path[2] == ':')
        {
            path = path.substr(1);
        }

        // Remove trailing slash if present
        if (!path.empty() && path.back() == '\\')
        {
            path.pop_back();
        }
#endif
    }
}
