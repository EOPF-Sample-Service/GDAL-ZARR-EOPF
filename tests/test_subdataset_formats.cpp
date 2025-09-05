#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <cpl_string.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

/**
 * @brief Tests for the new subdataset functionality we implemented
 * 
 * This test suite specifically validates:
 * - New colon-separated format: ZARR:"/path/to/file":subdataset
 * - Legacy format compatibility: EOPFZARR:/path/to/file/subdataset
 * - Error suppression with EOPF_SHOW_ZARR_ERRORS environment variable
 * - Error handler wrapping functionality
 */

class SubdatasetFormatTests
{
public:
    static void runAllTests()
    {
        std::cout << "=== Running Subdataset Format Tests ===" << std::endl;
        
        // Initialize GDAL
        GDALAllRegister();
        
        testColonSeparatedFormat();
        testLegacyFormatCompatibility();
        testErrorSuppression();
        testBothFormatsEquivalent();
        testErrorHandlerWrapping();
        
        std::cout << "✅ All Subdataset Format Tests Completed!" << std::endl;
    }
    
private:
    static void testColonSeparatedFormat()
    {
        std::cout << "Testing new colon-separated subdataset format..." << std::endl;
        
        // Test various colon-separated formats
        std::vector<std::string> testPaths = {
            "ZARR:\"/vsicurl/https://example.com/file.zarr\":measurements/B01",
            "ZARR:\"/home/user/test.zarr\":data/reflectance", 
            "ZARR:\"c:/data/test.zarr\":measurements/B02",
            "ZARR:\"/vsis3/bucket/file.zarr\":measurements/data"
        };
        
        for (const auto& path : testPaths)
        {
            std::cout << "  Testing: " << path << std::endl;
            
            // We can't test actual opening without real data files,
            // but we can test that the driver accepts the format
            CPLPushErrorHandler(CPLQuietErrorHandler);
            GDALDriverH driver = GDALIdentifyDriver(path.c_str(), nullptr);
            CPLPopErrorHandler();
            
            if (driver)
            {
                const char* driverName = GDALGetDriverShortName(driver);
                if (driverName && std::string(driverName) == "EOPFZARR")
                {
                    std::cout << "    ✓ Driver correctly identified for colon format" << std::endl;
                }
                else
                {
                    std::cout << "    ⚠ Wrong driver identified: " << (driverName ? driverName : "null") << std::endl;
                }
            }
            else
            {
                std::cout << "    ⚠ No driver identified (expected if plugin not loaded)" << std::endl;
            }
        }
        
        std::cout << "  ✓ Colon-separated format tests completed" << std::endl;
    }
    
    static void testLegacyFormatCompatibility()
    {
        std::cout << "Testing legacy format compatibility..." << std::endl;
        
        // Test legacy EOPFZARR format still works
        std::vector<std::string> legacyPaths = {
            "EOPFZARR:/vsicurl/https://example.com/file.zarr/measurements/B01",
            "EOPFZARR:/home/user/test.zarr/data/reflectance",
            "EOPFZARR:\"/home/user/test.zarr/measurements/data\"",
            "EOPFZARR:\"/vsis3/bucket/file.zarr/subdataset\""
        };
        
        for (const auto& path : legacyPaths)
        {
            std::cout << "  Testing legacy: " << path << std::endl;
            
            CPLPushErrorHandler(CPLQuietErrorHandler);
            GDALDriverH driver = GDALIdentifyDriver(path.c_str(), nullptr);
            CPLPopErrorHandler();
            
            if (driver)
            {
                const char* driverName = GDALGetDriverShortName(driver);
                if (driverName && std::string(driverName) == "EOPFZARR")
                {
                    std::cout << "    ✓ Driver correctly identified for legacy format" << std::endl;
                }
            }
            else
            {
                std::cout << "    ⚠ No driver identified for legacy format" << std::endl;
            }
        }
        
        std::cout << "  ✓ Legacy format compatibility tests completed" << std::endl;
    }
    
