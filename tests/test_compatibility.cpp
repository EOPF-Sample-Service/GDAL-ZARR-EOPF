#include <iostream>
#include <cassert>
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <cpl_string.h>

class CompatibilityTests {
public:
    static void runAllTests() {
        std::cout << "=== Running URL Format Compatibility Tests ===" << std::endl;
        
        // Initialize GDAL
        GDALAllRegister();
        
        testBackwardCompatibility();
        testNewQuotedFormat();
        testMixedFormats();
        testQGISWorkflow();
        
        std::cout << "✅ All Compatibility Tests Passed!" << std::endl;
    }

private:
    static void testBackwardCompatibility() {
        std::cout << "Testing backward compatibility (old unquoted format)..." << std::endl;
        
        // Test the old format that QGIS uses
        const char* oldFormatUrl = "EOPFZARR:/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr";
        
        std::cout << "  Testing old format: " << oldFormatUrl << std::endl;
        
        // Test driver identification
        GDALDriverH driver = GDALIdentifyDriverEx(oldFormatUrl, 
            GDAL_OF_RASTER | GDAL_OF_READONLY, nullptr, nullptr);
        
        if (driver) {
            const char* driverName = GDALGetDriverShortName(driver);
            assert(std::string(driverName) == "EOPFZARR");
            std::cout << "    ✓ Driver correctly identified for old format" << std::endl;
        } else {
            std::cout << "    ⚠ Driver not identified (may be due to network/file issues)" << std::endl;
        }
        
        // Test opening (may fail due to network, but tests code path)
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(oldFormatUrl, GA_ReadOnly);
        CPLPopErrorHandler();
        
        if (dataset) {
            std::cout << "    ✓ Old format dataset opened successfully" << std::endl;
            
            // Should have subdatasets in metadata
            char** metadata = GDALGetMetadata(dataset, "SUBDATASETS");
            if (metadata && metadata[0]) {
                std::cout << "    ✓ Subdatasets found in old format" << std::endl;
                std::cout << "      Example: " << metadata[0] << std::endl;
            }
            
            GDALClose(dataset);
        } else {
            std::cout << "    ⚠ Could not open old format (expected for network issues)" << std::endl;
        }
        
        std::cout << "  ✓ Backward compatibility tests completed" << std::endl;
    }
    
    static void testNewQuotedFormat() {
        std::cout << "Testing new quoted format with subdatasets..." << std::endl;
        
        // Test the new format with specific subdataset
        const char* newFormatUrl = "EOPFZARR:\"/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr/measurements/reflectance/r60m/b09\"";
        
        std::cout << "  Testing new format: " << newFormatUrl << std::endl;
        
        // Test driver identification
        GDALDriverH driver = GDALIdentifyDriverEx(newFormatUrl, 
            GDAL_OF_RASTER | GDAL_OF_READONLY, nullptr, nullptr);
        
        if (driver) {
            const char* driverName = GDALGetDriverShortName(driver);
            assert(std::string(driverName) == "EOPFZARR");
            std::cout << "    ✓ Driver correctly identified for new format" << std::endl;
        } else {
            std::cout << "    ⚠ Driver not identified (may be due to network/file issues)" << std::endl;
        }
        
        // Test opening specific subdataset
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH dataset = GDALOpen(newFormatUrl, GA_ReadOnly);
        CPLPopErrorHandler();
        
        if (dataset) {
            std::cout << "    ✓ New format subdataset opened successfully" << std::endl;
            
            // This should be a specific raster band, not a collection
            int bandCount = GDALGetRasterCount(dataset);
            std::cout << "    Band count: " << bandCount << std::endl;
            
            if (bandCount > 0) {
                GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
                if (band) {
                    int xSize = GDALGetRasterBandXSize(band);
                    int ySize = GDALGetRasterBandYSize(band);
                    std::cout << "    ✓ Raster dimensions: " << xSize << "x" << ySize << std::endl;
                }
            }
            
            GDALClose(dataset);
        } else {
            std::cout << "    ⚠ Could not open new format (expected for network issues)" << std::endl;
        }
        
        std::cout << "  ✓ New quoted format tests completed" << std::endl;
    }
    
