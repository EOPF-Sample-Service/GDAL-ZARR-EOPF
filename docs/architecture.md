# Architecture Documentation

## Overview

The EOPF-Zarr GDAL plugin follows GDAL's driver architecture pattern, implementing custom dataset and raster band classes that handle EOPF data stored in Zarr format.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    GDAL Applications                        │
│  (QGIS, gdalinfo, gdal_translate, Python scripts, etc.)   │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                    GDAL Core                                │
│  (Dataset Management, I/O, Coordinate Systems, etc.)       │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                 EOPF-Zarr Plugin                           │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │  EOPFDriver     │  │  EOPFDataset    │  │EOPFRasterBand│ │
│  │  (Registration) │  │  (Dataset Mgmt) │  │(Band Access)│ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                Zarr Format Layer                           │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │  Metadata       │  │  Chunk Reader   │  │ Compression │ │
│  │  Parser         │  │                 │  │ Support     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│                File System / Cloud Storage                  │
│         (Local files, S3, HTTP, etc.)                      │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. EOPFDriver

**Purpose:** Driver registration and dataset identification

**Key Responsibilities:**
- Register the driver with GDAL
- Identify EOPF/Zarr datasets
- Create `EOPFDataset` instances

**Files:** `src/EOPFDriver.cpp`

```cpp
class EOPFDriver : public GDALDriver {
public:
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);
    static void Register();
};
```

### 2. EOPFDataset

**Purpose:** Dataset-level operations and management

**Key Responsibilities:**
- Parse Zarr metadata (`.zarray`, `.zgroup`, `.zattrs`)
- Manage coordinate reference systems
- Handle subdatasets (multiple variables/dimensions)
- Create and manage raster bands
- Implement GDAL dataset interface

**Files:** `src/EOPFDataset.cpp`, `include/EOPFDataset.h`

```cpp
class EOPFDataset : public GDALDataset {
private:
    std::vector<std::unique_ptr<EOPFRasterBand>> m_bands;
    std::string m_zarrPath;
    json m_metadata;
    
public:
    static GDALDataset* Open(const char* pszFilename);
    CPLErr GetGeoTransform(double* padfTransform) override;
    const char* GetProjectionRef() override;
    char** GetMetadata(const char* pszDomain) override;
};
```

### 3. EOPFRasterBand

**Purpose:** Individual band/array access and I/O

**Key Responsibilities:**
- Read raster data from Zarr chunks
- Handle data type conversions
- Implement windowed reading
- Manage band-specific metadata
- Support various compression formats

**Files:** `src/EOPFRasterBand.cpp`, `include/EOPFRasterBand.h`

```cpp
class EOPFRasterBand : public GDALRasterBand {
private:
    std::string m_arrayPath;
    json m_arrayMetadata;
    std::vector<size_t> m_chunkShape;
    
public:
    CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
    CPLErr IRasterIO(GDALRWFlag eRWFlag, int nXOff, int nYOff, 
                     int nXSize, int nYSize, void* pData, 
                     int nBufXSize, int nBufYSize, 
                     GDALDataType eBufType, 
                     GSpacing nPixelSpace, GSpacing nLineSpace,
                     GDALRasterIOExtraArg* psExtraArg) override;
};
```

## Data Flow

### Opening a Dataset

1. **Identification Phase**
   ```
   GDAL Core → EOPFDriver::Identify() → Check for Zarr structure
   ```

2. **Opening Phase**
   ```
   GDAL Core → EOPFDriver::Open() → EOPFDataset::Open()
   ```

3. **Initialization**
   ```
   EOPFDataset::Open() → Parse metadata → Create bands → Return dataset
   ```

### Reading Data

1. **Request Processing**
   ```
   Application → GDAL Core → EOPFDataset → EOPFRasterBand
   ```

2. **Chunk-based Reading**
   ```
   EOPFRasterBand → Identify chunks → Read chunks → Decompress → Assemble
   ```

3. **Data Return**
   ```
   Assembled data → Type conversion → Return to application
   ```

## File Structure

