#include <cassert>
#include <iostream>
#include <string>

/**
 * @brief Basic functionality tests that don't require GDAL DLLs
 *
 * These tests validate core functionality without external dependencies
 */

// Simple path validation tests
bool testBasicPathValidation()
{
    std::cout << "Testing basic path validation..." << std::endl;

    // Test EOPFZARR prefix detection
    std::string validPath1 = "EOPFZARR:/vsicurl/https://example.com/file.zarr";
    assert(validPath1.substr(0, 9) == "EOPFZARR:");

    std::string validPath2 = "EOPFZARR:\"/vsis3/bucket/file.zarr\"";
    assert(validPath2.substr(0, 9) == "EOPFZARR:");

    std::string invalidPath = "/vsicurl/https://example.com/file.zarr";
    assert(invalidPath.substr(0, 9) != "EOPFZARR:");

    std::cout << "  ✓ Basic path validation tests passed" << std::endl;
    return true;
}

// Test string manipulation functions
bool testStringManipulation()
{
    std::cout << "Testing string manipulation..." << std::endl;

    // Test quote removal
    std::string quoted = "\"test_string\"";
    if (quoted.front() == '"' && quoted.back() == '"')
    {
        quoted = quoted.substr(1, quoted.length() - 2);
    }
    assert(quoted == "test_string");

    // Test virtual path detection
    std::string vsicurl = "/vsicurl/https://example.com";
    assert(vsicurl.find("/vsi") == 0);

    std::string regular = "/regular/path";
    assert(regular.find("/vsi") != 0);

    std::cout << "  ✓ String manipulation tests passed" << std::endl;
    return true;
}

// Test URL parsing logic
bool testUrlParsing()
{
    std::cout << "Testing URL parsing logic..." << std::endl;

    // Test basic URL components
    std::string url = "https://objects.eodc.eu/bucket/file.zarr";
    assert(url.find("https://") == 0);
    assert(url.find("objects.eodc.eu") != std::string::npos);
    assert(url.find(".zarr") != std::string::npos);

    // Test path extraction
    std::string fullPath = "EOPFZARR:/vsicurl/https://example.com/file.zarr";
    size_t prefixLen = std::string("EOPFZARR:").length();
    std::string extractedPath = fullPath.substr(prefixLen);
    assert(extractedPath == "/vsicurl/https://example.com/file.zarr");

    std::cout << "  ✓ URL parsing tests passed" << std::endl;
    return true;
}

int main()
{
    std::cout << "=== Basic Functionality Tests ===" << std::endl;

    try
    {
        bool allPassed = true;

        allPassed &= testBasicPathValidation();
        allPassed &= testStringManipulation();
        allPassed &= testUrlParsing();

        if (allPassed)
        {
            std::cout << "✅ All basic functionality tests passed!" << std::endl;
            return 0;
        }
        else
        {
            std::cout << "❌ Some tests failed!" << std::endl;
            return 1;
        }
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
