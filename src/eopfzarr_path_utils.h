#pragma once
#include <string>

namespace EOPFPathUtils
{
/**
 * @brief Utility class for handling EOPF Zarr path operations
 */
class PathParser
{
  public:
    struct ParsedPath
    {
        std::string mainPath;
        std::string subdatasetPath;
        bool isSubdataset;
        bool isUrl;
        bool isVirtualPath;
    };

    /**
     * @brief Parse an EOPF path into components
     * @param fullPath The input path to parse
     * @return ParsedPath structure with all components
     */
    static ParsedPath Parse(const std::string& fullPath);

    /**
     * @brief Check if a path is a URL or virtual file system path
     * @param path The path to check
     * @return true if URL or virtual path
     */
    static bool IsUrlOrVirtualPath(const std::string& path);

    /**
     * @brief Create QGIS-compatible path format
     * @param path The input path
     * @return Formatted path for QGIS compatibility
     */
    static std::string CreateQGISCompatiblePath(const std::string& path);

  private:
    static std::string StripEOPFPrefix(const std::string& path);
    static void NormalizeWindowsPath(std::string& path);
};
}  // namespace EOPFPathUtils
