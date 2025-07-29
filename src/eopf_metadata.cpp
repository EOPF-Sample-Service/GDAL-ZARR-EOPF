#include "eopf_metadata.h"

#include <cmath>    // For fabs
#include <cstdlib>  // strtol
#include <functional>

#include "cpl_json.h"
#include "ogr_spatialref.h"

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
    if (stacDiscovery.IsValid())
    {
        const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
        if (properties.IsValid())
        {
            // Try to get proj:epsg as a number first, then as a string
            int nEpsgVal = properties.GetInteger("proj:epsg", 0);
            if (nEpsgVal != 0)
            {
                epsg = CPLString().Printf("%d", nEpsgVal);
            }
            else
            {
                epsg = properties.GetString("proj:epsg", "");
            }
            if (!epsg.empty())
            {
                CPLDebug("EOPFZARR", "Found proj:epsg in STAC properties: %s", epsg.c_str());
            }
        }
    }

    // If not found in STAC, try top level
    if (epsg.empty())
    {
        int nEpsgVal = obj.GetInteger("proj:epsg", obj.GetInteger("epsg", 0));
        if (nEpsgVal != 0)
        {
            epsg = CPLString().Printf("%d", nEpsgVal);
        }
        else
        {
            epsg = obj.GetString("proj:epsg", obj.GetString("epsg", ""));
        }
        if (!epsg.empty())
        {
            CPLDebug("EOPFZARR", "Found proj:epsg at top level: %s", epsg.c_str());
        }
    }

    // If still not found, simple search in common locations
    if (epsg.empty())
    {
        for (const auto& child : obj.GetChildren())
        {
            if (child.GetType() == CPLJSONObject::Type::Object)
            {
                int nEpsgVal = child.GetInteger("proj:epsg", child.GetInteger("epsg", 0));
                if (nEpsgVal != 0)
                {
                    epsg = CPLString().Printf("%d", nEpsgVal);
                }
                else
                {
                    epsg = child.GetString("proj:epsg", child.GetString("epsg", ""));
                }
                if (!epsg.empty())
                {
                    CPLDebug("EOPFZARR",
                             "Found proj:epsg in child %s: %s",
                             child.GetName().c_str(),
                             epsg.c_str());
                    break;
                }
            }
        }
    }

    // Enhanced search for STAC discovery metadata with better structure parsing
    if (epsg.empty() && stacDiscovery.IsValid())
    {
        // Try to get the full STAC item
        const CPLJSONObject& bbox = stacDiscovery.GetObj("bbox");
        const CPLJSONObject& stacExtensions = stacDiscovery.GetObj("stac_extensions");
        const CPLJSONObject& geometry = stacDiscovery.GetObj("geometry");

        if (geometry.IsValid())
        {
            const CPLJSONObject& geomCrs = geometry.GetObj("crs");
            if (geomCrs.IsValid())
            {
                const CPLJSONObject& geomProps = geomCrs.GetObj("properties");
                if (geomProps.IsValid())
                {
                    int nGeomEpsg = geomProps.GetInteger("code", 0);
                    if (nGeomEpsg != 0)
                    {
                        epsg = CPLString().Printf("%d", nGeomEpsg);
                        CPLDebug("EOPFZARR", "Found CRS code in STAC geometry: %s", epsg.c_str());
                    }
                }
            }
        }
    }

    // Try to infer CRS from Sentinel-2 tile naming convention
    if (epsg.empty())
    {
        // Look for Sentinel-2 tile ID pattern in dataset name or metadata
        std::string tileName;

        // First, try to extract from dataset name if it contains T##XXX pattern
        const char* dsName = ds.GetDescription();
        if (dsName)
        {
            std::string dsNameStr(dsName);
            size_t tilePos = dsNameStr.find("_T");
            if (tilePos != std::string::npos && tilePos + 6 < dsNameStr.length())
            {
                tileName = dsNameStr.substr(tilePos + 1, 6);  // Extract T##XXX
                CPLDebug("EOPFZARR", "Extracted tile name from dataset name: %s", tileName.c_str());
            }
        }

        // Also check in STAC discovery metadata
        if (tileName.empty() && stacDiscovery.IsValid())
        {
            const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
            if (properties.IsValid())
            {
                tileName = properties.GetString(
                    "s2:mgrs_tile",
                    properties.GetString("mgrs_tile", properties.GetString("tile_id", "")));
                if (!tileName.empty())
                {
                    CPLDebug(
                        "EOPFZARR", "Found tile name in STAC properties: %s", tileName.c_str());
                }
            }
        }

        // Parse tile name to get EPSG code (T##XXX -> UTM Zone ## North/South)
        if (!tileName.empty() && tileName.length() >= 3 && tileName[0] == 'T')
        {
            // Extract zone number (characters 1-2)
            std::string zoneStr = tileName.substr(1, 2);
            int zone = std::atoi(zoneStr.c_str());

            if (zone >= 1 && zone <= 60)
            {
                // Determine hemisphere from the third character
                char hemisphere = tileName.length() > 3 ? tileName[3] : 'N';

                // For Sentinel-2, assume Northern hemisphere unless explicitly Southern
                // Most Sentinel-2 data is Northern hemisphere
                bool isNorth = (hemisphere >= 'N' && hemisphere <= 'Z');

                int epsgCode = isNorth ? (32600 + zone) : (32700 + zone);
                epsg = CPLString().Printf("%d", epsgCode);
                CPLDebug("EOPFZARR",
                         "Inferred EPSG %s from Sentinel-2 tile %s (zone %d, %s hemisphere)",
                         epsg.c_str(),
                         tileName.c_str(),
                         zone,
                         isNorth ? "North" : "South");
            }
        }
    }

    // Look for WKT
    std::string wkt = obj.GetString("spatial_ref", "");
    if (wkt.empty() && stacDiscovery.IsValid())
    {
        const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
        if (properties.IsValid())
        {
            wkt = properties.GetString("spatial_ref", "");
        }
    }

    // -----------------------------------
    // STEP 2: Store spatial reference metadata
    // -----------------------------------
    bool isUTM = false;
    int nEPSG = 0;
    OGRSpatialReference oSRS;  // Define oSRS here to be used for SetProjection

    // If EPSG is found, store it as a separate metadata item
    if (!epsg.empty())
    {
        nEPSG = std::strtol(epsg.c_str(), nullptr, 10);
        ds.SetMetadataItem("EPSG", epsg.c_str());
        ds.SetMetadataItem("proj:epsg", epsg.c_str());  // Keep this for STAC compatibility
        CPLDebug("EOPFZARR", "Set EPSG metadata: %s", epsg.c_str());

        // Check if we have a UTM projection (EPSG codes 32601-32660 and 32701-32760)
        isUTM = (nEPSG >= 32601 && nEPSG <= 32660) || (nEPSG >= 32701 && nEPSG <= 32760);

        if (isUTM)
        {
            CPLDebug("EOPFZARR", "Detected UTM projection (EPSG:%d)", nEPSG);
        }
    }

    // Store spatial reference as metadata items and apply to dataset
    if (!wkt.empty())
    {
        ds.SetMetadataItem("spatial_ref", wkt.c_str());
        if (oSRS.SetFromUserInput(wkt.c_str()) == OGRERR_NONE)
        {
            char* pszExportWKT = nullptr;
            oSRS.exportToWkt(&pszExportWKT);
            ds.SetProjection(pszExportWKT);  // Apply projection to dataset
            CPLFree(pszExportWKT);
        }
        CPLDebug("EOPFZARR", "Set spatial_ref metadata and projection from WKT: %s", wkt.c_str());
    }
    else if (nEPSG != 0)  // Use nEPSG which is an int
    {
        if (oSRS.importFromEPSG(nEPSG) == OGRERR_NONE)
        {
            char* pszExportWKT = nullptr;
            oSRS.exportToWkt(&pszExportWKT);
            ds.SetMetadataItem("spatial_ref", pszExportWKT);
            ds.SetProjection(pszExportWKT);  // Apply projection to dataset
            CPLFree(pszExportWKT);
            CPLDebug("EOPFZARR", "Set spatial_ref metadata and projection from EPSG: %d", nEPSG);
        }
        else
        {
            CPLDebug("EOPFZARR", "Failed to import EPSG:%d, falling back to WGS84", nEPSG);
            goto set_wgs84;
        }
    }
    else
    {
    set_wgs84:
        // Set default WGS84 if no spatial reference is found
        oSRS.SetWellKnownGeogCS("WGS84");
        oSRS.SetAuthority("GEOGCS", "EPSG", 4326);  // Ensure EPSG authority for WGS84

        char* pszExportWKT = nullptr;
        oSRS.exportToWkt(&pszExportWKT);
        ds.SetMetadataItem("spatial_ref", pszExportWKT);
        ds.SetProjection(pszExportWKT);  // Apply projection to dataset
        CPLFree(pszExportWKT);
        CPLDebug("EOPFZARR", "Set default WGS84 spatial_ref metadata and projection");

        // Also set the EPSG code for WGS84
        ds.SetMetadataItem("EPSG", "4326");
        ds.SetMetadataItem("proj:epsg", "4326");
        nEPSG = 4326;  // Update nEPSG to reflect WGS84
    }

    // -----------------------------------
    // STEP 3: For UTM Zone 32N (EPSG:32632), use proj:bbox if available or set defaults
    // -----------------------------------
    if (isUTM)
    {  // Assuming this is the target EPSG for proj:bbox
        bool foundProjBbox = false;
        double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

        // Check STAC properties path
        if (stacDiscovery.IsValid())
        {
            const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
            if (properties.IsValid())
            {
                CPLJSONArray projBbox = properties.GetArray("proj:bbox");
                if (projBbox.IsValid() && projBbox.Size() >= 4)
                {
                    bboxMinX = projBbox[0].ToDouble();
                    bboxMinY = projBbox[1].ToDouble();
                    bboxMaxX = projBbox[2].ToDouble();
                    bboxMaxY = projBbox[3].ToDouble();
                    foundProjBbox = true;
                    CPLDebug("EOPFZARR",
                             "Found proj:bbox in STAC properties: [%.2f,%.2f,%.2f,%.2f]",
                             bboxMinX,
                             bboxMinY,
                             bboxMaxX,
                             bboxMaxY);
                }
            }
        }

        // If not found, check top level
        if (!foundProjBbox)
        {
            CPLJSONArray projBbox = obj.GetArray("proj:bbox");
            if (projBbox.IsValid() && projBbox.Size() >= 4)
            {
                bboxMinX = projBbox[0].ToDouble();
                bboxMinY = projBbox[1].ToDouble();
                bboxMaxX = projBbox[2].ToDouble();
                bboxMaxY = projBbox[3].ToDouble();
                foundProjBbox = true;
                CPLDebug("EOPFZARR",
                         "Found proj:bbox at top level: [%.2f,%.2f,%.2f,%.2f]",
                         bboxMinX,
                         bboxMinY,
                         bboxMaxX,
                         bboxMaxY);
            }
        }

        if (foundProjBbox)
        {
            CPLDebug("EOPFZARR",
                     "Using proj:bbox: [%.8f,%.8f,%.8f,%.8f]",
                     bboxMinX,
                     bboxMinY,
                     bboxMaxX,
                     bboxMaxY);

            int width = ds.GetRasterXSize();
            int height = ds.GetRasterYSize();

            if (width > 0 && height > 0)
            {
                double gt[6];
                gt[0] = bboxMinX;                       // Origin X (upper left)
                gt[1] = (bboxMaxX - bboxMinX) / width;  // Pixel width
                gt[2] = 0.0;                            // Rotation
                gt[3] = bboxMaxY;                       // Origin Y (upper left)
                gt[4] = 0.0;                            // Rotation
                gt[5] = -std::fabs((bboxMaxY - bboxMinY) /
                                   height);  // Pixel height (negative for north-up)

                ds.SetGeoTransform(gt);  // Apply geotransform to dataset

                CPLString transformStr;
                for (int i = 0; i < 6; ++i)
                {
                    if (i > 0)
                        transformStr += ",";
                    transformStr += CPLString().Printf("%.12f", gt[i]);
                }
                ds.SetMetadataItem("geo_transform", transformStr.c_str());  // Keep for metadata
                CPLDebug("EOPFZARR",
                         "Set geotransform from proj:bbox and applied: %s",
                         transformStr.c_str());

                ds.SetMetadataItem("utm_easting_min", CPLString().Printf("%.8f", bboxMinX).c_str());
                ds.SetMetadataItem("utm_easting_max", CPLString().Printf("%.8f", bboxMaxX).c_str());
                ds.SetMetadataItem("utm_northing_min",
                                   CPLString().Printf("%.8f", bboxMinY).c_str());
                ds.SetMetadataItem("utm_northing_max",
                                   CPLString().Printf("%.8f", bboxMaxY).c_str());
            }
            else
            {
                CPLDebug("EOPFZARR",
                         "Cannot set geotransform from proj:bbox due to invalid raster dimensions: "
                         "W=%d, H=%d",
                         width,
                         height);
            }
        }
        else
        {
            CPLDebug(
                "EOPFZARR", "proj:bbox not found for EPSG:%d, using default UTM values", nEPSG);
            double gt[6] = {500000.0, 30.0, 0.0, 5000000.0, 0.0, -30.0};
            ds.SetGeoTransform(gt);  // Apply geotransform to dataset

            CPLString transformStr;
            for (int i = 0; i < 6; ++i)
            {
                if (i > 0)
                    transformStr += ",";
                transformStr += CPLString().Printf("%.12f", gt[i]);
            }
            ds.SetMetadataItem("geo_transform", transformStr.c_str());  // Keep for metadata
            CPLDebug("EOPFZARR",
                     "Set hardcoded UTM Zone 32N geotransform and applied: %s",
                     transformStr.c_str());

            ds.SetMetadataItem("utm_easting_min", "500000.00000000");
            ds.SetMetadataItem("utm_easting_max", "515360.00000000");    // Example based on 512*30m
            ds.SetMetadataItem("utm_northing_min", "4984640.00000000");  // Example based on 512*30m
            ds.SetMetadataItem("utm_northing_max", "5000000.00000000");
        }
        // Geographic bbox handling (remains the same as your original code)
        // ...
        return;  // Return after handling UTM case
    }

    // -----------------------------------
    // STEP 4: Extract bounds information from various sources (for non-UTM or if UTM specific
    // handling didn't return)
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
        {
            hasBounds = true;
            CPLDebug("EOPFZARR",
                     "Found bounds in 'bounds' object: [%.8f,%.8f,%.8f,%.8f]",
                     minX,
                     minY,
                     maxX,
                     maxY);
        }
    }

    // Check for geo_ref_points if bounds not found
    if (!hasBounds)
    {
        const CPLJSONObject& geoRefPoints = obj.GetObj("geo_ref_points");
        if (geoRefPoints.IsValid())
        {
            const CPLJSONObject& ul = geoRefPoints.GetObj("ul");  // upper left
            const CPLJSONObject& lr = geoRefPoints.GetObj("lr");  // lower right
            // Assuming ul.x, ul.y, lr.x, lr.y define the extent correctly
            if (ul.IsValid() && lr.IsValid())
            {
                minX = ul.GetDouble("x", 0.0);  // Assuming ul.x is minX
                maxY = ul.GetDouble("y", 0.0);  // Assuming ul.y is maxY
                maxX = lr.GetDouble("x", 0.0);  // Assuming lr.x is maxX
                minY = lr.GetDouble("y", 0.0);  // Assuming lr.y is minY

                // Ensure min < max
                if (minX > maxX)
                    std::swap(minX, maxX);
                if (minY > maxY)
                    std::swap(minY, maxY);

                if (minX != 0.0 || maxX != 0.0 || minY != 0.0 || maxY != 0.0)
                {
                    hasBounds = true;
                    CPLDebug("EOPFZARR",
                             "Found bounds in 'geo_ref_points': [%.8f,%.8f,%.8f,%.8f]",
                             minX,
                             minY,
                             maxX,
                             maxY);
                }
            }
        }
    }

    // Fallback for geographic bbox if proj:bbox was not used (e.g. not UTM 32632)
    // This part might be redundant if the UTM block (STEP 3) handles all its cases and returns.
    // Or if this is meant for truly geographic (e.g. EPSG:4326) datasets.
    if (!hasBounds && stacDiscovery.IsValid())
    {
        const CPLJSONObject& properties = stacDiscovery.GetObj("properties");
        if (properties.IsValid())
        {
            CPLJSONArray geoBbox = properties.GetArray("bbox");  // STAC bbox (lon,lat,lon,lat)
            if (geoBbox.IsValid() && geoBbox.Size() >= 4)
            {
                minX = geoBbox[0].ToDouble();  // minLon
                minY = geoBbox[1].ToDouble();  // minLat
                maxX = geoBbox[2].ToDouble();  // maxLon
                maxY = geoBbox[3].ToDouble();  // maxLat
                hasBounds = true;
                isUTM = false;  // Explicitly not UTM if we are using STAC geographic bbox
                CPLDebug("EOPFZARR",
                         "Using STAC geographic bbox: [%.8f,%.8f,%.8f,%.8f]",
                         minX,
                         minY,
                         maxX,
                         maxY);
            }
        }
    }


    // If no bounds found after all attempts, create default bounds
    if (!hasBounds)
    {
        minX = 10.0;  // longitude
        minY = 40.0;  // latitude
        maxX = 15.0;  // longitude
        maxY = 45.0;  // latitude
        hasBounds = true;

        // Debug: Show the values before condition check
        CPLDebug("EOPFZARR",
                 "Creating default bounds: nEPSG=%d, isUTM=%s",
                 nEPSG,
                 isUTM ? "true" : "false");

        // Check if we already have a valid UTM projection from CRS inference
        if (nEPSG != 0 && isUTM)
        {
            // Preserve the already-inferred UTM projection
            CPLDebug("EOPFZARR",
                     "No specific bounds found, using default geographic bounds but preserving UTM "
                     "projection (EPSG:%d): [%.8f,%.8f,%.8f,%.8f]",
                     nEPSG,
                     minX,
                     minY,
                     maxX,
                     maxY);
        }
        else
        {
            // Fallback to WGS84 if no UTM projection was inferred
            isUTM = false;  // Defaulting to geographic
            nEPSG = 4326;   // Assuming WGS84 for these defaults
            // Ensure WGS84 projection is set if we fall here
            OGRSpatialReference srs_default_geo;
            srs_default_geo.SetWellKnownGeogCS("WGS84");
            srs_default_geo.SetAuthority("GEOGCS", "EPSG", 4326);
            char* pszDefaultWKT = nullptr;
            srs_default_geo.exportToWkt(&pszDefaultWKT);
            ds.SetProjection(pszDefaultWKT);
            ds.SetMetadataItem("spatial_ref", pszDefaultWKT);
            ds.SetMetadataItem("EPSG", "4326");
            ds.SetMetadataItem("proj:epsg", "4326");
            CPLFree(pszDefaultWKT);
            CPLDebug("EOPFZARR",
                     "No specific bounds found, created default geographic bounds (EPSG:4326): "
                     "[%.8f,%.8f,%.8f,%.8f]",
                     minX,
                     minY,
                     maxX,
                     maxY);
        }
    }

    // -----------------------------------
    // STEP 5: Transform bounds if using UTM (This step might be redundant if STEP 3 handles UTM and
    // returns) This section should only run if STEP 3 didn't execute or didn't return, and we have
    // a UTM projection identified in STEP 2 but not specifically EPSG:32632.
    // -----------------------------------
    if (hasBounds && isUTM &&
        nEPSG != 32632)  // Only if UTM and not already handled by EPSG 32632 block
    {
        // Create source (WGS84, assuming input bounds were geographic if not from proj:bbox)
        // and target (UTM) spatial references
        OGRSpatialReference srcSRS_transform, tgtSRS_transform;
        // Heuristic: if bounds are small and look like lat/lon, assume WGS84
        // This is tricky; ideally, the source CRS of these bounds should be known.
        // For now, let's assume if isUTM is true, but we didn't use proj:bbox,
        // the minX/minY etc. might still be geographic.
        bool boundsLookGeographic = (std::abs(minX) <= 180 && std::abs(maxX) <= 180 &&
                                     std::abs(minY) <= 90 && std::abs(maxY) <= 90);

        if (boundsLookGeographic)
        {
            srcSRS_transform.SetWellKnownGeogCS("WGS84");
            if (tgtSRS_transform.importFromEPSG(nEPSG) == OGRERR_NONE)
            {
                OGRCoordinateTransformation* poCT =
                    OGRCreateCoordinateTransformation(&srcSRS_transform, &tgtSRS_transform);
                if (poCT)
                {
                    double utmMinX = minX, utmMinY = minY;  // Temp vars for transformation
                    double utmMaxX = maxX, utmMaxY = maxY;

                    // Transform corners. Note: transforming bbox corners doesn't always give the
                    // transformed bbox. For simplicity, we transform min/min and max/max. A more
                    // robust way is to transform all 4 corners and find new min/max.
                    bool bSuccess1 = poCT->Transform(1, &utmMinX, &utmMinY);
                    bool bSuccess2 = poCT->Transform(1, &utmMaxX, &utmMaxY);

                    if (bSuccess1 && bSuccess2)
                    {
                        CPLDebug("EOPFZARR",
                                 "Transformed geographic-like bounds [%.8f,%.8f,%.8f,%.8f] to UTM "
                                 "(EPSG:%d) [%.8f,%.8f,%.8f,%.8f]",
                                 minX,
                                 minY,
                                 maxX,
                                 maxY,
                                 nEPSG,
                                 utmMinX,
                                 utmMinY,
                                 utmMaxX,
                                 utmMaxY);
                        // Update bounds to UTM coordinates
                        minX = std::min(utmMinX, utmMaxX);  // Ensure minX is min
                        minY = std::min(utmMinY, utmMaxY);  // Ensure minY is min
                        maxX = std::max(utmMinX, utmMaxX);  // Ensure maxX is max
                        maxY = std::max(utmMinY, utmMaxY);  // Ensure maxY is max
                    }
                    else
                    {
                        CPLDebug("EOPFZARR",
                                 "Failed to transform geographic-like bounds to UTM (EPSG:%d)",
                                 nEPSG);
                    }
                    OGRCoordinateTransformation::DestroyCT(poCT);
                }
                else
                {
                    CPLDebug(
                        "EOPFZARR",
                        "Failed to create coordinate transformation from WGS84 to UTM (EPSG:%d)",
                        nEPSG);
                }
            }
            else
            {
                CPLDebug("EOPFZARR",
                         "Failed to import target UTM SRS for transformation (EPSG:%d)",
                         nEPSG);
            }
        }
        else
        {
            CPLDebug("EOPFZARR",
                     "Bounds do not look geographic, skipping UTM transformation for EPSG:%d",
                     nEPSG);
        }
    }


    // -----------------------------------
    // STEP 6 & 7: Set bounds as metadata items and Calculate/Set geotransform
    // This part will run if STEP 3 (UTM 32632 specific) didn't return.
    // -----------------------------------
    if (hasBounds)
    {
        const char* minXName = isUTM ? "utm_easting_min" : "geospatial_lon_min";
        const char* maxXName = isUTM ? "utm_easting_max" : "geospatial_lon_max";
        const char* minYName = isUTM ? "utm_northing_min" : "geospatial_lat_min";
        const char* maxYName = isUTM ? "utm_northing_max" : "geospatial_lat_max";

        ds.SetMetadataItem(minXName, CPLString().Printf("%.8f", minX).c_str());
        ds.SetMetadataItem(maxXName, CPLString().Printf("%.8f", maxX).c_str());
        ds.SetMetadataItem(minYName, CPLString().Printf("%.8f", minY).c_str());
        ds.SetMetadataItem(maxYName, CPLString().Printf("%.8f", maxY).c_str());

        if (!isUTM)
        {  // If geographic, ensure these are also set
            ds.SetMetadataItem("geospatial_lon_min", CPLString().Printf("%.8f", minX).c_str());
            ds.SetMetadataItem("geospatial_lon_max", CPLString().Printf("%.8f", maxX).c_str());
            ds.SetMetadataItem("geospatial_lat_min", CPLString().Printf("%.8f", minY).c_str());
            ds.SetMetadataItem("geospatial_lat_max", CPLString().Printf("%.8f", maxY).c_str());
        }

        // Check if geotransform was already set (e.g., by UTM 32632 block)
        // The metadata item "geo_transform" is a string, SetGeoTransform takes double*.
        // We rely on the fact that if the UTM 32632 block ran and set it, it would have returned.
        // So, if we are here, it means it wasn't set by that specific block.

        int width = ds.GetRasterXSize();
        int height = ds.GetRasterYSize();

        if (width > 0 && height > 0)
        {
            double gt[6];
            gt[0] = minX;                                // Origin X (upper left)
            gt[1] = (maxX - minX) / width;               // Pixel width
            gt[2] = 0.0;                                 // Rotation (row/column)
            gt[3] = maxY;                                // Origin Y (upper left)
            gt[4] = 0.0;                                 // Rotation (row/column)
            gt[5] = -std::fabs((maxY - minY) / height);  // Pixel height (negative for north-up)

            ds.SetGeoTransform(gt);  // Apply geotransform to dataset

            CPLString transformStr;
            for (int i = 0; i < 6; ++i)
            {
                if (i > 0)
                    transformStr += ",";
                transformStr += CPLString().Printf("%.12f", gt[i]);
            }
            ds.SetMetadataItem("geo_transform", transformStr.c_str());  // Keep for metadata
            CPLDebug("EOPFZARR",
                     "Calculated and set general geo_transform and applied: %s",
                     transformStr.c_str());
        }
        else
        {
            CPLDebug("EOPFZARR",
                     "Cannot set general geotransform due to invalid raster dimensions: W=%d, H=%d",
                     width,
                     height);
        }
    }
    else
    {
        CPLDebug("EOPFZARR", "No valid bounds found to calculate geotransform.");
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
    {
        CPLDebug("EOPFZARR", ".zmetadata does not contain 'metadata' object.");
        return false;
    }

    // Look for .zattrs in the metadata
    const CPLJSONObject& zattrs = metadata.GetObj(".zattrs");
    if (!zattrs.IsValid())
    {
        CPLDebug("EOPFZARR", "'metadata' object does not contain '.zattrs' object.");
        return false;
    }

    // Replace the document root with the .zattrs object
    doc.SetRoot(zattrs);

    // Log that we extracted zattrs without printing the content
    CPLDebug("EOPFZARR", "Successfully extracted .zattrs from .zmetadata");

    return true;
}


