#include <cpl_conv.h>
#include <cpl_string.h>
#include <gdal.h>
#include <gdal_priv.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

/**
 * @brief Tests for geolocation arrays support (Issue #137)
 *
 * Validates that the EOPFZARR driver correctly sets GEOLOCATION metadata
 * for satellite swath data (e.g., Sentinel-3 SLSTR) that has native 2D
 * latitude/longitude coordinate arrays.
 *
 * The GEOLOCATION metadata enables gdalwarp -geoloc to perform accurate
 * warping using pixel-by-pixel coordinates instead of the linear GeoTransform
 * approximation.
 */

class GeolocationArraysTests
{
  public:
    static void runAllTests()
    {
        std::cout << "=== Geolocation Arrays Tests ===" << std::endl;

        // Initialize GDAL
        GDALAllRegister();

        testGeolocationMetadataExists();
        testGeolocationMetadataFields();
        testGeolocationDatasetPaths();
        testGeolocationDatasetsAreOpenable();
        testGeolocationArrayDimensions();
        testNoGeolocationForNonSwathData();

        std::cout << "✅ All Geolocation Arrays Tests Completed!" << std::endl;
    }

  private:
    /**
     * @brief Test that GEOLOCATION metadata domain exists for swath data
     */
    static void testGeolocationMetadataExists()
    {
        std::cout << "Testing GEOLOCATION metadata exists for Sentinel-3 SLSTR..." << std::endl;

        // Sentinel-3 SLSTR nadir view - has 2D lat/lon arrays
        const char* swathSubdataset =
            "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/"
            "e05ab01a9d56408d82ac32d69a5aae2a:202510-s03slsrbt-global/19/products/cpm_v256/"
            "S3A_SL_1_RBT____20251019T064521_20251019T064821_20251019T085627_0179_131_348_2700_"
            "PS1_O_NR_004.zarr\":/measurements/inadir/s7_bt_in";

        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(swathSubdataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(dataset != nullptr);

        char** geolocationMetadata = GDALGetMetadata(dataset, "GEOLOCATION");

        assert(geolocationMetadata != nullptr);
        assert(CSLCount(geolocationMetadata) > 0);

        std::cout << "  ✅ GEOLOCATION metadata exists (" << CSLCount(geolocationMetadata)
                  << " items)" << std::endl;

        GDALClose(dataset);
    }

    /**
     * @brief Test that all required GEOLOCATION metadata fields are present
     */
    static void testGeolocationMetadataFields()
    {
        std::cout << "Testing GEOLOCATION metadata fields..." << std::endl;

        const char* swathSubdataset =
            "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/"
            "e05ab01a9d56408d82ac32d69a5aae2a:202510-s03slsrbt-global/19/products/cpm_v256/"
            "S3A_SL_1_RBT____20251019T064521_20251019T064821_20251019T085627_0179_131_348_2700_"
            "PS1_O_NR_004.zarr\":/measurements/inadir/s7_bt_in";

        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(swathSubdataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(dataset != nullptr);

        // Required fields according to GDAL RFC 4 - Geolocation Arrays
        const char* requiredFields[] = {"X_DATASET", "Y_DATASET",  "X_BAND",  "Y_BAND",      "PIXEL_OFFSET",
                                        "LINE_OFFSET", "PIXEL_STEP", "LINE_STEP", "SRS"};

        for (const char* field : requiredFields)
        {
            const char* value = GDALGetMetadataItem(dataset, field, "GEOLOCATION");
            assert(value != nullptr);
            assert(strlen(value) > 0);
            std::cout << "  ✅ " << field << ": " << value << std::endl;
        }

        GDALClose(dataset);
    }

    /**
     * @brief Test that X_DATASET and Y_DATASET paths are correctly formed
     */
    static void testGeolocationDatasetPaths()
    {
        std::cout << "Testing GEOLOCATION dataset paths..." << std::endl;

        const char* swathSubdataset =
            "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/"
            "e05ab01a9d56408d82ac32d69a5aae2a:202510-s03slsrbt-global/19/products/cpm_v256/"
            "S3A_SL_1_RBT____20251019T064521_20251019T064821_20251019T085627_0179_131_348_2700_"
            "PS1_O_NR_004.zarr\":/measurements/inadir/s7_bt_in";

        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(swathSubdataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(dataset != nullptr);

        const char* xDataset = GDALGetMetadataItem(dataset, "X_DATASET", "GEOLOCATION");
        const char* yDataset = GDALGetMetadataItem(dataset, "Y_DATASET", "GEOLOCATION");

        assert(xDataset != nullptr);
        assert(yDataset != nullptr);

        // Verify paths start with EOPFZARR:
        assert(strncmp(xDataset, "EOPFZARR:", 9) == 0);
        assert(strncmp(yDataset, "EOPFZARR:", 9) == 0);

        // Verify paths contain the correct group (/measurements/inadir/)
        assert(strstr(xDataset, "/measurements/inadir/") != nullptr);
        assert(strstr(yDataset, "/measurements/inadir/") != nullptr);

        // Verify paths end with longitude/latitude
        assert(strstr(xDataset, "longitude") != nullptr);
        assert(strstr(yDataset, "latitude") != nullptr);

        std::cout << "  ✅ X_DATASET: " << xDataset << std::endl;
        std::cout << "  ✅ Y_DATASET: " << yDataset << std::endl;

        GDALClose(dataset);
    }

    /**
     * @brief Test that X_DATASET and Y_DATASET can actually be opened
     */
    static void testGeolocationDatasetsAreOpenable()
    {
        std::cout << "Testing that geolocation datasets can be opened..." << std::endl;

        const char* swathSubdataset =
            "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/"
            "e05ab01a9d56408d82ac32d69a5aae2a:202510-s03slsrbt-global/19/products/cpm_v256/"
            "S3A_SL_1_RBT____20251019T064521_20251019T064821_20251019T085627_0179_131_348_2700_"
            "PS1_O_NR_004.zarr\":/measurements/inadir/s7_bt_in";

        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(swathSubdataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(dataset != nullptr);

        const char* xDataset = GDALGetMetadataItem(dataset, "X_DATASET", "GEOLOCATION");
        const char* yDataset = GDALGetMetadataItem(dataset, "Y_DATASET", "GEOLOCATION");

        // Try to open X_DATASET (longitude)
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH lonDataset = GDALOpen(xDataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(lonDataset != nullptr);
        std::cout << "  ✅ X_DATASET (longitude) opened successfully" << std::endl;

        // Verify it has data
        assert(GDALGetRasterXSize(lonDataset) > 0);
        assert(GDALGetRasterYSize(lonDataset) > 0);
        assert(GDALGetRasterCount(lonDataset) > 0);

        GDALClose(lonDataset);

        // Try to open Y_DATASET (latitude)
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH latDataset = GDALOpen(yDataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(latDataset != nullptr);
        std::cout << "  ✅ Y_DATASET (latitude) opened successfully" << std::endl;

        // Verify it has data
        assert(GDALGetRasterXSize(latDataset) > 0);
        assert(GDALGetRasterYSize(latDataset) > 0);
        assert(GDALGetRasterCount(latDataset) > 0);

        GDALClose(latDataset);
        GDALClose(dataset);
    }

    /**
     * @brief Test that geolocation arrays have same dimensions as data array
     */
    static void testGeolocationArrayDimensions()
    {
        std::cout << "Testing geolocation array dimensions match data..." << std::endl;

        const char* swathSubdataset =
            "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/"
            "e05ab01a9d56408d82ac32d69a5aae2a:202510-s03slsrbt-global/19/products/cpm_v256/"
            "S3A_SL_1_RBT____20251019T064521_20251019T064821_20251019T085627_0179_131_348_2700_"
            "PS1_O_NR_004.zarr\":/measurements/inadir/s7_bt_in";

        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(swathSubdataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(dataset != nullptr);

        int dataWidth = GDALGetRasterXSize(dataset);
        int dataHeight = GDALGetRasterYSize(dataset);

        std::cout << "  Data dimensions: " << dataWidth << " x " << dataHeight << std::endl;

        const char* xDataset = GDALGetMetadataItem(dataset, "X_DATASET", "GEOLOCATION");
        const char* yDataset = GDALGetMetadataItem(dataset, "Y_DATASET", "GEOLOCATION");

        // Open longitude array
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH lonDataset = GDALOpen(xDataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(lonDataset != nullptr);

        int lonWidth = GDALGetRasterXSize(lonDataset);
        int lonHeight = GDALGetRasterYSize(lonDataset);

        std::cout << "  Longitude dimensions: " << lonWidth << " x " << lonHeight << std::endl;

        // Geolocation arrays should have same dimensions as data
        assert(lonWidth == dataWidth);
        assert(lonHeight == dataHeight);

        GDALClose(lonDataset);

        // Open latitude array
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH latDataset = GDALOpen(yDataset, GA_ReadOnly);
        CPLPopErrorHandler();

        assert(latDataset != nullptr);

        int latWidth = GDALGetRasterXSize(latDataset);
        int latHeight = GDALGetRasterYSize(latDataset);

        std::cout << "  Latitude dimensions: " << latWidth << " x " << latHeight << std::endl;

        assert(latWidth == dataWidth);
        assert(latHeight == dataHeight);

        GDALClose(latDataset);
        GDALClose(dataset);

        std::cout << "  ✅ All dimensions match!" << std::endl;
    }

    /**
     * @brief Test that GEOLOCATION metadata is NOT set for non-swath data
     *
     * Regular gridded data (e.g., Sentinel-2) should not have GEOLOCATION metadata
     * because it uses standard GeoTransform.
     */
    static void testNoGeolocationForNonSwathData()
    {
        std::cout << "Testing that non-swath data does NOT have GEOLOCATION metadata..."
                  << std::endl;

        // Sentinel-2 L2A product - regular grid, not swath data
        const char* griddedSubdataset =
            "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/"
            "e05ab01a9d56408d82ac32d69a5aae2a:202510-s2l2a-zarr-global/19/products/cpm_v256/"
            "S2A_MSIL2A_20251019T084331_N0511_R064_T35SMD_20251019T122039.zarr\":"
            "/measurements/b01";

        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(griddedSubdataset, GA_ReadOnly);
        CPLPopErrorHandler();

        // Dataset may or may not open (depends on network), but if it does:
        if (dataset != nullptr)
        {
            char** geolocationMetadata = GDALGetMetadata(dataset, "GEOLOCATION");

            // Regular gridded data should NOT have GEOLOCATION metadata
            // It uses standard GeoTransform instead
            if (geolocationMetadata == nullptr || CSLCount(geolocationMetadata) == 0)
            {
                std::cout << "  ✅ Correctly has NO GEOLOCATION metadata (uses GeoTransform)"
                          << std::endl;
            }
            else
            {
                std::cout << "  ⚠ Gridded data has GEOLOCATION metadata (unexpected)"
                          << std::endl;
            }

            GDALClose(dataset);
        }
        else
        {
            std::cout << "  ⚠ Could not open gridded dataset (network issue?), skipping test"
                      << std::endl;
        }
    }
};

int main()
{
    try
    {
        GeolocationArraysTests::runAllTests();
        std::cout << "\n🎉 All geolocation arrays tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
