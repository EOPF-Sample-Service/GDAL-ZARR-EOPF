<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" class="logo" width="120"/>

# 

---

### **Current Capabilities of the GDAL Zarr Driver**

The GDAL Zarr driver, introduced in GDAL 3.4.0, provides functionality for working with Zarr V2 datasets and experimental support for Zarr V3. Below is a summary of its current capabilities based on online resources and our discussions:

---

## **1. Supported Zarr Versions**

- **Zarr V2**:
    - Full read/write support for Zarr V2 datasets.
    - Handles `.zgroup`, `.zarray`, and `.zmetadata` files.
    - Compatible with both the **classic raster API** (2D arrays) and **multidimensional API** (n-dimensional arrays)[^3][^5].
- **Zarr V3**:
    - Experimental support for Zarr V3 as of GDAL 3.8.
    - Reads and writes `zarr.json` metadata format (introduced in V3).
    - Some features like sharding (ZEP0002) are not yet fully implemented[^3].

---

## **2. Metadata Handling**

- **Geospatial Metadata**:
    - Encodes spatial reference systems (CRS) via a custom `_CRS` attribute.
    - Limited support for CF conventions (e.g., `grid_mapping`, `coordinates`)[^2][^5].
- **Multidimensional Metadata**:
    - Supports dimension labels through `_ARRAY_DIMENSIONS` attributes, enabling compatibility with tools like `xarray`.

---

## **3. Compression Codecs**

- Supports multiple compression methods, including:
    - Blosc
    - LZ4
    - Zstd
- Compression is configurable via driver options during dataset creation[^3][^5].

---

## **4. Cloud-Native Functionality**

- Leverages GDAL's Virtual File System (VFS) for cloud storage access:
    - `/vsicurl/` for HTTP(S) access.
    - `/vsis3/`, `/vsigs/`, and `/vsiaz/` for AWS S3, Google Cloud Storage, and Azure Blob Storage, respectively[^6].
- Optimized for chunk-based I/O but lacks advanced features like adaptive prefetching or distributed locking.

---

## **5. APIs Supported**

- **Classic Raster API**:
    - Reads/writes 2D arrays as raster bands.
    - Splits higher-dimensional arrays into subdatasets.
- **Multidimensional API**:
    - Provides n-dimensional array access via `GDALMDArray`.
    - Includes methods like `GetCoordinateVariables()` and `Cache()` for improved multidimensional workflows[^3].

---

## **6. Limitations**

1. **Zarr V3 Features**:
    - Sharding (ZEP0002) is not yet fully supported.
    - Limited handling of evolving V3 metadata formats.
2. **CF Conventions**:
    - Partial compliance with CF metadata standards, which are critical for scientific workflows.
3. **Performance Bottlenecks**:
    - Inefficient chunk caching for large datasets hosted on cloud storage.
    - No support for advanced cloud-native optimizations like tile presence caching or parallel writes.
4. **Interoperability Issues**:
    - Metadata conflicts between GDAL’s custom `_CRS` attribute and CF conventions used by Python tools like `xarray`.
5. **Testing Gaps**:
    - Limited automated testing for edge cases (e.g., invalid metadata, mixed-version datasets).

---

## **Key Features Added in Recent Releases**

1. **GDAL 3.4.0**:
    - Initial implementation of the Zarr driver with support for Zarr V2 and experimental V3[^3][^5].
2. **GDAL 3.5.0**:
    - Added support for new data types (e.g., `GDT_Int64`, `GDT_UInt64`) in the Zarr driver[^4].
3. **GDAL ≥3.8**:
    - Improved handling of Zarr V3 datasets based on the May 2023 specification snapshot[^5].

---

## **Next Steps Based on Audit**

1. Implement full compliance with the latest Zarr V3 specification, including sharding and unified metadata (`zarr.json`).
2. Enhance geospatial metadata handling to align with CF conventions and emerging GeoZarr standards[^7].
3. Optimize cloud-native performance by adding features like adaptive chunk caching and distributed locking mechanisms.

This audit summary provides a clear overview of the current state of the GDAL Zarr driver and highlights areas for improvement in future development efforts!

<div style="text-align: center">⁂</div>

[^1]: https://gdal.org/en/stable/development/rfc/rfc46_gdal_ogr_unification.html

[^2]: https://docs.ogc.org/per/21-032.html

[^3]: https://abi-laboratory.pro/index.php?view=changelog\&l=gdal\&v=3.4.0

[^4]: https://www.osgeo.org/foundation-news/gdal-3-5-0-is-released/

[^5]: https://www.osgeo.org/foundation-news/gdal-3-4-0-is-released/

[^6]: https://gdal.org/en/latest/user/virtual_file_systems.html

[^7]: https://www.ogc.org/announcement/ogc-forms-new-geozarr-standards-working-group-to-establish-a-zarr-encoding-for-geospatial-data/

[^8]: https://fossies.org/linux/gdal/frmts/zarr/zarr_v2_array.cpp