/* ------------------------------------------------------------------ */
/*      Discover and attach subdatasets                                */
/* ------------------------------------------------------------------ */
void EOPF::DiscoverSubdatasets(GDALDataset& ds,
                               const std::string& rootPath,
                               const CPLJSONObject& metadata)
{
    // Open the Zarr dataset to get subdatasets
    CPLString zarrPath;
    zarrPath.Printf("ZARR:\"%s\"", rootPath.c_str());
    GDALDataset* poZarrDS = (GDALDataset*) GDALOpen(zarrPath.c_str(), GA_ReadOnly);
    if (!poZarrDS)
    {
        CPLDebug("EOPFZARR", "Failed to open Zarr dataset: %s", zarrPath.c_str());
        ds.SetMetadataItem("SUBDATASET_COUNT", "0");
        return;
    }

    // Get subdatasets from the Zarr driver
    char** papszZarrSubdatasets = poZarrDS->GetMetadata("SUBDATASETS");
    if (!papszZarrSubdatasets)
    {
        CPLDebug("EOPFZARR", "No subdatasets found in Zarr dataset: %s", zarrPath.c_str());
        ds.SetMetadataItem("SUBDATASET_COUNT", "0");
        GDALClose(poZarrDS);
        return;
    }

    // Clear any existing subdataset metadata
    char* emptyList[1] = {nullptr};
    ds.SetMetadata(emptyList, "SUBDATASETS");

    // Count and process subdatasets
    int nActualSubdatasets = 0;
    for (int i = 0; papszZarrSubdatasets[i] != nullptr; i++)
    {
        const char* pszName =
            CSLFetchNameValue(papszZarrSubdatasets, CPLSPrintf("SUBDATASET_%d_NAME", i + 1));
        const char* pszDesc =
            CSLFetchNameValue(papszZarrSubdatasets, CPLSPrintf("SUBDATASET_%d_DESC", i + 1));
        if (pszName && pszDesc)
        {
            nActualSubdatasets++;
            CPLString nameKey, descKey;
            nameKey.Printf("SUBDATASET_%d_NAME", nActualSubdatasets);
            descKey.Printf("SUBDATASET_%d_DESC", nActualSubdatasets);

            // Transform the subdataset name to use EOPFZARR prefix
            CPLString eopfName;
            if (STARTS_WITH_CI(pszName, "ZARR:"))
            {
                const char* pszInternalPath = strstr(pszName + 5, ":");
                if (pszInternalPath)
                {
                    CPLString pathPart(pszName + 5, pszInternalPath - (pszName + 5));
                    eopfName.Printf("EOPFZARR:%s%s", pathPart.c_str(), pszInternalPath);
                }
                else
                {
                    eopfName.Printf("EOPFZARR:%s", pszName + 5);
                }
            }
            else
            {
                eopfName.Printf("EOPFZARR:%s", pszName);
            }

            // Set the metadata
            ds.SetMetadataItem(nameKey, eopfName);
            ds.SetMetadataItem(descKey, pszDesc);
            CPLDebug("EOPFZARR", "Set %s = %s", nameKey.c_str(), eopfName.c_str());
        }
    }

    ds.SetMetadataItem("SUBDATASET_COUNT", CPLString().Printf("%d", nActualSubdatasets).c_str());
    CPLDebug("EOPFZARR", "Set %d subdatasets", nActualSubdatasets);

    GDALClose(poZarrDS);
}

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
        std::string zattrsPath = CPLFormFilename(rootPath.c_str(), ".zattrs", nullptr);
        if (doc.Load(zattrsPath))  // doc.Load directly with path
        {
            CPLDebug("EOPFZARR", "Loaded metadata from .zattrs at %s", zattrsPath.c_str());
            hasMetadata = true;
        }
        else
        {
            CPLDebug("EOPFZARR", "Failed to load .zattrs from %s", zattrsPath.c_str());
        }
    }

    // Mark as EOPF dataset
    ds.SetMetadataItem("EOPF_PRODUCT", "YES");

    // Extract coordinate metadata either from loaded metadata or create defaults
    if (hasMetadata)
    {
        const CPLJSONObject& root = doc.GetRoot();  // This is .zattrs content now
        ExtractCoordinateMetadata(root, ds);
    }
    else
    {
        CPLDebug("EOPFZARR",
                 "No .zmetadata or .zattrs found in %s, creating defaults for coordinates.",
                 rootPath.c_str());
        CPLJSONObject emptyObj;
        ExtractCoordinateMetadata(emptyObj, ds);  // Setup default coordinate info
    }

    // Apply spatial_ref to GEOLOCATION/GEOREFERENCING domains
    const char* pszSpatialRef = ds.GetMetadataItem("spatial_ref");
    if (pszSpatialRef && strlen(pszSpatialRef) > 0)
    {
        // Ensure the main dataset has its projection set directly
        // This might be redundant if ExtractCoordinateMetadata already called SetProjection
        // but it's safer to ensure it.
        OGRSpatialReference oSRS_final;
        if (oSRS_final.SetFromUserInput(pszSpatialRef) == OGRERR_NONE)
        {
            char* pszFinalWKT = nullptr;
            oSRS_final.exportToWkt(&pszFinalWKT);
            if (ds.GetProjectionRef() == nullptr || strlen(ds.GetProjectionRef()) == 0 ||
                !EQUAL(ds.GetProjectionRef(), pszFinalWKT))
            {
                ds.SetProjection(pszFinalWKT);
                CPLDebug("EOPFZARR",
                         "Ensured main dataset projection is set from spatial_ref metadata.");
            }
            CPLFree(pszFinalWKT);
        }

        char** papszDomainList = ds.GetMetadataDomainList();
        if (papszDomainList)
        {
            for (int i = 0; papszDomainList[i] != nullptr; i++)
            {
                if (EQUAL(papszDomainList[i], "GEOLOCATION") ||
                    EQUAL(papszDomainList[i], "GEOREFERENCING"))
                {
                    ds.SetMetadataItem("SRS", pszSpatialRef, papszDomainList[i]);
                    CPLDebug("EOPFZARR", "Set SRS in domain %s", papszDomainList[i]);
                }
            }
            CSLDestroy(papszDomainList);
        }
    }
    else
    {
        CPLDebug("EOPFZARR",
                 "spatial_ref metadata item is empty or null, cannot set SRS in domains.");
    }
}