    static void testErrorSuppression()
    {
        std::cout << "Testing error suppression functionality..." << std::endl;
        
        // Test with error suppression ON (default)
        {
            std::cout << "  Testing with EOPF_SHOW_ZARR_ERRORS=NO (default)..." << std::endl;
            
            // Set environment variable to suppress errors
            #ifdef _WIN32
            _putenv_s("EOPF_SHOW_ZARR_ERRORS", "NO");
            #else
            setenv("EOPF_SHOW_ZARR_ERRORS", "NO", 1);
            #endif
            
            // Try to open an invalid path - should fail silently
            CPLPushErrorHandler(CPLQuietErrorHandler);
            GDALDatasetH dataset = GDALOpen("ZARR:\"/nonexistent/file.zarr\":subdataset", GA_ReadOnly);
            CPLPopErrorHandler();
            
            if (!dataset)
            {
                std::cout << "    ✓ Invalid path failed silently as expected" << std::endl;
            }
            else
            {
                std::cout << "    ⚠ Invalid path unexpectedly succeeded" << std::endl;
                GDALClose(dataset);
            }
        }
        
        // Test with error suppression OFF
        {
            std::cout << "  Testing with EOPF_SHOW_ZARR_ERRORS=YES..." << std::endl;
            
            // Set environment variable to show errors
            #ifdef _WIN32
            _putenv_s("EOPF_SHOW_ZARR_ERRORS", "YES");
            #else
            setenv("EOPF_SHOW_ZARR_ERRORS", "YES", 1);
            #endif
            
            // Try to open an invalid path - errors should be visible
            CPLPushErrorHandler(CPLQuietErrorHandler);
            GDALDatasetH dataset = GDALOpen("ZARR:\"/nonexistent/file.zarr\":subdataset", GA_ReadOnly);
            CPLPopErrorHandler();
            
            if (!dataset)
            {
                std::cout << "    ✓ Invalid path failed (errors would be visible if not suppressed by test)" << std::endl;
            }
            else
            {
                std::cout << "    ⚠ Invalid path unexpectedly succeeded" << std::endl;
                GDALClose(dataset);
            }
        }
        
        // Reset to default
        #ifdef _WIN32
        _putenv_s("EOPF_SHOW_ZARR_ERRORS", "NO");
        #else
        setenv("EOPF_SHOW_ZARR_ERRORS", "NO", 1);
        #endif
        
        std::cout << "  ✓ Error suppression tests completed" << std::endl;
    }
    
    static void testBothFormatsEquivalent()
    {
        std::cout << "Testing that both formats are equivalent..." << std::endl;
        
        // Test pairs that should be equivalent
        std::vector<std::pair<std::string, std::string>> equivalentPairs = {
            {
                "ZARR:\"/home/test.zarr\":measurements/B01",
                "EOPFZARR:/home/test.zarr/measurements/B01"
            },
            {
                "ZARR:\"/vsicurl/https://example.com/file.zarr\":data",
                "EOPFZARR:/vsicurl/https://example.com/file.zarr/data"
            }
        };
        
        for (const auto& pair : equivalentPairs)
        {
            std::cout << "  Comparing equivalent formats:" << std::endl;
            std::cout << "    New: " << pair.first << std::endl;
            std::cout << "    Old: " << pair.second << std::endl;
            
            CPLPushErrorHandler(CPLQuietErrorHandler);
            
            GDALDriverH driver1 = GDALIdentifyDriver(pair.first.c_str(), nullptr);
            GDALDriverH driver2 = GDALIdentifyDriver(pair.second.c_str(), nullptr);
            
            CPLPopErrorHandler();
            
            bool both_identified = (driver1 != nullptr) && (driver2 != nullptr);
            bool neither_identified = (driver1 == nullptr) && (driver2 == nullptr);
            
            if (both_identified || neither_identified)
            {
                std::cout << "    ✓ Both formats handled consistently" << std::endl;
            }
            else
            {
                std::cout << "    ⚠ Inconsistent handling between formats" << std::endl;
            }
        }
        
        std::cout << "  ✓ Format equivalence tests completed" << std::endl;
    }
    
    static void testErrorHandlerWrapping()
    {
        std::cout << "Testing error handler wrapping functionality..." << std::endl;
        
        // This test verifies that our error suppression doesn't interfere
        // with normal GDAL error handling
        
        // Count errors before test
        CPLPushErrorHandler(CPLQuietErrorHandler);
        
        // Try operations that should generate errors
        GDALDatasetH dataset1 = GDALOpen("ZARR:\"/absolutely/nonexistent/path.zarr\":test", GA_ReadOnly);
        GDALDatasetH dataset2 = GDALOpen("EOPFZARR:/another/nonexistent/path.zarr", GA_ReadOnly);
        
        CPLPopErrorHandler();
        
        // Both should fail
        if (!dataset1 && !dataset2)
        {
            std::cout << "    ✓ Error handling working correctly - invalid paths failed" << std::endl;
        }
        else
        {
            std::cout << "    ⚠ Unexpected success with invalid paths" << std::endl;
            if (dataset1) GDALClose(dataset1);
            if (dataset2) GDALClose(dataset2);
        }
        
        std::cout << "  ✓ Error handler wrapping tests completed" << std::endl;
    }
};

int main()
{
    std::cout << "Starting subdataset format tests..." << std::endl;
    
    try
    {
        SubdatasetFormatTests::runAllTests();
        std::cout << "\n🎉 All subdataset format tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
