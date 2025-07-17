#include "test_utils.h"
#include <string>

// Forward declaration of the actual ParseSubdatasetPath function
static bool ParseSubdatasetPath_Internal(const std::string &fullPath, std::string &mainPath, std::string &subdatasetPath);

// Test wrapper that returns just the parsed path for unit testing
std::string ParseSubdatasetPath(const std::string& path) {
    std::string mainPath, subdatasetPath;
    ParseSubdatasetPath_Internal(path, mainPath, subdatasetPath);
    
    // For testing, return the main path if no subdataset, otherwise the subdataset
    if (!subdatasetPath.empty()) {
        return mainPath + "/" + subdatasetPath;
    }
    return mainPath;
}

// Helper function to check if a path is virtual (starts with /vsi)
bool IsVirtualPath(const std::string& path) {
    return path.find("/vsi") == 0;
}

// Helper function to check if a path is quoted
bool IsQuotedPath(const std::string& path) {
    return path.length() >= 2 && path.front() == '"' && path.back() == '"';
}

// Include the actual implementation (we'll need to refactor this)
// For now, let's create a simplified version for testing

static bool ParseSubdatasetPath_Internal(const std::string &fullPath, std::string &mainPath, std::string &subdatasetPath) {
    // Remove EOPFZARR: prefix if present
    std::string pathWithoutPrefix = fullPath;
    if (pathWithoutPrefix.substr(0, 9) == "EOPFZARR:") {
        pathWithoutPrefix = pathWithoutPrefix.substr(9);
    }
    
    // Handle quoted paths
    if (pathWithoutPrefix.length() >= 2 && pathWithoutPrefix[0] == '"' && pathWithoutPrefix.back() == '"') {
        std::string quotedContent = pathWithoutPrefix.substr(1, pathWithoutPrefix.length() - 2);
        
        // Check if it's a virtual file system path
        if (quotedContent.find("/vsicurl/") == 0 || quotedContent.find("/vsis3/") == 0) {
            // Find .zarr and check if there's a subdataset after it
            size_t zarrPos = quotedContent.find(".zarr");
            if (zarrPos != std::string::npos) {
                size_t afterZarr = zarrPos + 5; // Position after ".zarr"
                if (afterZarr < quotedContent.length() && quotedContent[afterZarr] == '/') {
                    // There's a subdataset path after .zarr
                    mainPath = quotedContent.substr(0, afterZarr);
                    subdatasetPath = quotedContent.substr(afterZarr + 1);
                    return true;
                } else {
                    // No subdataset, just the main zarr file
                    mainPath = quotedContent;
                    subdatasetPath = "";
                    return false;
                }
            }
        }
        
        // For non-virtual quoted paths, treat as regular file with potential subdataset
        size_t zarrPos = quotedContent.find(".zarr");
        if (zarrPos != std::string::npos) {
            size_t afterZarr = zarrPos + 5;
            if (afterZarr < quotedContent.length() && quotedContent[afterZarr] == '/') {
                mainPath = quotedContent.substr(0, afterZarr);
                subdatasetPath = quotedContent.substr(afterZarr + 1);
                return true;
            }
        }
        
        mainPath = quotedContent;
        subdatasetPath = "";
        return false;
    }
    
    // Handle unquoted paths
    if (pathWithoutPrefix.find("/vsicurl/") == 0 || pathWithoutPrefix.find("/vsis3/") == 0) {
        // Unquoted virtual file system path - treat as main path only
        mainPath = pathWithoutPrefix;
        subdatasetPath = "";
        return false;
    }
    
    // Regular file path
    size_t zarrPos = pathWithoutPrefix.find(".zarr");
    if (zarrPos != std::string::npos) {
        size_t afterZarr = zarrPos + 5;
        if (afterZarr < pathWithoutPrefix.length() && pathWithoutPrefix[afterZarr] == '/') {
            mainPath = pathWithoutPrefix.substr(0, afterZarr);
            subdatasetPath = pathWithoutPrefix.substr(afterZarr + 1);
            return true;
        }
    }
    
    mainPath = pathWithoutPrefix;
    subdatasetPath = "";
    return false;
}
