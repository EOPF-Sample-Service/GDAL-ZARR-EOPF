#include "eopf_metadata.h"
#include "cpl_json.h"
#include "ogr_spatialref.h"
#include <cstdlib>              // strtol

/* ------------------------------------------------------------------ */
/*      Helper: flatten nested JSON objects to "prefix.key" pairs      */
/* ------------------------------------------------------------------ */
static void
FlattenObject(const CPLJSONObject& obj,
    const std::string& prefix,
    GDALDataset& ds)
{
    for (const auto& child : obj.GetChildren())
    {
        std::string name = child.GetName();
        std::string key = prefix.empty() ? name : prefix + "." + name;

        if (child.GetType() == CPLJSONObject::Type::String ||
            child.GetType() == CPLJSONObject::Type::Integer ||
            child.GetType() == CPLJSONObject::Type::Double)
        {
            ds.SetMetadataItem(key.c_str(), child.ToString().c_str());
        }
        else if (child.GetType() == CPLJSONObject::Type::Object)
        {
            FlattenObject(child, key, ds);          // recursive step
        }
        else
        {   /* arrays or other types: stringify */
            ds.SetMetadataItem(key.c_str(), child.ToString().c_str());
        }
    }
}

/* ------------------------------------------------------------------ */
/*      AttachMetadata main entry                                      */
/* ------------------------------------------------------------------ */
void EOPF::AttachMetadata(GDALDataset& ds, const std::string& rootPath)
{
    std::string zattrs =
        CPLFormFilename(rootPath.c_str(), ".zattrs", nullptr);

    CPLJSONDocument doc;
    if (!doc.Load(zattrs))
        return;                         // no .zattrs → leave pixels

    const CPLJSONObject& root = doc.GetRoot();

    /* ---- spatial reference ---------------------------------------- */
    std::string wkt = root.GetString("spatial_ref", "");
    std::string epsg = root.GetString("proj:epsg",
        root.GetString("epsg", ""));

    OGRSpatialReference srs;
    if (!wkt.empty())
        srs.SetFromUserInput(wkt.c_str());
    else if (!epsg.empty())
        srs.importFromEPSG(std::strtol(epsg.c_str(), nullptr, 10));

    if (!srs.IsEmpty())
    {
        char* w = nullptr;  srs.exportToWkt(&w);
        ds.SetProjection(w);  CPLFree(w);
    }

    /* ---- affine transform ----------------------------------------- */
    CPLJSONArray tr = root.GetArray("transform");
    if (!tr.IsValid()) tr = root.GetArray("grid_transform");
    if (tr.IsValid() && tr.Size() == 6)
    {
        double gt[6];
        for (int i = 0; i < 6; ++i) gt[i] = tr[i].ToDouble();
        ds.SetGeoTransform(gt);
    }

    /* ---- copy all remaining keys ---------------------------------- */
    FlattenObject(root, "", ds);
}
