#ifndef EOPF_ZARR_H_INCLUDED
#define EOPF_ZARR_H_INCLUDED

// Ensure proper DLL export/import on Windows
#ifdef _WIN32
#ifdef EOPF_ZARR_BUILDING
#define EOPF_ZARR_API __declspec(dllexport)
#else
#define EOPF_ZARR_API __declspec(dllimport)
#endif
#else
#define EOPF_ZARR_API
#endif

// Standard GDAL driver entry point
extern "C" EOPF_ZARR_API void GDALRegister_EOPFZarr();

#endif /* EOPF_ZARR_H_INCLUDED */
