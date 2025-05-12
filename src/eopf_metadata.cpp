#include "eopf_metadata.h"
#include "cpl_json.h"
#include "ogr_spatialref.h"
#include <cstdlib>              // strtol

/* ------------------------------------------------------------------ */
/*      Extract coordinate-related metadata from JSON                  */
/* ------------------------------------------------------------------ */
static void ExtractCoordinateMetadata(const CPLJSONObject& obj, GDALDataset& ds)
{
    // Look for various forms of spatial reference information
    std::string wkt = obj.GetString("spatial_ref", "");
    std::string epsg = obj.GetString("proj:epsg", obj.GetString("epsg", ""));

    // Store spatial reference as metadata items rather than setting directly
    if (!wkt.empty())
    {
        ds.SetMetadataItem("spatial_ref", wkt.c_str());
        CPLDebug("EOPFZARR", "Set spatial_ref metadata: %s", wkt.c_str());
    }
    else if (!epsg.empty())
    {
        OGRSpatialReference srs;
        srs.importFromEPSG(std::strtol(epsg.c_str(), nullptr, 10));
        char* w = nullptr;
        srs.exportToWkt(&w);
        ds.SetMetadataItem("spatial_ref", w);
        CPLFree(w);
        CPLDebug("EOPFZARR", "Set spatial_ref metadata from EPSG: %s", epsg.c_str());
    }
    else
    {
        // Set default WGS84 if no spatial reference is found
        OGRSpatialReference srs;
        srs.SetWellKnownGeogCS("WGS84");
        char* w = nullptr;
        srs.exportToWkt(&w);
        ds.SetMetadataItem("spatial_ref", w);
        CPLFree(w);
        CPLDebug("EOPFZARR", "Set default WGS84 spatial_ref metadata");
    }

    // Look for transform information
    CPLJSONArray transform = obj.GetArray("transform");
    if (!transform.IsValid())
        transform = obj.GetArray("grid_transform");
    if (!transform.IsValid())
        transform = obj.GetArray("geo_transform");

    // Store transform as metadata
    if (transform.IsValid() && transform.Size() == 6)
    {
        CPLString transformStr;
        for (int i = 0; i < 6; ++i)
        {
            if (i > 0) transformStr += ",";
            transformStr += CPLString().Printf("%.12f", transform[i].ToDouble());
        }
        ds.SetMetadataItem("geo_transform", transformStr.c_str());
        CPLDebug("EOPFZARR", "Set geo_transform metadata: %s", transformStr.c_str());
    }

    // Extract bounds information from various sources
    double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
    bool hasBounds = false;

    // Check for direct bounds object
    const CPLJSONObject& bounds = obj.GetObj("bounds");
    if (bounds.IsValid())
    {
        minX = bounds.GetDouble("minx", bounds.GetDouble("left", 0.0));
        maxX = bounds.GetDouble("maxx", bounds.GetDouble("right", 0.0));
        minY = bounds.GetDouble("miny", bounds.GetDouble("bottom", 0.0));
        maxY = bounds.GetDouble("maxy", bounds.GetDouble("top", 0.0));

        if (minX != 0.0 || maxX != 0.0 || minY != 0.0 || maxY != 0.0)
            hasBounds = true;
    }

    // Check for geo_ref_points if bounds not found
    if (!hasBounds)
    {
        const CPLJSONObject& geoRefPoints = obj.GetObj("geo_ref_points");
        if (geoRefPoints.IsValid())
        {
            const CPLJSONObject& ll = geoRefPoints.GetObj("ll"); // lower left
            const CPLJSONObject& lr = geoRefPoints.GetObj("lr"); // lower right
            const CPLJSONObject& ul = geoRefPoints.GetObj("ul"); // upper left
            const CPLJSONObject& ur = geoRefPoints.GetObj("ur"); // upper right

            if (ul.IsValid() && lr.IsValid())
            {
                minX = ul.GetDouble("x", 0.0);
                maxY = ul.GetDouble("y", 0.0);
                maxX = lr.GetDouble("x", 0.0);
                minY = lr.GetDouble("y", 0.0);

                if (minX != 0.0 || maxX != 0.0 || minY != 0.0 || maxY != 0.0)
                    hasBounds = true;
            }
        }
    }

    // If no bounds found, create default bounds for Europe (or your region of interest)
    if (!hasBounds)
    {
        minX = 10.0;     // longitude
        minY = 40.0;     // latitude
        maxX = 15.0;     // longitude
        maxY = 45.0;     // latitude
        hasBounds = true;
        CPLDebug("EOPFZARR", "Created default bounds: [%.8f,%.8f,%.8f,%.8f]",
            minX, minY, maxX, maxY);
    }

    // Set bounds as metadata items
    if (hasBounds)
    {
        ds.SetMetadataItem("geospatial_lon_min", CPLString().Printf("%.8f", minX).c_str());
        ds.SetMetadataItem("geospatial_lon_max", CPLString().Printf("%.8f", maxX).c_str());
        ds.SetMetadataItem("geospatial_lat_min", CPLString().Printf("%.8f", minY).c_str());
        ds.SetMetadataItem("geospatial_lat_max", CPLString().Printf("%.8f", maxY).c_str());

        // Calculate and store a transform as metadata if none exists
        if (!ds.GetMetadataItem("geo_transform"))
        {
            int width = ds.GetRasterXSize();
            int height = ds.GetRasterYSize();

            if (width > 0 && height > 0)
            {
                double gt[6];
                gt[0] = minX;                         // Origin X (upper left)
                gt[1] = (maxX - minX) / width;        // Pixel width
                gt[2] = 0.0;                          // Rotation (row/column)
                gt[3] = maxY;                         // Origin Y (upper left)
                gt[4] = 0.0;                          // Rotation (row/column)
                gt[5] = -fabs((maxY - minY) / height); // Pixel height (negative for north-up)

                CPLString transformStr;
                for (int i = 0; i < 6; ++i)
                {
                    if (i > 0) transformStr += ",";
                    transformStr += CPLString().Printf("%.12f", gt[i]);
                }
                ds.SetMetadataItem("geo_transform", transformStr.c_str());
                CPLDebug("EOPFZARR", "Calculated and set geo_transform metadata: %s",
                    transformStr.c_str());
            }
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

    // Mark as EOPF dataset
    ds.SetMetadataItem("EOPF_PRODUCT", "YES");

    // Extract coordinate metadata either from loaded metadata or create defaults
    if (hasMetadata)
    {
        const CPLJSONObject& root = doc.GetRoot();
        ExtractCoordinateMetadata(root, ds);
    }
    else
    {
        CPLDebug("EOPFZARR", "No metadata found in %s, creating defaults", rootPath.c_str());

        // Create a dummy object to pass to ExtractCoordinateMetadata
        // This will ensure we set up default coordinate information
        CPLJSONObject emptyObj;
        ExtractCoordinateMetadata(emptyObj, ds);
    }

    // Store coordinate info in domain-specific metadata
    // The underlying driver might look for spatial reference in a specific domain
    const char* spatialRef = ds.GetMetadataItem("spatial_ref");
    if (spatialRef)
    {
        char** papszDomainList = ds.GetMetadataDomainList();
        if (papszDomainList)
        {
            for (int i = 0; papszDomainList[i] != nullptr; i++)
            {
                if (EQUAL(papszDomainList[i], "GEOLOCATION") ||
                    EQUAL(papszDomainList[i], "GEOREFERENCING"))
                {
                    ds.SetMetadataItem("SRS", spatialRef, papszDomainList[i]);
                }
            }
            CSLDestroy(papszDomainList);
        }
    }
}