### Zarr Directory Structure
```
dataset.zarr/
├── .zgroup              # Group metadata
├── .zattrs              # Group attributes
├── variable1/
│   ├── .zarray          # Array metadata
│   ├── .zattrs          # Array attributes
│   ├── 0.0              # Chunk files
│   ├── 0.1
│   └── ...
└── variable2/
    ├── .zarray
    ├── .zattrs
    └── chunks...
```

### Plugin Source Structure
```
src/
├── EOPFDriver.cpp       # Driver registration
├── EOPFDataset.cpp      # Dataset implementation
├── EOPFRasterBand.cpp   # Band implementation
├── eopf_metadata.cpp    # Metadata utilities
└── eopfzarr_*.cpp       # Additional utilities

include/
├── EOPFDataset.h        # Dataset header
├── EOPFRasterBand.h     # Band header
├── eopf_metadata.h      # Metadata utilities
└── eopf_zarr.h          # Common definitions
```

## Memory Management

### Object Lifecycle
1. **Dataset Creation:** `EOPFDataset` owns `EOPFRasterBand` objects
2. **Reference Counting:** GDAL handles dataset reference counting
3. **Cleanup:** Automatic cleanup when dataset is closed

### Memory Optimization
- **Lazy Loading:** Metadata and chunks loaded on demand
- **Caching:** GDAL's built-in caching for frequently accessed chunks
- **Streaming:** Large datasets processed in blocks

## Threading and Concurrency

### Thread Safety
- **Read Operations:** Thread-safe for concurrent reads
- **Metadata Access:** Protected by internal locking
- **GDAL Integration:** Follows GDAL's threading model

### Performance Considerations
- **Chunk Alignment:** Optimize reads to align with Zarr chunks
- **Parallel Access:** Multiple bands can be read concurrently
- **Cache Efficiency:** Leverage GDAL's cache for repeated access

## Error Handling

### Error Categories
1. **File System Errors:** Missing files, permission issues
2. **Format Errors:** Invalid Zarr metadata, corrupted chunks
3. **Memory Errors:** Allocation failures, out of memory
4. **Network Errors:** Cloud access failures, timeouts

### Error Propagation
```
Low-level error → CPLError() → GDAL error handler → Application
```

### Recovery Strategies
- **Graceful Degradation:** Partial data access when possible
- **Retry Logic:** For transient network errors
- **Fallback Modes:** Alternative data access paths

## Extension Points

### Adding New Formats
1. **Metadata Parsers:** Support for additional metadata schemas
2. **Compression:** New compression algorithm support
3. **Coordinate Systems:** Enhanced CRS detection and handling

### Performance Optimizations
1. **Caching Strategies:** Custom caching for specific use cases
2. **Parallel Processing:** Multi-threaded chunk processing
3. **Memory Management:** Custom allocators for large datasets

## Dependencies

### Required Dependencies
- **GDAL 3.4+:** Core functionality
- **JSON Library:** Metadata parsing (nlohmann/json or similar)
- **Compression Libraries:** gzip, lz4, blosc (optional)

### Optional Dependencies
- **Cloud SDKs:** For enhanced cloud storage support
- **HDF5:** For NetCDF compatibility
- **Threading Libraries:** For parallel processing

## Testing Architecture

### Unit Tests
- **Component Testing:** Individual class testing
- **Mock Objects:** Simulated file systems and data
- **Edge Cases:** Error conditions and boundary values

### Integration Tests
- **End-to-End:** Full workflow testing
- **Real Data:** Testing with actual EOPF datasets
- **Performance:** Benchmarking and profiling

### Continuous Integration
- **Automated Testing:** GitHub Actions workflows
- **Multiple Platforms:** Linux, macOS, Windows
- **GDAL Versions:** Test against multiple GDAL versions

## Future Architecture Considerations

### Scalability
- **Distributed Processing:** Support for distributed computing
- **Streaming:** Large dataset streaming capabilities
- **Cloud-Native:** Optimizations for cloud-native architectures

### Extensibility  
- **Plugin System:** Support for custom extensions
- **Configuration:** Runtime configuration options
- **APIs:** Stable APIs for external integration
