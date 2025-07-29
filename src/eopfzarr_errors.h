#pragma once
#include <string>

#include "cpl_error.h"

namespace EOPFErrorUtils
{
/**
 * @brief Centralized error handling for EOPF operations
 */
class ErrorHandler
{
  public:
    /**
     * @brief Report file not found error
     * @param path The path that wasn't found
     */
    static void ReportFileNotFound(const std::string& path);

    /**
     * @brief Report driver open failure
     * @param path The path that failed to open
     * @param reason Optional reason for failure
     */
    static void ReportOpenFailure(const std::string& path, const std::string& reason = "");

    /**
     * @brief Report subdataset not found
     * @param subdatasetPath The subdataset path
     */
    static void ReportSubdatasetNotFound(const std::string& subdatasetPath);

    /**
     * @brief Report wrapper creation failure
     * @param reason The reason for failure
     */
    static void ReportWrapperFailure(const std::string& reason);

    /**
     * @brief Debug log with EOPFZARR prefix
     * @param message The debug message
     */
    static void Debug(const std::string& message);

  private:
    static const char* DRIVER_NAME;
};
}  // namespace EOPFErrorUtils
