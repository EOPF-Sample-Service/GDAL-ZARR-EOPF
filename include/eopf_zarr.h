#ifndef EOPF_ZARR_H_INCLUDED
#define EOPF_ZARR_H_INCLUDED

// Ensure proper DLL export/import on Windows
#ifdef _WIN32
#ifdef EOPF_ZARR_BUILDING
#define EOPFZARR_DLL __declspec(dllexport)
#else
#define EOPFZARR_DLL __declspec(dllimport)
#endif
#else
#define EOPFZARR_DLL
#endif

// For backward compatibility
#define EOPF_ZARR_API EOPFZARR_DLL

// Standard GDAL driver entry point
extern "C" EOPFZARR_DLL void GDALRegister_EOPFZarr();

#endif /* EOPF_ZARR_H_INCLUDED */
