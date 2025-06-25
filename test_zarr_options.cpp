#include "gdal_priv.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>  // For SHGetFolderPath
#endif

bool CreateQGISConfigFile() {
    // Find appropriate location for config file
    std::string configFilePath;

#ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        // QGIS config directory
        configFilePath = std::string(appDataPath) + "\\QGIS\\QGIS3\\profiles\\default";
    }
#else
    // On Linux/Mac, use home directory
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        configFilePath = std::string(homeDir) + "/.local/share/QGIS/QGIS3/profiles/default";
    }
#endif

    if (configFilePath.empty()) {
        std::cerr << "Could not determine QGIS config directory" << std::endl;
        return false;
    }

    // Create options file for Zarr
    std::string optionsFile = configFilePath + "/zarr_options.ini";
    std::cout << "Creating QGIS Zarr options file at: " << optionsFile << std::endl;

    std::ofstream iniFile(optionsFile);
    if (!iniFile) {
        std::cerr << "Failed to create " << optionsFile << std::endl;
        return false;
    }

    // Write options configuration
    iniFile << "[GDAL_ZARR]\n";
    iniFile << "EOPF_PROCESS=YES/NO:Enable Earth Observation Processing Framework features\n";
    iniFile.close();

    // Also create a GDAL config file
    std::string gdalConfigFile = configFilePath + "/gdal_EOPF.ini";
    std::cout << "Creating GDAL config file at: " << gdalConfigFile << std::endl;

    std::ofstream gdalIniFile(gdalConfigFile);
    if (!gdalIniFile) {
        std::cerr << "Failed to create " << gdalConfigFile << std::endl;
        return false;
    }

    // Write GDAL configuration
    gdalIniFile << "# GDAL Configuration for EOPF-Zarr\n";
    gdalIniFile << "GDAL_ZARR_HAS_EOPF=YES\n";
    gdalIniFile << "GDAL_EXTENSIONS=.zarr\n";
    gdalIniFile.close();

    return true;
}

int main() {
    // Initialize GDAL
    GDALAllRegister();

    std::cout << "Creating QGIS configuration files for EOPF-Zarr support" << std::endl;

    // Check for Zarr driver and its options
    GDALDriver* poZarrDriver = GetGDALDriverManager()->GetDriverByName("Zarr");
    if (poZarrDriver) {
        std::cout << "Zarr driver found" << std::endl;
        const char* pszOptions = poZarrDriver->GetMetadataItem(GDAL_DMD_OPENOPTIONLIST);
        if (pszOptions && strstr(pszOptions, "EOPF_PROCESS") != nullptr) {
            std::cout << "EOPF_PROCESS option already exists in Zarr driver options!" << std::endl;
        } else {
            std::cout << "EOPF_PROCESS option not found in Zarr driver options" << std::endl;
            std::cout << "Creating configuration files to add the option..." << std::endl;

            if (CreateQGISConfigFile()) {
                std::cout << "Configuration files created successfully" << std::endl;
                std::cout << "Please restart QGIS for changes to take effect" << std::endl;
            } else {
                std::cerr << "Failed to create configuration files" << std::endl;
            }
        }
    } else {
        std::cerr << "Zarr driver not found!" << std::endl;
    }

    return 0;
}