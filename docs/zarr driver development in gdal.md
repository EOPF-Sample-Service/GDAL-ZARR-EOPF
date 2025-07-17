# Zarr Driver Development in GDAL: Technical Evolution and Implementation Analysis

---

The integration of the Zarr data format into the Geospatial Data Abstraction Library (GDAL) represents a significant advancement in handling multidimensional geospatial datasets for cloud-native workflows. This report examines the technical evolution of GDAL's Zarr driver, analyzing its implementation details, capabilities, and integration with modern geospatial processing paradigms.

## Historical Context and Design Philosophy

The Zarr driver emerged as a response to the growing need for cloud-optimized formats capable of managing large-scale, chunked, and compressed N-dimensional arrays. Unlike traditional formats like NetCDF or HDF5, Zarr's design emphasizes simplicity, scalability, and interoperability with distributed storage systems[^1][^3]. GDAL's implementation leverages Zarr's inherent strengths while addressing geospatial-specific requirements through extensions like spatial reference system (SRS) encoding and multidimensional API support[^1].

Initial development focused on Zarr V2 compatibility, with GDAL 3.4 introducing basic read/write capabilities. Subsequent versions expanded functionality, with GDAL 3.8 adding support for the evolving Zarr V3 specification. This version alignment required careful handling of backward compatibility issues, particularly regarding metadata structure and chunking patterns[^1]. The driver's architecture separates low-level storage interactions from geospatial abstractions, enabling support for both local filesystems and cloud object storage through GDAL's Virtual File System (VFS) layer[^3].

## Multidimensional Data Model Integration

GDAL's multidimensional API (introduced in 3.1) provides the foundation for Zarr integration, enabling unified access to 3D/4D datasets through dimension-based slicing operations. The driver maps Zarr arrays to GDAL's `MDArray` concept, preserving dimension labels through the `_ARRAY_DIMENSIONS` attribute convention used by Xarray[^1][^2]. This enables direct translation between Zarr's hierarchical group structure and GDAL's multidimensional data model, maintaining semantic relationships between variables[^3].

A critical implementation challenge involved handling temporal dimensions in climate datasets (e.g., CMIP6). The driver resolves this through implicit dimension typing when encountering CF-compliant metadata, enabling time-aware subsetting operations without requiring explicit coordinate system declarations[^2]. Spatial dimensions benefit from GDAL's extended SRS encoding system, which stores CRS information in a `_CRS` attribute containing WKT, PROJJSON, or OGC CRS URLs[^1].

## Cloud-Native Optimization Features

The Zarr driver implements several cloud-specific optimizations:

1. **Chunk Presence Caching**: For remote datasets, the `CACHE_TILE_PRESENCE` option generates `.gmac` index files to accelerate spatial queries by pre-recording chunk availability[^1].
2. **Multithreaded Decoding**: Parallel tile decoding through `AdviseRead()` utilizes configurable thread pools and memory budgets (via `GDAL_CACHEMAX`), critical for processing large climate model outputs[^1][^6].
3. **Consolidated Metadata Handling**: Automatic use of `.zmetadata` in Zarr V2 minimizes remote storage requests, while the `USE_ZMETADATA` option allows fallback to per-array metadata for compatibility[^1][^5].

These features enable efficient access to petabyte-scale datasets stored in cloud object stores like AWS S3 or Google Cloud Storage, as demonstrated in OGC Testbed 17 evaluations[^3]. The driver's performance characteristics show particular advantages for strided access patterns common in web mapping applications, where spatial subsampling at multiple zoom levels benefits from Zarr's chunk-level compression[^2].

## Spatial Reference System Encoding

GDAL extends Zarr's metadata capabilities through a custom `_CRS` attribute that combines multiple CRS representation formats:

```json
"_CRS": {
  "wkt": "PROJCRS[...]",
  "projjson": { ... },
  "url": "http://www.opengis.net/def/crs/EPSG/0/26711"
}
```

