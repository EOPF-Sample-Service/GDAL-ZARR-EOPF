#pragma once
#include "gdal_priv.h"

namespace EOPFZarrRegistry
{
    /**
     * @brief Handles GDAL driver registration and lifecycle management
     */
    class DriverRegistry
    {
    public:
    /**
     * @brief Register the EOPF Zarr driver with GDAL
     * @param pfnIdentify Pointer to the identify function
     * @param pfnOpen Pointer to the open function
     * @return true if registration successful
     */
    static bool RegisterDriver(
        int (*pfnIdentify)(GDALOpenInfo*),
        GDALDataset* (*pfnOpen)(GDALOpenInfo*));

        /**
         * @brief Deregister the EOPF Zarr driver
         */
        static void DeregisterDriver();

        /**
         * @brief Get the registered driver instance
         * @return Driver pointer or nullptr if not registered
         */
        static GDALDriver* GetDriver();

        /**
         * @brief Check if driver is already registered
         * @return true if registered
         */
        static bool IsRegistered();

    private:
        static GDALDriver* s_driver;
        static void SetupDriverMetadata(GDALDriver* driver);
    };
}