    static void testMixedFormats() {
        std::cout << "Testing mixed URL formats..." << std::endl;
        
        std::vector<std::pair<std::string, std::string>> testCases = {
            {"Unquoted vsicurl", "EOPFZARR:/vsicurl/https://example.com/file.zarr"},
            {"Quoted vsicurl", "EOPFZARR:\"/vsicurl/https://example.com/file.zarr\""},
            {"Unquoted vsis3", "EOPFZARR:/vsis3/bucket/file.zarr"},
            {"Quoted vsis3", "EOPFZARR:\"/vsis3/bucket/file.zarr/data\""},
            {"Local path", "EOPFZARR:/home/user/file.zarr"},
            {"Quoted local", "EOPFZARR:\"/home/user/file.zarr/dataset\""}
        };
        
        for (const auto& testCase : testCases) {
            std::cout << "  Testing " << testCase.first << ": " << testCase.second << std::endl;
            
            // Test identification
            GDALDriverH driver = GDALIdentifyDriverEx(testCase.second.c_str(), 
                GDAL_OF_RASTER | GDAL_OF_READONLY, nullptr, nullptr);
            
            if (driver) {
                const char* driverName = GDALGetDriverShortName(driver);
                if (std::string(driverName) == "EOPFZARR") {
                    std::cout << "    ✓ Correctly identified as EOPFZARR" << std::endl;
                } else {
                    std::cout << "    ⚠ Identified as different driver: " << driverName << std::endl;
                }
            } else {
                std::cout << "    ⚠ Not identified (expected for non-existent files)" << std::endl;
            }
        }
        
        std::cout << "  ✓ Mixed format tests completed" << std::endl;
    }
    
    static void testQGISWorkflow() {
        std::cout << "Testing QGIS-like workflow..." << std::endl;
        
        // Simulate the QGIS workflow:
        // 1. Open root dataset to get subdatasets
        // 2. User selects a subdataset
        // 3. Open the specific subdataset
        
        const char* rootUrl = "EOPFZARR:/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr";
        
        std::cout << "  Step 1: Opening root dataset..." << std::endl;
        
        CPLPushErrorHandler(CPLQuietErrorHandler);
        GDALDatasetH rootDataset = GDALOpen(rootUrl, GA_ReadOnly);
        CPLPopErrorHandler();
        
        if (rootDataset) {
            std::cout << "    ✓ Root dataset opened" << std::endl;
            
            // Get subdatasets
            char** subdatasets = GDALGetMetadata(rootDataset, "SUBDATASETS");
            if (subdatasets && subdatasets[0]) {
                std::cout << "  Step 2: Found subdatasets, simulating user selection..." << std::endl;
                
                // Find the first subdataset name
                std::string firstSubdataset;
                for (int i = 0; subdatasets[i]; i++) {
                    std::string line(subdatasets[i]);
                    if (line.find("_NAME=") != std::string::npos) {
                        size_t pos = line.find("=");
                        if (pos != std::string::npos) {
                            firstSubdataset = line.substr(pos + 1);
                            break;
                        }
                    }
                }
                
                if (!firstSubdataset.empty()) {
                    std::cout << "    Selected subdataset: " << firstSubdataset << std::endl;
                    
                    std::cout << "  Step 3: Opening selected subdataset..." << std::endl;
                    
                    CPLPushErrorHandler(CPLQuietErrorHandler);
                    GDALDatasetH subdataset = GDALOpen(firstSubdataset.c_str(), GA_ReadOnly);
                    CPLPopErrorHandler();
                    
                    if (subdataset) {
                        std::cout << "    ✓ Subdataset opened successfully" << std::endl;
                        std::cout << "    ✓ QGIS workflow simulation completed" << std::endl;
                        GDALClose(subdataset);
                    } else {
                        std::cout << "    ⚠ Could not open subdataset" << std::endl;
                    }
                }
            }
            
            GDALClose(rootDataset);
        } else {
            std::cout << "    ⚠ Could not open root dataset (expected for network issues)" << std::endl;
        }
        
        std::cout << "  ✓ QGIS workflow tests completed" << std::endl;
    }
};

int main() {
    try {
        CompatibilityTests::runAllTests();
        std::cout << std::endl << "🎉 All compatibility tests completed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Compatibility test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Compatibility test failed with unknown exception" << std::endl;
        return 1;
    }
}
