#pragma once
#include "gdal_priv.h"
#include "eopfzarr_path_utils.h"
#include <string>

namespace EOPFZarrOpener
{
    /**
     * @brief Factory class for opening EOPF Zarr datasets
     */
    class DatasetOpener
    {
    public:
        /**
         * @brief Open a main dataset (non-subdataset)
         * @param path The dataset path
         * @param openFlags GDAL open flags
         * @param openOptions Open options
         * @return Opened dataset or nullptr
         */
        static GDALDataset* OpenMainDataset(
            const std::string& path,
            unsigned int openFlags,
            char** openOptions);

        /**
         * @brief Open a subdataset
         * @param mainPath The main dataset path
         * @param subdatasetPath The subdataset path
         * @param openFlags GDAL open flags  
         * @param openOptions Open options
         * @return Opened dataset or nullptr
         */
        static GDALDataset* OpenSubdataset(
            const std::string& mainPath,
            const std::string& subdatasetPath,
            unsigned int openFlags,
            char** openOptions);

    private:
        static std::string FormatZarrPath(const std::string& path);
        static char** FilterOpenOptions(char** originalOptions);
    };
}