This hybrid approach ensures compatibility with diverse geospatial toolchains. On write operations, GDAL prioritizes WKT:2019 for accuracy, while read operations check `url` first for OGC CRS service compatibility[^1]. The implementation automatically handles datum transformations between WKT versions, crucial for maintaining precision in historical datasets using NAD27 or other legacy datums[^1].

## API Compatibility Layers

The driver supports dual access modes:

### Classic Raster API

For 2D slices, the driver exposes Zarr arrays as traditional GDALDatasets. Multidimensional arrays generate subdatasets for each 2D slice (e.g., time steps in climate data). This enables backward compatibility with legacy GIS software but requires careful handling of dimension ordering in chunk storage[^1][^2].

### Multidimensional API

Full n-dimensional access uses GDAL's `GDALMDArray` interface, supporting:

- Dimension subsetting via `GetView()`
- Attribute-based variable selection
- Type-preserving chunk compression/decompression

Benchmarks show the multidimensional API reduces data transfer volumes by 40-60% compared to classic API usage for 4D climate datasets[^3].

## Compression and Performance

The driver's compression support varies by build configuration:

```plaintext
COMPRESSORS=blosc,zlib,gzip,lzma,zstd,lz4  
BLOSC_COMPRESSORS=blosclz,lz4,lz4hc,snappy,zlib,zstd  
```

Runtime registration of custom compressors via `CPLCompressor`/`CPLDecompressor` allows integration with domain-specific algorithms. Performance analysis reveals Zstd with level 3 compression provides optimal balance between ratio and speed for Sentinel-2 L2A datasets, achieving 4:1 compression with subsecond tile access times[^3].

Multithreaded decoding scales linearly up to 16 cores for LZ4-compressed chunks, though diminishing returns occur with higher thread counts due to Python's Global Interpreter Lock in CPython-based workflows[^5][^6].

## Challenges and Limitations

Current implementation constraints include:

1. **V3 Specification Volatility**: GDAL 3.8's Zarr V3 support reflects the 2023-05-07 spec snapshot, causing incompatibility with earlier V3 experiments[^1].
2. **Metadata Discovery**: Consolidated metadata (.zmetadata) doesn't fully map to GDAL's metadata model, leading to empty `GetMetadata()` calls in some Python bindings[^5].
3. **Cloud Listing Overhead**: Initial `CACHE_TILE_PRESENCE` runs on S3-like stores can exceed 30 minutes for datasets with >1 million chunks[^1].

The OGC Testbed 17 evaluation identified additional challenges in coordinate system alignment between Zarr and STAC metadata, requiring custom extensions for full CRS provenance tracking[^3].

## Future Development Directions

1. **V3 Feature Completion**: Implementing full V3 sharding support and enhanced attr storage.
2. **Proactive Chunk Caching**: Machine learning-driven prefetching based on access patterns.
3. **Distributed Locking**: Cloud-native write locking using DynamoDB or similar services.
4. **STAC Integration**: Tight coupling with SpatioTemporal Asset Catalogs for metadata discovery.

These enhancements aim to position GDAL as the reference implementation for cloud-based geospatial Zarr processing, complementing ongoing OGC standardization efforts[^3].

## Conclusion

GDAL's Zarr driver development demonstrates how open-source geospatial tools adapt to cloud computing paradigms. By bridging Zarr's generic array model with geospatial metadata requirements, the implementation enables new workflows in climate modeling, SAR processing, and planetary-scale analytics. Continued evolution will require balancing specification agility with stability guarantees—a challenge mitigated through GDAL's pluggable architecture and active community engagement.



<div style="text-align: center">⁂</div>

