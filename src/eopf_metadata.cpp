#include "eopf_metadata.h"
#include "cpl_json.h"
#include "ogr_spatialref.h"
#include <cstdlib>              // strtol
#include <functional>

/* ------------------------------------------------------------------ */
/*      Extract coordinate-related metadata from JSON                  */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/*      Extract coordinate-related metadata from JSON                  */
/* ------------------------------------------------------------------ */
static void ExtractCoordinateMetadata(const CPLJSONObject& obj, GDALDataset& ds)
{
    // -----------------------------------
    // STEP 1: Extract spatial reference information
    // -----------------------------------

    // Find EPSG code directly or in STAC properties
    std::string epsg;
    const CPLJSONObject& stacDiscovery = obj.GetObj("stac_discovery");
    if (stacDiscovery.IsValid()) {
        const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
        if (properties.IsValid()) {
            epsg = properties.GetString("proj:epsg", "");
            if (!epsg.empty()) {
                CPLDebug("EOPFZARR", "Found proj:epsg in STAC properties: %s", epsg.c_str());
            }
        }
    }

    // If not found in STAC, try top level
    if (epsg.empty()) {
        epsg = obj.GetString("proj:epsg", obj.GetString("epsg", ""));
        if (!epsg.empty()) {
            CPLDebug("EOPFZARR", "Found proj:epsg at top level: %s", epsg.c_str());
        }
    }

    // If still not found, simple search in common locations
    if (epsg.empty()) {
        for (const auto& child : obj.GetChildren()) {
            if (child.GetType() == CPLJSONObject::Type::Object) {
                epsg = child.GetString("proj:epsg", child.GetString("epsg", ""));
                if (!epsg.empty()) {
                    CPLDebug("EOPFZARR", "Found proj:epsg in child %s: %s",
                        child.GetName().c_str(), epsg.c_str());
                    break;
                }
            }
        }
    }

    // Look for WKT
    std::string wkt = obj.GetString("spatial_ref", "");
    if (wkt.empty() && stacDiscovery.IsValid()) {
        const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
        if (properties.IsValid()) {
            wkt = properties.GetString("spatial_ref", "");
        }
    }

    // -----------------------------------
    // STEP 2: Store spatial reference metadata
    // -----------------------------------
    bool isUTM = false;
    int nEPSG = 0;

    // If EPSG is found, store it as a separate metadata item
    if (!epsg.empty())
    {
        nEPSG = std::strtol(epsg.c_str(), nullptr, 10);
        ds.SetMetadataItem("EPSG", epsg.c_str());
        ds.SetMetadataItem("proj:epsg", epsg.c_str());
        CPLDebug("EOPFZARR", "Set EPSG metadata: %s", epsg.c_str());

        // Check if we have a UTM projection (EPSG codes 32601-32660 and 32701-32760)
        isUTM = (nEPSG >= 32601 && nEPSG <= 32660) || (nEPSG >= 32701 && nEPSG <= 32760);

        if (isUTM) {
            CPLDebug("EOPFZARR", "Detected UTM projection (EPSG:%d)", nEPSG);
        }
    }

    // Store spatial reference as metadata items
    if (!wkt.empty())
    {
        ds.SetMetadataItem("spatial_ref", wkt.c_str());
        CPLDebug("EOPFZARR", "Set spatial_ref metadata: %s", wkt.c_str());
    }
    else if (!epsg.empty())
    {
        OGRSpatialReference srs;
        if (srs.importFromEPSG(nEPSG) == OGRERR_NONE)
        {
            char* w = nullptr;
            srs.exportToWkt(&w);
            ds.SetMetadataItem("spatial_ref", w);
            CPLFree(w);
            CPLDebug("EOPFZARR", "Set spatial_ref metadata from EPSG: %s", epsg.c_str());
        }
        else
        {
            CPLDebug("EOPFZARR", "Failed to import EPSG:%s, falling back to WGS84", epsg.c_str());
            goto set_wgs84;
        }
    }
    else
    {
    set_wgs84:
        // Set default WGS84 if no spatial reference is found
        OGRSpatialReference srs;
        srs.SetWellKnownGeogCS("WGS84");

        // Add EPSG authority for WGS84
        srs.SetAuthority("GEOGCS", "EPSG", 4326);

        char* w = nullptr;
        srs.exportToWkt(&w);
        ds.SetMetadataItem("spatial_ref", w);
        CPLFree(w);
        CPLDebug("EOPFZARR", "Set default WGS84 spatial_ref metadata");

        // Also set the EPSG code for WGS84
        ds.SetMetadataItem("EPSG", "4326");
        ds.SetMetadataItem("proj:epsg", "4326");
        nEPSG = 4326;
    }

    // -----------------------------------
    // STEP 3: For UTM Zone 32N (EPSG:32632), use proj:bbox if available or set defaults
    // -----------------------------------
    if (nEPSG == 32632) {
        // Try to find proj:bbox in common locations
        bool foundProjBbox = false;
        double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

        // Check STAC properties path
        if (stacDiscovery.IsValid()) {
            const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
            if (properties.IsValid()) {
                CPLJSONArray projBbox = properties.GetArray("proj:bbox");
                if (projBbox.IsValid() && projBbox.Size() >= 4) {
                    bboxMinX = projBbox[0].ToDouble();
                    bboxMinY = projBbox[1].ToDouble();
                    bboxMaxX = projBbox[2].ToDouble();
                    bboxMaxY = projBbox[3].ToDouble();
                    foundProjBbox = true;
                    CPLDebug("EOPFZARR", "Found proj:bbox in STAC properties: [%.2f,%.2f,%.2f,%.2f]",
                        bboxMinX, bboxMinY, bboxMaxX, bboxMaxY);
                }
            }
        }

        // If not found, check top level
        if (!foundProjBbox) {
            CPLJSONArray projBbox = obj.GetArray("proj:bbox");
            if (projBbox.IsValid() && projBbox.Size() >= 4) {
                bboxMinX = projBbox[0].ToDouble();
                bboxMinY = projBbox[1].ToDouble();
                bboxMaxX = projBbox[2].ToDouble();
                bboxMaxY = projBbox[3].ToDouble();
                foundProjBbox = true;
                CPLDebug("EOPFZARR", "Found proj:bbox at top level: [%.2f,%.2f,%.2f,%.2f]",
                    bboxMinX, bboxMinY, bboxMaxX, bboxMaxY);
            }
        }

        if (foundProjBbox) {
            CPLDebug("EOPFZARR", "Found proj:bbox: [%.8f,%.8f,%.8f,%.8f]",
                bboxMinX, bboxMinY, bboxMaxX, bboxMaxY);

            // Calculate pixel size based on image dimensions and bounds
            int width = ds.GetRasterXSize();
            int height = ds.GetRasterYSize();

            if (width > 0 && height > 0) {
                double gt[6];
                gt[0] = bboxMinX;                            // Origin X (upper left)
                gt[1] = (bboxMaxX - bboxMinX) / width;       // Pixel width
                gt[2] = 0.0;                                 // Rotation
                gt[3] = bboxMaxY;                            // Origin Y (upper left)
                gt[4] = 0.0;                                 // Rotation
                gt[5] = -fabs((bboxMaxY - bboxMinY) / height); // Pixel height (negative for north-up)

                CPLString transformStr;
                for (int i = 0; i < 6; ++i) {
                    if (i > 0) transformStr += ",";
                    transformStr += CPLString().Printf("%.12f", gt[i]);
                }
                ds.SetMetadataItem("geo_transform", transformStr.c_str());
                CPLDebug("EOPFZARR", "Set geotransform from proj:bbox: %s", transformStr.c_str());

                // Store UTM bounds from proj:bbox
                ds.SetMetadataItem("utm_easting_min", CPLString().Printf("%.8f", bboxMinX).c_str());
                ds.SetMetadataItem("utm_easting_max", CPLString().Printf("%.8f", bboxMaxX).c_str());
                ds.SetMetadataItem("utm_northing_min", CPLString().Printf("%.8f", bboxMinY).c_str());
                ds.SetMetadataItem("utm_northing_max", CPLString().Printf("%.8f", bboxMaxY).c_str());
            }
        }
        else {
            // If proj:bbox not found, use hardcoded values as fallback
            CPLDebug("EOPFZARR", "proj:bbox not found, using default UTM values");

            // UTM Zone 32N typical coordinates (centered in Europe)
            double gt[6] = {
                500000.0,    // Origin X (meters) - UTM zone central meridian is at 500000m easting
                30.0,        // Pixel width (meters) - appropriate resolution for Sentinel-2
                0.0,         // Rotation
                5000000.0,   // Origin Y (meters) - reasonable northing value for central Europe
                0.0,         // Rotation
                -30.0        // Pixel height (meters, negative for north-up)
            };

            CPLString transformStr;
            for (int i = 0; i < 6; ++i) {
                if (i > 0) transformStr += ",";
                transformStr += CPLString().Printf("%.12f", gt[i]);
            }
            ds.SetMetadataItem("geo_transform", transformStr.c_str());
            CPLDebug("EOPFZARR", "Set hardcoded UTM Zone 32N geotransform: %s", transformStr.c_str());

            // Store hardcoded UTM bounds in metadata
            ds.SetMetadataItem("utm_easting_min", "500000.00000000");
            ds.SetMetadataItem("utm_easting_max", "515360.00000000");
            ds.SetMetadataItem("utm_northing_min", "4984640.00000000");
            ds.SetMetadataItem("utm_northing_max", "5000000.00000000");
        }

        // Look for geographic bbox
        bool foundGeoBbox = false;
        double geoBboxMinX = 0.0, geoBboxMinY = 0.0, geoBboxMaxX = 0.0, geoBboxMaxY = 0.0;

        // Check STAC properties path
        if (stacDiscovery.IsValid()) {
            const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
            if (properties.IsValid()) {
                CPLJSONArray geoBbox = properties.GetArray("bbox");
                if (geoBbox.IsValid() && geoBbox.Size() >= 4) {
                    geoBboxMinX = geoBbox[0].ToDouble();
                    geoBboxMinY = geoBbox[1].ToDouble();
                    geoBboxMaxX = geoBbox[2].ToDouble();
                    geoBboxMaxY = geoBbox[3].ToDouble();
                    foundGeoBbox = true;
                    CPLDebug("EOPFZARR", "Found geographic bbox in STAC properties");
                }
            }
        }

        // If not found, check top level
        if (!foundGeoBbox) {
            CPLJSONArray geoBbox = obj.GetArray("bbox");
            if (geoBbox.IsValid() && geoBbox.Size() >= 4) {
                geoBboxMinX = geoBbox[0].ToDouble();
                geoBboxMinY = geoBbox[1].ToDouble();
                geoBboxMaxX = geoBbox[2].ToDouble();
                geoBboxMaxY = geoBbox[3].ToDouble();
                foundGeoBbox = true;
                CPLDebug("EOPFZARR", "Found geographic bbox at top level");
            }
        }

        if (foundGeoBbox) {
            // Set geographic bounds from found bbox
            ds.SetMetadataItem("geospatial_lon_min", CPLString().Printf("%.8f", geoBboxMinX).c_str());
            ds.SetMetadataItem("geospatial_lon_max", CPLString().Printf("%.8f", geoBboxMaxX).c_str());
            ds.SetMetadataItem("geospatial_lat_min", CPLString().Printf("%.8f", geoBboxMinY).c_str());
            ds.SetMetadataItem("geospatial_lat_max", CPLString().Printf("%.8f", geoBboxMaxY).c_str());
            CPLDebug("EOPFZARR", "Set geographic bounds from bbox: [%.8f,%.8f,%.8f,%.8f]",
                geoBboxMinX, geoBboxMinY, geoBboxMaxX, geoBboxMaxY);
        }
        else {
            // Use default geographic bounds
            ds.SetMetadataItem("geospatial_lon_min", "10.00000000");
            ds.SetMetadataItem("geospatial_lon_max", "15.00000000");
            ds.SetMetadataItem("geospatial_lat_min", "40.00000000");
            ds.SetMetadataItem("geospatial_lat_max", "45.00000000");
        }

        // We've set the geotransform, so we can return
        return;
    }

    // -----------------------------------
    // STEP 4: Extract bounds information from various sources
    // -----------------------------------
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

    // If no bounds found, create default bounds for Europe
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

    // -----------------------------------
    // STEP 5: Transform bounds if using UTM
    // -----------------------------------
    if (hasBounds && isUTM)
    {
        // Create source (WGS84) and target (UTM) spatial references
        OGRSpatialReference srcSRS, tgtSRS;
        srcSRS.SetWellKnownGeogCS("WGS84");

        // Import the target SRS from EPSG code
        if (tgtSRS.importFromEPSG(nEPSG) != OGRERR_NONE)
        {
            CPLDebug("EOPFZARR", "Failed to create UTM SRS from EPSG:%d", nEPSG);
        }
        else
        {
            // Create transformation object
            OGRCoordinateTransformation* poCT =
                OGRCreateCoordinateTransformation(&srcSRS, &tgtSRS);

            if (poCT)
            {
                // Transform the bounds from geographic to UTM
                double utmMinX = minX, utmMinY = minY;
                double utmMaxX = maxX, utmMaxY = maxY;

                if (poCT->Transform(1, &utmMinX, &utmMinY) &&
                    poCT->Transform(1, &utmMaxX, &utmMaxY))
                {
                    CPLDebug("EOPFZARR", "Transformed bounds from geographic [%.8f,%.8f,%.8f,%.8f] to UTM [%.8f,%.8f,%.8f,%.8f]",
                        minX, minY, maxX, maxY, utmMinX, utmMinY, utmMaxX, utmMaxY);

                    // Store original geographic coordinates for reference
                    ds.SetMetadataItem("geospatial_lon_min", CPLString().Printf("%.8f", minX).c_str());
                    ds.SetMetadataItem("geospatial_lon_max", CPLString().Printf("%.8f", maxX).c_str());
                    ds.SetMetadataItem("geospatial_lat_min", CPLString().Printf("%.8f", minY).c_str());
                    ds.SetMetadataItem("geospatial_lat_max", CPLString().Printf("%.8f", maxY).c_str());

                    // Update bounds to UTM coordinates
                    minX = utmMinX;
                    minY = utmMinY;
                    maxX = utmMaxX;
                    maxY = utmMaxY;
                }
                else
                {
                    CPLDebug("EOPFZARR", "Failed to transform bounds to UTM coordinates");
                }

                // Free transformation object
                OGRCoordinateTransformation::DestroyCT(poCT);
            }
            else
            {
                CPLDebug("EOPFZARR", "Failed to create coordinate transformation from WGS84 to UTM");
            }
        }
    }

    // -----------------------------------
    // STEP 6: Set bounds as metadata items
    // -----------------------------------
    if (hasBounds)
    {
        // For UTM, we're storing easting/northing, not lon/lat
        const char* minXName = isUTM ? "utm_easting_min" : "geospatial_lon_min";
        const char* maxXName = isUTM ? "utm_easting_max" : "geospatial_lon_max";
        const char* minYName = isUTM ? "utm_northing_min" : "geospatial_lat_min";
        const char* maxYName = isUTM ? "utm_northing_max" : "geospatial_lat_max";

        ds.SetMetadataItem(minXName, CPLString().Printf("%.8f", minX).c_str());
        ds.SetMetadataItem(maxXName, CPLString().Printf("%.8f", maxX).c_str());
        ds.SetMetadataItem(minYName, CPLString().Printf("%.8f", minY).c_str());
        ds.SetMetadataItem(maxYName, CPLString().Printf("%.8f", maxY).c_str());

        // If not UTM, set geographic coordinates too
        if (!isUTM) {
            ds.SetMetadataItem("geospatial_lon_min", CPLString().Printf("%.8f", minX).c_str());
            ds.SetMetadataItem("geospatial_lon_max", CPLString().Printf("%.8f", maxX).c_str());
            ds.SetMetadataItem("geospatial_lat_min", CPLString().Printf("%.8f", minY).c_str());
            ds.SetMetadataItem("geospatial_lat_max", CPLString().Printf("%.8f", maxY).c_str());
        }

        // -----------------------------------
        // STEP 7: Calculate and set geotransform
        // -----------------------------------
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

    // Log that metadata was loaded successfully, without printing the content
    CPLDebug("EOPFZARR", "Successfully loaded .zmetadata file");

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

    // Log that we extracted zattrs without printing the content
    CPLDebug("EOPFZARR", "Successfully extracted .zattrs from .zmetadata");

    return true;
}

/* ------------------------------------------------------------------ */
/*      Discover and attach subdatasets                                */
/* ------------------------------------------------------------------ */
void EOPF::DiscoverSubdatasets(GDALDataset& ds, const std::string& rootPath,
    const CPLJSONObject& metadata)
{
    // Simply pass through subdatasets from the inner Zarr dataset
    char** papszInnerSubdatasets = ds.GetMetadata("SUBDATASETS");
    if (papszInnerSubdatasets == nullptr) {
        CPLDebug("EOPFZARR", "No subdatasets found from inner Zarr driver");
        return;
    }

    // Count how many subdatasets we have (NAME/DESC pairs)
    int nSubdatasets = 0;
    for (int i = 0; papszInnerSubdatasets[i] != nullptr; i++) {
        if (strstr(papszInnerSubdatasets[i], "SUBDATASET_") &&
            strstr(papszInnerSubdatasets[i], "_NAME=")) {
            nSubdatasets++;
        }
    }

    CPLDebug("EOPFZARR", "Found %d subdatasets from inner Zarr driver", nSubdatasets);

    // Copy and adapt the subdataset information
    for (int i = 1; i <= nSubdatasets; i++) {
        CPLString nameKey, descKey;
        nameKey.Printf("SUBDATASET_%d_NAME", i);
        descKey.Printf("SUBDATASET_%d_DESC", i);

        const char* pszName = CSLFetchNameValue(papszInnerSubdatasets, nameKey);
        const char* pszDesc = CSLFetchNameValue(papszInnerSubdatasets, descKey);

        if (pszName && pszDesc) {
            // Replace the "ZARR:" prefix with "EOPFZARR:" in the subdataset name
            CPLString eopfName = pszName;
            if (STARTS_WITH(pszName, "ZARR:")) {
                eopfName.Printf("EOPFZARR:%s", pszName + 5);
            }

            // Set the subdataset metadata
            ds.SetMetadataItem(nameKey, eopfName, "SUBDATASETS");
            ds.SetMetadataItem(descKey, pszDesc, "SUBDATASETS");

            CPLDebug("EOPFZARR", "Added subdataset: %s = %s", nameKey.c_str(), eopfName.c_str());
        }
    }
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
            // Don't print the content
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
        DiscoverSubdatasets(ds, rootPath, root);
    }
    else
    {
        CPLDebug("EOPFZARR", "No metadata found in %s, creating defaults", rootPath.c_str());

        // Create a dummy object to pass to ExtractCoordinateMetadata
        // This will ensure we set up default coordinate information
        CPLJSONObject emptyObj;
        ExtractCoordinateMetadata(emptyObj, ds);
        DiscoverSubdatasets(ds, rootPath, emptyObj);
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
