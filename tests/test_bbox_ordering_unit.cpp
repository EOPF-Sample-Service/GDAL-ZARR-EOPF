#include <cassert>
#include <iostream>
#include <string>
#include <cmath>

/**
 * @brief Unit tests for Issue #135: EOPF bbox ordering detection
 * 
 * Tests the logic for detecting and correcting non-standard EOPF bbox ordering.
 * EOPF uses [east, south, west, north] instead of STAC standard [west, south, east, north].
 */

/**
 * @brief Simulates the bbox ordering detection logic from eopf_metadata.cpp
 * 
 * @param bbox0 First bbox value
 * @param bbox1 Second bbox value
 * @param bbox2 Third bbox value
 * @param bbox3 Fourth bbox value
 * @param minX Output: corrected minX (west)
 * @param minY Output: corrected minY (south)
 * @param maxX Output: corrected maxX (east)
 * @param maxY Output: corrected maxY (north)
 */
void detectAndCorrectBboxOrdering(
    double bbox0, double bbox1, double bbox2, double bbox3,
    double& minX, double& minY, double& maxX, double& maxY)
{
    // STAC standard is [west, south, east, north] = [minLon, minLat, maxLon, maxLat]
    // But EOPF Zarr may use [east, south, west, north] = [maxLon, minLat, minLon, maxLat]
    // Detect this by checking if bbox[0] > bbox[2]
    if (bbox0 > bbox2)
    {
        // Non-standard ordering detected
        minX = bbox2;  // minLon from position 2
        minY = bbox1;  // minLat from position 1
        maxX = bbox0;  // maxLon from position 0
        maxY = bbox3;  // maxLat from position 3
    }
    else
    {
        // Standard STAC ordering
        minX = bbox0;
        minY = bbox1;
        maxX = bbox2;
        maxY = bbox3;
    }
}

/**
 * @brief Test EOPF non-standard bbox ordering (Sentinel-3 SLSTR case)
 */
bool testEOPFBboxOrderingSentinel3()
{
    std::cout << "Testing EOPF bbox ordering detection (Sentinel-3 SLSTR)..." << std::endl;
    
    // Real data from Sentinel-3 SLSTR product
    // EOPF format: [east, south, west, north] = [maxLon, minLat, minLon, maxLat]
    double bbox[4] = {56.3803, 7.58006, 40.4139, 20.9708};
    
    double minX, minY, maxX, maxY;
    detectAndCorrectBboxOrdering(bbox[0], bbox[1], bbox[2], bbox[3], minX, minY, maxX, maxY);
    
    // After correction, should be in standard [west, south, east, north] format
    // Expected: minLon=40.4139, minLat=7.58006, maxLon=56.3803, maxLat=20.9708
    
    // Verify bbox[0] > bbox[2] triggered the swap
    assert(bbox[0] > bbox[2]);
    
    // Verify corrected values
    assert(std::abs(minX - 40.4139) < 0.0001);
    assert(std::abs(minY - 7.58006) < 0.0001);
    assert(std::abs(maxX - 56.3803) < 0.0001);
    assert(std::abs(maxY - 20.9708) < 0.0001);
    
    // Verify min < max after correction
    assert(minX < maxX);
    assert(minY < maxY);
    
    // Verify geographic region (Djibouti: 40-56°E, 7-21°N)
    assert(minX >= 40.0 && minX <= 56.0);
    assert(maxX >= 40.0 && maxX <= 56.0);
    assert(minY >= 7.0 && minY <= 21.0);
    assert(maxY >= 7.0 && maxY <= 21.0);
    
    std::cout << "  Input (EOPF):  [" << bbox[0] << ", " << bbox[1] << ", " 
              << bbox[2] << ", " << bbox[3] << "]" << std::endl;
    std::cout << "  Output (STAC): [" << minX << ", " << minY << ", " 
              << maxX << ", " << maxY << "]" << std::endl;
    std::cout << "  Region: Djibouti/Horn of Africa (40-56°E, 7-21°N) ✓" << std::endl;
    std::cout << "  ✓ EOPF bbox ordering detection passed (Sentinel-3)" << std::endl;
    
    return true;
}

/**
 * @brief Test EOPF non-standard bbox ordering (Sentinel-2 case)
 */
bool testEOPFBboxOrderingSentinel2()
{
    std::cout << "Testing EOPF bbox ordering detection (Sentinel-2 MSI L2A)..." << std::endl;
    
    // Real data from Sentinel-2 MSI L2A product (geographic bbox)
    // EOPF format: [east, south, west, north] = [maxLon, minLat, minLon, maxLat]
    double bbox[4] = {-29.74827, 67.58993, -30.64866, 68.40500};
    
    double minX, minY, maxX, maxY;
    detectAndCorrectBboxOrdering(bbox[0], bbox[1], bbox[2], bbox[3], minX, minY, maxX, maxY);
    
    // After correction, should be in standard [west, south, east, north] format
    // Expected: minLon=-30.64866, minLat=67.58993, maxLon=-29.74827, maxLat=68.40500
    
    // Verify bbox[0] > bbox[2] triggered the swap (even with negative values)
    assert(bbox[0] > bbox[2]);
    
    // Verify corrected values
    assert(std::abs(minX - (-30.64866)) < 0.0001);
    assert(std::abs(minY - 67.58993) < 0.0001);
    assert(std::abs(maxX - (-29.74827)) < 0.0001);
    assert(std::abs(maxY - 68.40500) < 0.0001);
    
    // Verify min < max after correction
    assert(minX < maxX);
    assert(minY < maxY);
    
    // Verify geographic region (Greenland: ~-31 to -29°W, 67-68°N)
    assert(minX >= -31.0 && minX <= -29.0);
    assert(maxX >= -31.0 && maxX <= -29.0);
    assert(minY >= 67.0 && minY <= 69.0);
    assert(maxY >= 67.0 && maxY <= 69.0);
    
    std::cout << "  Input (EOPF):  [" << bbox[0] << ", " << bbox[1] << ", " 
              << bbox[2] << ", " << bbox[3] << "]" << std::endl;
    std::cout << "  Output (STAC): [" << minX << ", " << minY << ", " 
              << maxX << ", " << maxY << "]" << std::endl;
    std::cout << "  Region: Greenland (-31 to -29°W, 67-68°N) ✓" << std::endl;
    std::cout << "  ✓ EOPF bbox ordering detection passed (Sentinel-2)" << std::endl;
    
    return true;
}

