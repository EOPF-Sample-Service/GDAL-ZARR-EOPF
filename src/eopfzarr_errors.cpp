#include "eopfzarr_errors.h"
#include "cpl_error.h"

namespace EOPFErrorUtils
{
    const char* ErrorHandler::DRIVER_NAME = "EOPFZARR";

    void ErrorHandler::ReportFileNotFound(const std::string& path)
    {
        CPLError(CE_Failure, CPLE_OpenFailed, 
                 "%s driver: Main path '%s' does not exist", 
                 DRIVER_NAME, path.c_str());
    }

    void ErrorHandler::ReportOpenFailure(const std::string& path, const std::string& reason)
    {
        if (reason.empty())
        {
            CPLError(CE_Failure, CPLE_OpenFailed, 
                     "%s driver could not open %s", 
                     DRIVER_NAME, path.c_str());
        }
        else
        {
            CPLError(CE_Failure, CPLE_OpenFailed, 
                     "%s driver could not open %s: %s", 
                     DRIVER_NAME, path.c_str(), reason.c_str());
        }
    }

    void ErrorHandler::ReportSubdatasetNotFound(const std::string& subdatasetPath)
    {
        CPLDebug(DRIVER_NAME, "No matching subdataset found for: %s", subdatasetPath.c_str());
    }

    void ErrorHandler::ReportWrapperFailure(const std::string& reason)
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "%s driver: %s", 
                 DRIVER_NAME, reason.c_str());
    }

    void ErrorHandler::Debug(const std::string& message)
    {
        CPLDebug(DRIVER_NAME, "%s", message.c_str());
    }
}
