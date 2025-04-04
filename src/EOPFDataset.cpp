#include "EOPFDataset.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_vsi.h"
#include "cpl_json.h"
#include "EOPFRasterBand.h" 

#include <algorithm>

// Constructor
EOPFDataset::EOPFDataset() :
    m_eMode(EOPFMode::CONVENIENCE),
    m_bIsZarrV3(false)
{
}

// Static method to identify EOPF datasets
int EOPFDataset::Identify(GDALOpenInfo* poOpenInfo)
{
    // Skip if filename is empty
    if (poOpenInfo->pszFilename == nullptr)
        return FALSE;

    // Get the filename
    const char* pszFilename = poOpenInfo->pszFilename;

    // Check if the file has a .zarr extension
    if (EQUAL(CPLGetExtension(pszFilename), "zarr"))
        return TRUE;

    // Check for EOPF: prefix
    if (STARTS_WITH_CI(pszFilename, "EOPF:"))
        return TRUE;

    // Look for zarr.json (Zarr V3) or .zarray (Zarr V2) files
    VSIStatBufL sStat;
    CPLString osZarrJsonPath = CPLFormFilename(pszFilename, "zarr.json", nullptr);
    CPLString osZarrayPath = CPLFormFilename(pszFilename, ".zarray", nullptr);

    if (VSIStatL(osZarrJsonPath, &sStat) == 0 || VSIStatL(osZarrayPath, &sStat) == 0)
        return TRUE;

    return FALSE;
}