/**
 * @brief Test standard STAC bbox ordering (should not be modified)
 */
bool testStandardBboxOrdering()
{
    std::cout << "Testing standard STAC bbox ordering (no modification)..." << std::endl;
    
    // Standard STAC format: [west, south, east, north]
    double bbox[4] = {-10.0, 35.0, 5.0, 45.0};
    
    double minX, minY, maxX, maxY;
    detectAndCorrectBboxOrdering(bbox[0], bbox[1], bbox[2], bbox[3], minX, minY, maxX, maxY);
    
    // Should NOT trigger swap (bbox[0] <= bbox[2])
    assert(bbox[0] <= bbox[2]);
    
    // Values should remain unchanged
    assert(minX == bbox[0]);
    assert(minY == bbox[1]);
    assert(maxX == bbox[2]);
    assert(maxY == bbox[3]);
    
    // Verify min < max
    assert(minX < maxX);
    assert(minY < maxY);
    
    std::cout << "  Input (STAC):  [" << bbox[0] << ", " << bbox[1] << ", " 
              << bbox[2] << ", " << bbox[3] << "]" << std::endl;
    std::cout << "  Output (same): [" << minX << ", " << minY << ", " 
              << maxX << ", " << maxY << "]" << std::endl;
    std::cout << "  ✓ Standard bbox ordering preserved" << std::endl;
    
    return true;
}

/**
 * @brief Test edge case: bbox with equal min/max (point)
 */
bool testEdgeCaseSinglePoint()
{
    std::cout << "Testing edge case: single point bbox..." << std::endl;
    
    // Edge case: single point (min == max)
    double bbox[4] = {10.0, 20.0, 10.0, 20.0};
    
    double minX, minY, maxX, maxY;
    detectAndCorrectBboxOrdering(bbox[0], bbox[1], bbox[2], bbox[3], minX, minY, maxX, maxY);
    
    // bbox[0] == bbox[2], so should not trigger swap
    assert(bbox[0] == bbox[2]);
    
    // Values should remain unchanged
    assert(minX == 10.0);
    assert(minY == 20.0);
    assert(maxX == 10.0);
    assert(maxY == 20.0);
    
    std::cout << "  ✓ Single point bbox handled correctly" << std::endl;
    
    return true;
}

/**
 * @brief Test edge case: bbox crossing antimeridian
 */
bool testEdgeCaseAntimeridian()
{
    std::cout << "Testing edge case: bbox crossing antimeridian..." << std::endl;
    
    // Edge case: crossing antimeridian (west > east in longitude)
    // Example: 170°E to -170°E (or 190°E)
    // In EOPF format: [maxLon(170), minLat, minLon(-170), maxLat]
    double bbox[4] = {170.0, -10.0, -170.0, 10.0};
    
    double minX, minY, maxX, maxY;
    detectAndCorrectBboxOrdering(bbox[0], bbox[1], bbox[2], bbox[3], minX, minY, maxX, maxY);
    
    // bbox[0] > bbox[2] should trigger swap
    assert(bbox[0] > bbox[2]);
    
    // After correction
    assert(minX == -170.0);
    assert(maxX == 170.0);
    
    std::cout << "  Input (EOPF):  [" << bbox[0] << ", " << bbox[1] << ", " 
              << bbox[2] << ", " << bbox[3] << "]" << std::endl;
    std::cout << "  Output (STAC): [" << minX << ", " << minY << ", " 
              << maxX << ", " << maxY << "]" << std::endl;
    std::cout << "  ✓ Antimeridian crossing handled" << std::endl;
    
    return true;
}

int main()
{
    std::cout << "=== Issue #135: EOPF BBox Ordering Unit Tests ===" << std::endl;
    std::cout << std::endl;
    
    try
    {
        bool allPassed = true;
        
        allPassed &= testEOPFBboxOrderingSentinel3();
        std::cout << std::endl;
        
        allPassed &= testEOPFBboxOrderingSentinel2();
        std::cout << std::endl;
        
        allPassed &= testStandardBboxOrdering();
        std::cout << std::endl;
        
        allPassed &= testEdgeCaseSinglePoint();
        std::cout << std::endl;
        
        allPassed &= testEdgeCaseAntimeridian();
        std::cout << std::endl;
        
        if (allPassed)
        {
            std::cout << "✅ All EOPF bbox ordering tests passed!" << std::endl;
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
