#include "EOPFDataset.h"
#include "EOPFRasterBand.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

/************************************************************************/
/*                             Identify()                               */
/************************************************************************/

int EOPFDataset::Identify(GDALOpenInfo* poOpenInfo) {
    return STARTS_WITH_CI(poOpenInfo->pszFilename, "EOPF-Zarr:");
}

/************************************************************************/
/*                              Open()                                  */
/************************************************************************/

GDALDataset* EOPFDataset::Open(GDALOpenInfo* poOpenInfo) {
    if (!Identify(poOpenInfo) || poOpenInfo->eAccess == GA_Update)
        return nullptr;

    // Extract path (strip "EOPF-Zarr:" prefix if present)
    std::string osPath = STARTS_WITH_CI(poOpenInfo->pszFilename, "EOPF-Zarr:")
        ? std::string(poOpenInfo->pszFilename + 10)
        : std::string(poOpenInfo->pszFilename);

    auto poDS = std::make_unique<EOPFDataset>();
    poDS->m_osPath = osPath;

    // Check if root is a group
    std::string osZgroup = CPLFormFilename(osPath.c_str(), ".zgroup", nullptr);
    if (VSIStatL(osZgroup.c_str(), nullptr) == 0) {
        // Load group hierarchy
        if (!poDS->LoadGroupStructure(osPath)) {
            CPLError(CE_Failure, CPLE_AppDefined, "Failed to load group structure");
            return nullptr;
        }

        // Create subdataset entries
        std::vector<std::string> subgroups = poDS->GetSubGroups();
        int nSubDs = 0;
        for (const auto& group : subgroups) {
            poDS->SetMetadataItem(
                CPLSPrintf("SUBDATASET_%d_NAME", ++nSubDs),
                CPLSPrintf("EOPF-Zarr:\"%s\"", group.c_str())
            );
            poDS->SetMetadataItem(
                CPLSPrintf("SUBDATASET_%d_DESC", nSubDs),
                CPLSPrintf("Group: %s", CPLGetFilename(group.c_str()))
            );
        }
    }
    // Check if root is an array
    else {
        std::string osZarray = CPLFormFilename(osPath.c_str(), ".zarray", nullptr);
        if (!poDS->ParseZarrMetadata(osZarray)) {
            CPLError(CE_Failure, CPLE_AppDefined, "Failed to parse array metadata");
            return nullptr;
        }
    }

    // Create raster bands
    poDS->nRasterXSize = poDS->m_nChunkX * 4;  // Example size
    poDS->nRasterYSize = poDS->m_nChunkY * 4;
    poDS->SetBand(1, new EOPFRasterBand(poDS.get(), 1, GDT_Float32));

    return poDS.release();
}


/************************************************************************/
/*                        ParseZarrMetadata()                           */
/************************************************************************/

bool EOPFDataset::ParseZarrMetadata(const std::string& osMetadataPath) {
    CPLJSONDocument oDoc;
    if (!oDoc.Load(osMetadataPath)) {
        CPLError(CE_Failure, CPLE_AppDefined, "Can't load %s", osMetadataPath.c_str());
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

    // Parse STAC metadata from parent group
    std::string osParentDir = CPLGetPath(osMetadataPath.c_str());
    std::string osGroupAttrs = CPLFormFilename(osParentDir.c_str(), ".zattrs", nullptr);
    if (oDoc.Load(osGroupAttrs)) {
        CPLJSONObject oSTAC = oDoc.GetRoot().GetObj("stac");
        if (oSTAC.IsValid()) {
            m_osSTACVersion = oSTAC.GetString("version");
            SetMetadataItem("STAC_VERSION", m_osSTACVersion.c_str());
        }
    }

    return true;
}


/************************************************************************/
/*                        LoadGroupStructure()                          */
/************************************************************************/

bool EOPFDataset::LoadGroupStructure(const std::string& osPath) {
    CPLJSONDocument oDoc;
    std::string osGroupFile = CPLFormFilename(osPath.c_str(), ".zgroup", nullptr);

    if (oDoc.Load(osGroupFile)) {
        m_oRootGroup.osPath = osPath;

        // Load attributes
        std::string osAttrFile = CPLFormFilename(osPath.c_str(), ".zattrs", nullptr);
        if (oDoc.Load(osAttrFile)) {
            CPLJSONObject oRoot = oDoc.GetRoot();
            for (const auto& item : oRoot.GetChildren()) {
                m_oRootGroup.attrs[item.GetName()] = item.ToString();
            }
        }

        // Process subgroups
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

    // Process arrays
    std::string osArrayFile = CPLFormFilename(osPath.c_str(), ".zarray", nullptr);
    VSIStatBufL sStat;
    if (oDoc.Load(osArrayFile)) {
        m_oRootGroup.arrays.push_back(osPath);
    }

    return true;
}

/************************************************************************/
/*                        GetSubGroupsRecursive()                       */
/************************************************************************/

void EOPFDataset::GetSubGroupsRecursive(const GroupInfo& group,
    std::vector<std::string>& output) const {
    for (const auto& subgroup : group.subgroups) {
        output.push_back(subgroup.osPath);
        GetSubGroupsRecursive(subgroup, output);
    }
}

/************************************************************************/
/*                           GetSubGroups()                             */
/************************************************************************/

std::vector<std::string> EOPFDataset::GetSubGroups() const {
    std::vector<std::string> subgroups;
    GetSubGroupsRecursive(m_oRootGroup, subgroups);
    return subgroups;
}

/************************************************************************/
/*                        ParseBandMetadata()                           */
/************************************************************************/

void EOPFDataset::ParseBandMetadata(const CPLJSONObject& oBand) {
    std::string osBandName = oBand.GetName();
    std::map<std::string, std::string> bandMetadata;

    bandMetadata["central_wavelength"] = oBand.GetString("central_wavelength");
    bandMetadata["bandwidth"] = oBand.GetString("bandwidth");
    bandMetadata["physical_gain"] = oBand.GetString("physical_gain");

    m_bandMetadata[osBandName] = bandMetadata;
}