[1,](https://gdal.org/en/stable/drivers/raster/zarr.html)
[2,](https://r-spatial.org/r/2022/09/13/zarr.html)
[3,](https://docs.ogc.org/per/21-032.pdf)
[4,](https://gdal.org/en/stable/drivers/raster/rraster.html)
[5,](https://gis.stackexchange.com/questions/480788/gdal-zarr-dataset-getmetadata-python-always-returns-empty)
[6,](https://gdal.org/en/stable/user/configoptions.html)
[7,](https://github.com/OSGeo/gdal)
[8](https://github.com/zarr-developers/zarr_implementations)

[[^9]: GDAL Pull Requests](https://github.com/OSGeo/gdal/pulls)

[[^10]: GDAL Issues](https://github.com/OSGeo/gdal/issues)

[[^11]: GDAL Community](https://gdal.org/en/stable/community/index.html)

[[^12]: Zarr Arrays](https://zarr.readthedocs.io/en/v3.0.0/user-guide/arrays.html)

[[^13]: Xarray IO](https://docs.xarray.dev/en/stable/user-guide/io.html)

[[^14]: OGC Testbed 17](https://portal.ogc.org/files/100727)

[[^15]: GDAL Issue 6436](https://github.com/OSGeo/gdal/issues/6436)

[[^16]: GDAL Raster Drivers](https://gdal.org/en/stable/drivers/raster/index.html)

[[^17]: Zarr Driver Test](https://github.com/OSGeo/gdal/blob/master/autotest/gdrivers/zarr_driver.py)

[[^18]: GDAL 3.5.0 Release](https://www.osgeo.org/foundation-news/gdal-3-5-0-is-released/)

[[^19]: Building GDAL from Source](https://gdal.org/en/stable/development/building_from_source.html)

[[^20]: Xarray Issue 6448](https://github.com/pydata/xarray/issues/6448)

[[^21]: GDAL](https://gdal.org)

[[^22]: Zarr Implementations](https://github.com/zarr-developers/zarr_implementations/blob/main/README.md)

[[^23]: Stars Issue 663](https://github.com/r-spatial/stars/issues/663)

[[^24]: QGIS Issue 54240](https://github.com/qgis/QGIS/issues/54240)

[[^25]: Zarr Spec Issue 33]( https://github.com/zarr-developers/geozarr-spec/issues/33)

[[^26]: GDAL MDArray API](https://gdal.org/en/stable/api/gdalmdarray_cpp.html)

[[^27]: GDAL Translate Object](https://gis.stackexchange.com/questions/316034/can-gdal-translate-return-an-object-instead-of-writing-a-file)

[[^28]: Even Rouault Tweet](https://twitter.com/EvenRouault/status/1365337244492562433)

[[^29]: GDAL Raster C API](https://gdal.org/en/stable/api/raster_c_api.html)

[[^31]: GDAL Issue 8212]( https://github.com/OSGeo/gdal/issues/8212/linked_closing_reference)

[[^32]: Planetary Computer Issue 366](https://github.com/microsoft/PlanetaryComputer/issues/366)

[[^33]: Xarray Issue 6374](https://github.com/pydata/xarray/issues/6374)

[[^34]: Zarr Community Issue 37](https://github.com/zarr-developers/community/issues/37)

[[^35]: GDAL News](https://github.com/OSGeo/gdal/blob/master/NEWS.md)

[[^36]: VCPKG Issue 36990](https://github.com/microsoft/vcpkg/discussions/36990)

[[^37]: GDAL Dev Mailing List](https://lists.osgeo.org/listinfo/gdal-dev)

[[^38]: GDAL PDF](https://gdal.org/gdal.pdf)

[[^39]: BioRxiv Paper](https://www.biorxiv.org/content/10.1101/2024.06.11.598241v1.full-text)

[[^40]: QGIS Data Source](https://docs.qgis.org/latest/en/docs/user_manual/managing_data_source/opening_data.html)

[[^41]: GDAL CMake Builds](https://github.com/OSGeo/gdal/actions/workflows/cmake_builds.yml)

[[^42]: GDAL Android Builds]( https://github.com/OSGeo/gdal/actions/workflows/android_cmake.yml)

[[^43]: PMC Paper](https://pmc.ncbi.nlm.nih.gov/articles/PMC11195102/)

[[^44]: Stack Overflow Question](https://stackoverflow.com/questions/69228924/how-to-convert-zarr-data-to-geotiff)
