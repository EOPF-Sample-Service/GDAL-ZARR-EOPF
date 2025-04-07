#include "EOPFDataset.h"
#include "EOPFRasterBand.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

/************************************************************************/
/*                             Identify()                               */
/************************************************************************/

int EOPFDataset::Identify(GDALOpenInfo* poOpenInfo) {
    if (poOpenInfo->pszFilename == nullptr)
        return FALSE;

    const char* pszFilename = poOpenInfo->pszFilename;

    // Check if the file has a .zarr extension
    if (EQUAL(CPLGetExtension(pszFilename), "zarr"))
        return TRUE;

    // Check for EOPF-Zarr prefix
    if (STARTS_WITH_CI(pszFilename, "EOPF-Zarr:"))
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

GDALDataset* EOPFDataset::Open(GDALOpenInfo* poOpenInfo) {
    if (!Identify(poOpenInfo) || poOpenInfo->eAccess == GA_Update)
        return nullptr;

    auto poDS = std::make_unique<EOPFDataset>();
    poDS->m_osPath = STARTS_WITH_CI(poOpenInfo->pszFilename, "EOPF-Zarr:")
        ? std::string(poOpenInfo->pszFilename + 10)
        : std::string(poOpenInfo->pszFilename);

    // Load metadata
    std::string osZarray = CPLFormFilename(poDS->m_osPath.c_str(), ".zarray", nullptr);
    if (!poDS->ParseZarrMetadata(osZarray)) {
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to parse Zarr metadata");
        return nullptr;
    }

    // Load group structure
    if (!poDS->LoadGroupStructure(poDS->m_osPath)) {
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to load group hierarchy");
        return nullptr;
    }

    // Create subdataset list
    std::vector<std::string> subgroups = poDS->GetSubGroups();
    int nSubDS = 0;
    for (const auto& group : subgroups) {
        poDS->SetMetadataItem(
            CPLSPrintf("SUBDATASET_%d_NAME", ++nSubDS),
            CPLSPrintf("EOPF-Zarr:\"%s\"", group.c_str())
        );
        poDS->SetMetadataItem(
            CPLSPrintf("SUBDATASET_%d_DESC", nSubDS),
            CPLSPrintf("Group: %s", CPLGetFilename(group.c_str()))
        );
    }

    // Create raster bands
    poDS->nRasterXSize = poDS->m_nChunkX * 4;  // Example size for testing
    poDS->nRasterYSize = poDS->m_nChunkY * 4;
    poDS->nBands = 1;
    poDS->SetBand(1, new EOPFRasterBand(poDS.get(), 1, GDT_Float32));

    return poDS.release();
}


// Parse Zarr metadata
// Add this to the ParseZarrMetadata method or Initialize method
bool EOPFDataset::ParseZarrMetadata(const std::string& osMetadataPath) {
    CPLJSONDocument oDoc;
    if (!oDoc.Load(osMetadataPath)) {
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to load %s", osMetadataPath.c_str());
        return false;
    }

    CPLJSONObject oRoot = oDoc.GetRoot();

    // Parse array dimensions
    CPLJSONArray oShape = oRoot.GetArray("shape");
    if (oShape.Size() >= 2) {
        nRasterYSize = oShape[0].ToInteger();
        nRasterXSize = oShape[1].ToInteger();
    }

    // Parse chunks
    CPLJSONArray oChunks = oRoot.GetArray("chunks");
    if (oChunks.Size() >= 2) {
        m_nChunkY = oChunks[0].ToInteger();
        m_nChunkX = oChunks[1].ToInteger();
    }

    // Parse STAC metadata
    CPLJSONObject oSTAC = oRoot.GetObj("stac");
    if (oSTAC.IsValid()) {
        m_osSTACVersion = oSTAC.GetString("version");
        SetMetadataItem("STAC_VERSION", m_osSTACVersion.c_str());
    }

    return true;
}

bool EOPFDataset::LoadGroupStructure(const std::string& osPath)
{
    CPLJSONDocument oDoc;
    std::string osGroupFile = CPLFormFilename(osPath.c_str(), ".zgroup", nullptr);
    if (!oDoc.Load(osGroupFile)) return false;
    m_oRootGroup.osPath = osPath;

    // Load attributes
    std::string osAttrFile = CPLFormFilename(osPath.c_str(), ".zattrs", nullptr);
    if (oDoc.Load(osAttrFile)) {
        CPLJSONObject oRoot = oDoc.GetRoot();
        for (const auto& item : oRoot.GetChildren()) {
            m_oRootGroup.attrs[item.GetName()] = item.ToString();
        }
    }

    // Recursively load subgroups
    std::string osSubGroupsFile = CPLFormFilename(osPath.c_str(), ".zgroup", nullptr);
    if (oDoc.Load(osSubGroupsFile)) {
        CPLJSONArray oGroups = oDoc.GetRoot().GetArray("groups");
        for (int i = 0; i < oGroups.Size(); ++i) {
            GroupInfo subgroup;
            subgroup.osPath = CPLFormFilename(osPath.c_str(),
                oGroups[i].ToString().c_str(),
                nullptr);
            if (LoadGroupStructure(subgroup.osPath)) {
                m_oRootGroup.subgroups.push_back(subgroup);
            }
        }
    }

	// Load arrays
    std::string osArrayfile = CPLFormFilename(osPath.c_str(), ".zarray", nullptr);
    if (oDoc.Load(osArrayfile)) {
        m_oRootGroup.arrays.push_back(osPath);
    }

	return true;
}


void EOPFDataset::ParseBandMetadata(const CPLJSONObject& oBand) {
    std::string osBandName = oBand.GetName();
    std::map<std::string, std::string> bandMetadata;

    bandMetadata["central_wavelength"] = oBand.GetString("central_wavelength");
    bandMetadata["bandwidth"] = oBand.GetString("bandwidth");
    bandMetadata["physical_gain"] = oBand.GetString("physical_gain");

    // Add spectral response (truncated for brevity)
    CPLJSONArray oSpectralResponse = oBand.GetArray("spectral_response_values");
    if (oSpectralResponse.IsValid()) {
        bandMetadata["spectral_response"] = "..."; // Store first 5 values
    }

    m_bandMetadata[osBandName] = bandMetadata;
}

std::vector<std::string> EOPFDataset::GetSubGroups() const {
    std::vector<std::string> subgroups;
    GetSubGroupsRecursive(m_oRootGroup, subgroups);
    return subgroups;
}

std::vector<std::string> EOPFDataset::GetArrays() const {
    return m_oRootGroup.arrays;
}

void EOPFDataset::GetSubGroupsRecursive(const GroupInfo& group,
    std::vector<std::string>& output) const {
    for (const auto& subgroup : group.subgroups) {
        output.push_back(subgroup.osPath);
        GetSubGroupsRecursive(subgroup, output);
    }
}