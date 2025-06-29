# API Documentation

## Overview
This document describes the public API and interfaces provided by the EOPF-Zarr GDAL plugin.

## Classes and Interfaces

### EOPFDataset
The main dataset class that implements GDAL's `GDALDataset` interface.

#### Methods

##### `Open(const char* pszFilename, GDALAccess eAccess)`
Opens an EOPF dataset from the specified file path.

**Parameters:**
- `pszFilename`: Path to the EOPF dataset file
- `eAccess`: Access mode (GA_ReadOnly, GA_Update)

**Returns:** Pointer to EOPFDataset instance or nullptr on failure

**Example:**
```cpp
GDALDataset* dataset = GDALOpen("path/to/eopf_dataset.zarr", GA_ReadOnly);
```

##### `GetRasterBand(int nBand)`
Returns the specified raster band.

**Parameters:**
- `nBand`: Band number (1-based)

**Returns:** Pointer to EOPFRasterBand instance

### EOPFRasterBand
Represents a single raster band within an EOPF dataset.

#### Methods

##### `RasterIO(GDALRWFlag eRWFlag, int nXOff, int nYOff, int nXSize, int nYSize, void* pData, int nBufXSize, int nBufYSize, GDALDataType eBufType, GSpacing nPixelSpace, GSpacing nLineSpace, GDALRasterIOExtraArg* psExtraArg)`
Performs raster I/O operations.

**Parameters:**
- `eRWFlag`: Read/write flag (GF_Read, GF_Write)
- `nXOff`, `nYOff`: Pixel offset in dataset coordinates
- `nXSize`, `nYSize`: Size of region to read/write
- `pData`: Buffer for data
- `nBufXSize`, `nBufYSize`: Size of buffer
- `eBufType`: Data type of buffer
- `nPixelSpace`, `nLineSpace`: Spacing between pixels/lines
- `psExtraArg`: Additional arguments

**Returns:** CE_None on success, CE_Failure on error

## Driver Registration

The EOPF driver is automatically registered when the plugin is loaded. It identifies files with the following characteristics:

- Extension: `.zarr`, `.eopf`
- Contains Zarr metadata (`.zarray`, `.zgroup` files)
- EOPF-specific metadata attributes

## Error Handling

The plugin uses GDAL's standard error reporting mechanism:

- `CPLError()` for reporting errors
- `CE_None`, `CE_Failure`, `CE_Warning` return codes
- Error messages are logged to GDAL's error handler

## Thread Safety

The plugin implements thread-safe operations for:
- Dataset opening and closing
- Metadata reading
- Raster data access (read-only)

Write operations are not thread-safe and should be serialized by the caller.

## Memory Management

- All objects follow GDAL's reference counting conventions
- Use `GDALClose()` to properly release datasets
- Raster bands are owned by their parent dataset