// Static method to open EOPF datasets
GDALDataset* EOPFDataset::Open(GDALOpenInfo* poOpenInfo)
{
    // Check if the driver can open this file
    if (!Identify(poOpenInfo))
        return nullptr;

	if (poOpenInfo->eAccess == GA_Update) {
		CPLError(CE_Failure, CPLE_NotSupported, "EOPF Driver is read-only.");
		return nullptr;
	}
    // Parse for driver mode option
    const char* pszMode = CSLFetchNameValue(poOpenInfo->papszOpenOptions, "MODE");
    EOPFMode eMode = EOPFMode::CONVENIENCE;  // Default to convenience mode

    // Create a new dataset
    EOPFDataset* poDS = new EOPFDataset();

    if (pszMode != nullptr) {
        if (EQUAL(pszMode, "SENSOR"))
            poDS->m_eMode = EOPFMode::SENSOR;
		else if (EQUAL(pszMode, "CONVENIENCE"))
            poDS->m_eMode = EOPFMode::CONVENIENCE;
        else {
            CPLError(CE_Warning, CPLE_AppDefined,
                "Unknown mode '%s', defaulting to CONVENIENCE mode", pszMode);
        }
    }

    poDS->m_osPath = poOpenInfo->pszFilename;
    if (STARTS_WITH_CI(poOpenInfo->pszFilename, "EOPF:"))
    {
        // strip the "EOPF:" prefix
		poDS->m_osPath = std::string(poOpenInfo->pszFilename + 5);
    }

    // Find the Zarr version and parse metadata
    VSIStatBufL sStat;
    CPLString osZarrJson = CPLFormFilename(poDS->m_osPath.c_str(), "zarr.json", nullptr);
    if (VSIStatL(osZarrJson, &sStat) == 0)
    {
        poDS->m_bIsZarrV3 = true;
		if (!poDS->ParseZarrMetadata(osZarrJson.c_str())) {
			delete poDS;
			return nullptr;
		}
    }
    else
    {
        // Fallback: try .zarray (assume Zarr V2)
        CPLString osZarray = CPLFormFilename(poDS->m_osPath.c_str(), ".zarray", nullptr);
        if (VSIStatL(osZarray, &sStat) == 0)
        {
            poDS->m_bIsZarrV3 = false;
            if (!poDS->ParseZarrMetadata(osZarray.c_str()))
            {
                delete poDS;
                return nullptr;
            }
        }
        else
        {
            CPLError(CE_Failure, CPLE_AppDefined, "No Zarr metadata found in %s", poDS->m_osPath.c_str());
            delete poDS;
            return nullptr;
        }
    }

    // Create raster bands based on metadata.
    // For simplicity, assume one band. In a full implementation, iterate over available arrays.
    poDS->nBands = 1;
    poDS->SetBand(1, new EOPFRasterBand(poDS, 1, GDT_Float32));

    // Set geotransform and projection if available.
    // For this example, we'll use a dummy geotransform.
    double adfGeoTransform[6] = { 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    poDS->SetGeoTransform(adfGeoTransform);
    poDS->SetProjection("LOCAL_CS[\"EOPF_Zarr\"]");

    return poDS;
}

// Initialize dataset
bool EOPFDataset::Initialize(const char* pszFilename, EOPFMode eMode)
{
    m_osPath = pszFilename;
    m_eMode = eMode;

    // If filename starts with EOPF:, extract the actual path
    if (STARTS_WITH_CI(pszFilename, "EOPF:")) {
        m_osPath = pszFilename + 5;  // Skip "EOPF:"
    }

    // Check Zarr version by looking for zarr.json (V3) or .zarray (V2)
    VSIStatBufL sStat;
    CPLString osZarrJsonPath = CPLFormFilename(m_osPath.c_str(), "zarr.json", nullptr);

    if (VSIStatL(osZarrJsonPath, &sStat) == 0) {
        m_bIsZarrV3 = true;
        return ParseZarrMetadata(osZarrJsonPath.c_str());
    }
    else {
        // If not V3, check for V2
        CPLString osZarrayPath = CPLFormFilename(m_osPath.c_str(), ".zarray", nullptr);
        if (VSIStatL(osZarrayPath, &sStat) == 0) {
            m_bIsZarrV3 = false;
            return ParseZarrMetadata(osZarrayPath.c_str());
        }
        else {
            CPLError(CE_Failure, CPLE_AppDefined,
                "Could not find zarr.json or .zarray in %s", m_osPath.c_str());
            return false;
        }
    }
}

// Parse Zarr metadata
// Add this to the ParseZarrMetadata method or Initialize method
bool EOPFDataset::ParseZarrMetadata(const char* pszMetadataPath) {
    CPLJSONDocument oDoc;
    if (!oDoc.Load(pszMetadataPath)) {
        CPLError(CE_Failure, CPLE_AppDefined,
            "Failed to load Zarr metadata from %s", pszMetadataPath);
        return false;
    }

    CPLJSONObject oRoot = oDoc.GetRoot();
    if (!oRoot.IsValid()) {
        CPLError(CE_Failure, CPLE_AppDefined,
            "Invalid JSON structure in %s", pszMetadataPath);
        return false;
    }

    // Parse "shape" array [rows, cols]
    CPLJSONArray oShape = oRoot.GetArray("shape");
    if (!oShape.IsValid() || oShape.Size() < 2) {
        CPLError(CE_Failure, CPLE_AppDefined,
            "Invalid or missing 'shape' in Zarr metadata");
        return false;
    }
    nRasterYSize = oShape[0].ToInteger();
    nRasterXSize = oShape[1].ToInteger();

    // Parse "chunks" array [chunk_rows, chunk_cols]
    CPLJSONArray oChunks = oRoot.GetArray("chunks");
    if (oChunks.IsValid() && oChunks.Size() >= 2) {
        chunkSizeY = oChunks[0].ToInteger();
        chunkSizeX = oChunks[1].ToInteger();
    }
    else {
        CPLError(CE_Warning, CPLE_AppDefined,
            "Missing chunk size info, defaulting to 256x256");
        chunkSizeX = 256;
        chunkSizeY = 256;
    }

    // Set metadata
    SetMetadataItem("ZARR_SHAPE", CPLSPrintf("%dx%d", nRasterXSize, nRasterYSize));
    SetMetadataItem("CHUNK_SIZE", CPLSPrintf("%dx%d", chunkSizeX, chunkSizeY));

    return true;
}



// Override GetGeoTransform
CPLErr EOPFDataset::GetGeoTransform(double* padfTransform)
{
    padfTransform[0] = 0.0;  // Origin X
    padfTransform[1] = 1.0;  // Pixel width
    padfTransform[2] = 0.0;  // Rotation (row/column)
    padfTransform[3] = 0.0;  // Origin Y
    padfTransform[4] = 0.0;  // Rotation (row/column)
    padfTransform[5] = 1.0;  // Pixel height

    return CE_None;
}
