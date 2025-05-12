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
        if (name == "spatial_ref" || name == "proj:epsg" || name == "epsg" ||
            name == "transform" || name == "grid_transform")
            continue;

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
/*      Create a default geotransform if none is present              */
/* ------------------------------------------------------------------ */
static void
EnsureValidGeotransform(GDALDataset& ds)
{
    double gt[6] = { 0 };

    // If GetGeoTransform returns CE_None and values look like defaults,
    // or if it fails, create a more useful default geotransform
    if (ds.GetGeoTransform(gt) != CE_None ||
        (gt[0] == 0.0 && gt[1] == 1.0 && gt[3] == 0.0 && gt[5] == 1.0))
    {
        // Use bounds info if available
        double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
        const char* boundsX = ds.GetMetadataItem("geospatial_lon_min");
        const char* boundsY = ds.GetMetadataItem("geospatial_lat_min");
        const char* boundsMaxX = ds.GetMetadataItem("geospatial_lon_max");
        const char* boundsMaxY = ds.GetMetadataItem("geospatial_lat_max");

        if (boundsX && boundsY && boundsMaxX && boundsMaxY)
        {
            minX = CPLAtof(boundsX);
            minY = CPLAtof(boundsY);
            maxX = CPLAtof(boundsMaxX);
            maxY = CPLAtof(boundsMaxY);

            int width = ds.GetRasterXSize();
            int height = ds.GetRasterYSize();

            if (width > 0 && height > 0)
            {
                // Calculate proper pixel size
                gt[0] = minX;                         // Origin X (upper left)
                gt[1] = (maxX - minX) / width;        // Pixel width
                gt[2] = 0.0;                          // Rotation (row/column)
                gt[3] = maxY;                         // Origin Y (upper left)
                gt[4] = 0.0;                          // Rotation (row/column)
                gt[5] = -fabs((maxY - minY) / height); // Pixel height (negative for north-up)

                CPLDebug("EOPFZARR", "Setting geotransform from bounds: [%f,%f,%f,%f,%f,%f]",
                    gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);

                ds.SetGeoTransform(gt);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*      Process EOPF-specific band metadata                           */
/* ------------------------------------------------------------------ */
static void
ProcessEOPFBandMetadata(const CPLJSONObject& root, GDALDataset& ds)
{
    const CPLJSONObject& otherMeta = root.GetObj("other_metadata");
    if (!otherMeta.IsValid())
        return;

    const CPLJSONObject& bandDesc = otherMeta.GetObj("band_description");
    if (!bandDesc.IsValid())
        return;

    // Process each band's metadata
    for (const auto& band : bandDesc.GetChildren())
    {
        std::string bandName = band.GetName();
        int bandIndex = -1;

        // Try to convert band name to index (works for "01", "02", etc.)
        try {
            bandIndex = std::stoi(bandName);
        }
        catch (...) {
            // Handle special bands like "8A" - map to appropriate index
            if (bandName == "8A" || bandName == "b8a")
                bandIndex = 8;  // Just an example, adjust based on your band indexing
            else if (bandName.substr(0, 1) == "b" && bandName.length() > 1)
                // Handle "b01", "b02" format by removing the 'b' prefix
                try {
                bandIndex = std::stoi(bandName.substr(1));
            }
            catch (...) {
                // Skip if we can't determine band index
                continue;
            }
        }

        // Skip if we couldn't determine a valid band index
        if (bandIndex <= 0 || bandIndex > ds.GetRasterCount())
            continue;

        GDALRasterBand* pBand = ds.GetRasterBand(bandIndex);
        if (!pBand)
            continue;

        // Extract and set key band properties
        for (const auto& property : band.GetChildren())
        {
            std::string propName = property.GetName();
            std::string propValue = property.ToString();

            // Set as band metadata with EOPF domain
            pBand->SetMetadataItem(propName.c_str(), propValue.c_str(), "EOPF");

            // Also set special properties as band-level properties
            if (propName == "central_wavelength")
                pBand->SetMetadataItem("WAVELENGTH", propValue.c_str());
            else if (propName == "bandwidth")
                pBand->SetMetadataItem("BANDWIDTH", propValue.c_str());
        }
    }
}

/* ------------------------------------------------------------------ */
/*      Check for .zmetadata file that may have consolidated metadata */
/* ------------------------------------------------------------------ */
static bool LoadZMetadata(const std::string& rootPath, CPLJSONDocument& doc)
{
    std::string zmetaPath = CPLFormFilename(rootPath.c_str(), ".zmetadata", nullptr);

    if (!doc.Load(zmetaPath))
        return false;

    // In .zmetadata, the actual content is in a "metadata" sub-object
    const CPLJSONObject& root = doc.GetRoot();
    const CPLJSONObject& metadata = root.GetObj("metadata");

    if (!metadata.IsValid())
        return false;

    // Look for .zattrs in the metadata
    const CPLJSONObject& zattrs = metadata.GetObj(".zattrs");
    if (!zattrs.IsValid())
        return false;

    // Replace the document root with the .zattrs object
    doc.SetRoot(zattrs);
    return true;
}

/* ------------------------------------------------------------------ */
/*      AttachMetadata main entry                                      */
/* ------------------------------------------------------------------ */
void EOPF::AttachMetadata(GDALDataset& ds, const std::string& rootPath)
{
    CPLJSONDocument doc;
    bool hasMetadata = false;

    // First try to load from .zmetadata (consolidated)
    if (LoadZMetadata(rootPath, doc))
    {
        CPLDebug("EOPFZARR", "Loaded metadata from .zmetadata (consolidated format)");
        hasMetadata = true;
    }
    else
    {
        // Fall back to .zattrs
        std::string zattrs = CPLFormFilename(rootPath.c_str(), ".zattrs", nullptr);
        if (doc.Load(zattrs))
        {
            CPLDebug("EOPFZARR", "Loaded metadata from .zattrs");
            hasMetadata = true;
        }
    }

    if (!hasMetadata)
    {
        CPLDebug("EOPFZARR", "No metadata found in %s", rootPath.c_str());
        return;  // no metadata → leave pixels
    }

    const CPLJSONObject& root = doc.GetRoot();

    // Mark as EOPF dataset for identification
    if (root.GetObj("other_metadata").GetString("eopf_category", "") == "eoproduct")
    {
        ds.SetMetadataItem("EOPF_PRODUCT", "YES");
        ds.SetMetadataItem("PRODUCT_TYPE", "SENTINEL2");
    }

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

    // Check for the geo_transform property if transform/grid_transform not found
    if (!tr.IsValid())
    {
        tr = root.GetArray("geo_transform");
    }

    // If we found a valid transform array with 6 elements
    if (tr.IsValid() && tr.Size() == 6)
    {
        double gt[6];
        for (int i = 0; i < 6; ++i) gt[i] = tr[i].ToDouble();

        // Validate geotransform values - make sure they're not all 0,1,0,0,0,1
        bool isDefaultTransform = (gt[0] == 0.0 && gt[1] == 1.0 && gt[2] == 0.0 &&
            gt[3] == 0.0 && gt[4] == 0.0 && gt[5] == 1.0);

        if (!isDefaultTransform) {
            CPLDebug("EOPFZARR", "Setting geotransform: [%f,%f,%f,%f,%f,%f]",
                gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);
            ds.SetGeoTransform(gt);
        }
        else {
            CPLDebug("EOPFZARR", "Found default-looking geotransform, will try to derive a better one");
        }
    }
    else {
        CPLDebug("EOPFZARR", "No valid geotransform found in metadata");
    }

    /* ---- process band-specific metadata --------------------------- */
    ProcessEOPFBandMetadata(root, ds);

    /* ---- copy all remaining keys ---------------------------------- */
    FlattenObject(root, "", ds);

    /* ---- ensure we have a usable geotransform -------------------- */
    EnsureValidGeotransform(ds);
}
