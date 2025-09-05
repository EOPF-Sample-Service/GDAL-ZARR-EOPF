#include <iostream>
#include <gdal.h>

int main()
{
    std::cout << "Testing basic GDAL functionality..." << std::endl;
    
    try 
    {
        std::cout << "Calling GDALVersionInfo..." << std::endl;
        const char* version = GDALVersionInfo("VERSION_NUM");
        std::cout << "GDAL Version: " << (version ? version : "null") << std::endl;
        
        std::cout << "Calling GDALAllRegister..." << std::endl;
        GDALAllRegister();
        std::cout << "GDALAllRegister completed successfully" << std::endl;
        
        std::cout << "Getting driver count..." << std::endl;
        int driverCount = GDALGetDriverCount();
        std::cout << "Found " << driverCount << " drivers" << std::endl;
        
        std::cout << "✅ Basic GDAL test passed!" << std::endl;
        return 0;
    }
    catch (...)
    {
        std::cout << "❌ Exception caught during GDAL operations" << std::endl;
        return 1;
    }
}
