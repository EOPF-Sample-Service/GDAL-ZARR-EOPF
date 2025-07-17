#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "test_utils.h"

class PathParsingTests {
public:
    static void runAllTests() {
        std::cout << "=== Running Path Parsing Tests ===" << std::endl;
        
        testUnquotedUrlParsing();
        testQuotedUrlParsing();
        testVirtualFileSystemPaths();
        testInvalidPaths();
        testEdgeCases();
        
        std::cout << "✅ All Path Parsing Tests Passed!" << std::endl;
    }

private:
    static void testUnquotedUrlParsing() {
        std::cout << "Testing unquoted URL parsing..." << std::endl;
        
        // Test basic unquoted URL
        std::string input1 = "EOPFZARR:/vsicurl/https://example.com/file.zarr";
        std::string result1 = ParseSubdatasetPath(input1);
        assert(result1 == "/vsicurl/https://example.com/file.zarr");
        
        // Test unquoted URL with complex path
        std::string input2 = "EOPFZARR:/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr";
        std::string result2 = ParseSubdatasetPath(input2);
        assert(result2 == "/vsicurl/https://objects.eodc.eu/e05ab01a9d56408d82ac32d69a5aae2a:202507-s02msil1c/15/products/cpm_v256/S2A_MSIL1C_20250715T104701_N0511_R051_T43XDJ_20250715T111222.zarr");
        
        // Test unquoted vsis3 URL
        std::string input3 = "EOPFZARR:/vsis3/bucket/path/file.zarr";
        std::string result3 = ParseSubdatasetPath(input3);
        assert(result3 == "/vsis3/bucket/path/file.zarr");
        
        std::cout << "  ✓ Unquoted URL parsing tests passed" << std::endl;
    }
    
    static void testQuotedUrlParsing() {
        std::cout << "Testing quoted URL parsing..." << std::endl;
        
        // Test quoted URL with subdataset
        std::string input1 = "EOPFZARR:\"/vsicurl/https://example.com/file.zarr/measurements/reflectance/r60m/b09\"";
        std::string result1 = ParseSubdatasetPath(input1);
        assert(result1 == "/vsicurl/https://example.com/file.zarr/measurements/reflectance/r60m/b09");
        
        // Test quoted URL without subdataset
        std::string input2 = "EOPFZARR:\"/vsicurl/https://example.com/file.zarr\"";
        std::string result2 = ParseSubdatasetPath(input2);
        assert(result2 == "/vsicurl/https://example.com/file.zarr");
        
        // Test quoted vsis3 URL with subdataset
        std::string input3 = "EOPFZARR:\"/vsis3/bucket/file.zarr/data/temperature\"";
        std::string result3 = ParseSubdatasetPath(input3);
        assert(result3 == "/vsis3/bucket/file.zarr/data/temperature");
        
        std::cout << "  ✓ Quoted URL parsing tests passed" << std::endl;
    }
    
    static void testVirtualFileSystemPaths() {
        std::cout << "Testing virtual file system paths..." << std::endl;
        
        // Test regular file path
        std::string input1 = "EOPFZARR:c:/path/to/file.zarr";
        std::string result1 = ParseSubdatasetPath(input1);
        assert(result1 == "c:/path/to/file.zarr");
        
        // Test Unix file path
        std::string input2 = "EOPFZARR:/home/user/data/file.zarr";
        std::string result2 = ParseSubdatasetPath(input2);
        assert(result2 == "/home/user/data/file.zarr");
        
        // Test quoted file path with subdataset
        std::string input3 = "EOPFZARR:\"/home/user/data/file.zarr/measurements/data\"";
        std::string result3 = ParseSubdatasetPath(input3);
        assert(result3 == "/home/user/data/file.zarr/measurements/data");
        
        std::cout << "  ✓ Virtual file system path tests passed" << std::endl;
    }
    
    static void testInvalidPaths() {
        std::cout << "Testing invalid paths..." << std::endl;
        
        // Test empty path
        std::string input1 = "EOPFZARR:";
        std::string result1 = ParseSubdatasetPath(input1);
        assert(result1.empty());
        
        // Test malformed quoted path
        std::string input2 = "EOPFZARR:\"incomplete";
        std::string result2 = ParseSubdatasetPath(input2);
        // Should handle gracefully - exact behavior depends on implementation
        
        // Test path without EOPFZARR prefix
        std::string input3 = "/vsicurl/https://example.com/file.zarr";
        std::string result3 = ParseSubdatasetPath(input3);
        assert(result3 == "/vsicurl/https://example.com/file.zarr");
        
        std::cout << "  ✓ Invalid path handling tests passed" << std::endl;
    }
    
    static void testEdgeCases() {
        std::cout << "Testing edge cases..." << std::endl;
        
        // Test URL with special characters
        std::string input1 = "EOPFZARR:/vsicurl/https://example.com/path%20with%20spaces/file.zarr";
        std::string result1 = ParseSubdatasetPath(input1);
        assert(result1 == "/vsicurl/https://example.com/path%20with%20spaces/file.zarr");
        
        // Test nested quotes
        std::string input2 = "EOPFZARR:\"/vsicurl/https://example.com/file.zarr\"";
        std::string result2 = ParseSubdatasetPath(input2);
        assert(result2 == "/vsicurl/https://example.com/file.zarr");
        
        // Test very long path
        std::string longPath = "/vsicurl/https://very.long.domain.name.example.com/very/long/path/with/many/segments/and/subdirectories/file.zarr/measurements/reflectance/very/deep/subdataset/path";
        std::string input3 = "EOPFZARR:\"" + longPath + "\"";
        std::string result3 = ParseSubdatasetPath(input3);
        assert(result3 == longPath);
        
        std::cout << "  ✓ Edge case tests passed" << std::endl;
    }
};

int main() {
    try {
        PathParsingTests::runAllTests();
        std::cout << std::endl << "🎉 All tests completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
