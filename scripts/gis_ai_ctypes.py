"""
GIS AI Library - Python ctypes binding
Provides Python access to the gis_ai C API via ctypes.
"""

import ctypes
import os
import platform
from typing import Optional


class GisAiRaster(ctypes.Structure):
    pass


class GisAiVector(ctypes.Structure):
    pass


class GisAiPointCloud(ctypes.Structure):
    pass


class GisAiModel(ctypes.Structure):
    pass


class GisAiRasterSeg(ctypes.Structure):
    pass


GisAiRasterPtr = ctypes.POINTER(GisAiRaster)
GisAiVectorPtr = ctypes.POINTER(GisAiVector)
GisAiPointCloudPtr = ctypes.POINTER(GisAiPointCloud)
GisAiModelPtr = ctypes.POINTER(GisAiModel)
GisAiRasterSegPtr = ctypes.POINTER(GisAiRasterSeg)


class GisAiLibrary:
    _loaded_libraries = {}

    def __init__(self, lib_path: Optional[str] = None):
        self._configure_proj_environment()
        if lib_path is None:
            lib_path = self._find_library()
        lib_path = os.path.abspath(lib_path)
        self._lib = self._loaded_libraries.get(lib_path)
        if self._lib is None:
            self._lib = ctypes.CDLL(lib_path)
            self._loaded_libraries[lib_path] = self._lib
        self._setup_signatures()

    def _configure_proj_environment(self):
        if os.environ.get("PROJ_LIB"):
            return

        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        proj_candidates = [
            os.path.join(base_dir, "build", "release", "vcpkg_installed", "x64-windows", "share", "proj"),
            os.path.join(base_dir, "build", "dev-windows", "vcpkg_installed", "x64-windows", "share", "proj"),
        ]

        vcpkg_root = os.environ.get("VCPKG_ROOT")
        if vcpkg_root:
            proj_candidates.append(os.path.join(vcpkg_root, "installed", "x64-windows", "share", "proj"))

        for proj_dir in proj_candidates:
            if os.path.exists(os.path.join(proj_dir, "proj.db")):
                os.environ["PROJ_LIB"] = proj_dir
                return

    def _find_library(self) -> str:
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        build_dir = os.path.join(base_dir, "build")

        search_paths = []
        if platform.system() == "Windows":
            for preset in ["dev-windows", "release-windows", "release"]:
                for config in ["Release", "Debug"]:
                    search_paths.append(os.path.join(build_dir, preset, "bin", config, "gis_ai.dll"))
                search_paths.append(os.path.join(build_dir, preset, "bin", "gis_ai.dll"))
            for config in ["Release", "Debug"]:
                search_paths.append(os.path.join(build_dir, "bin", config, "gis_ai.dll"))
        elif platform.system() == "Linux":
            for preset in ["dev-linux", "release-linux"]:
                search_paths.append(os.path.join(build_dir, preset, "lib", "libgis_ai.so"))
            search_paths.append(os.path.join(build_dir, "lib", "libgis_ai.so"))
        elif platform.system() == "Darwin":
            for preset in ["dev-macos", "release-macos"]:
                search_paths.append(os.path.join(build_dir, preset, "lib", "libgis_ai.dylib"))
            search_paths.append(os.path.join(build_dir, "lib", "libgis_ai.dylib"))

        for path in search_paths:
            if os.path.exists(path):
                return path

        raise FileNotFoundError(
            f"Could not find gis_ai library. Searched paths:\n"
            + "\n".join(f"  {p}" for p in search_paths)
            + "\nPlease provide the library path explicitly."
        )

    def _setup_signatures(self):
        lib = self._lib

        lib.GisAi_Init.argtypes = [ctypes.c_char_p]
        lib.GisAi_Init.restype = ctypes.c_int

        lib.GisAi_Shutdown.argtypes = []
        lib.GisAi_Shutdown.restype = None

        lib.GisAi_GetLastError.argtypes = []
        lib.GisAi_GetLastError.restype = ctypes.c_char_p

        lib.GisAi_GetLastErrorCode.argtypes = []
        lib.GisAi_GetLastErrorCode.restype = ctypes.c_int

        lib.GisAi_GetVersionMajor.argtypes = []
        lib.GisAi_GetVersionMajor.restype = ctypes.c_int
        lib.GisAi_GetVersionMinor.argtypes = []
        lib.GisAi_GetVersionMinor.restype = ctypes.c_int
        lib.GisAi_GetVersionPatch.argtypes = []
        lib.GisAi_GetVersionPatch.restype = ctypes.c_int
        lib.GisAi_GetVersionString.argtypes = []
        lib.GisAi_GetVersionString.restype = ctypes.c_char_p

        lib.GisAi_Raster_Load.argtypes = [ctypes.c_char_p]
        lib.GisAi_Raster_Load.restype = GisAiRasterPtr

        lib.GisAi_Raster_Save.argtypes = [GisAiRasterPtr, ctypes.c_char_p]
        lib.GisAi_Raster_Save.restype = ctypes.c_int

        lib.GisAi_Raster_Destroy.argtypes = [GisAiRasterPtr]
        lib.GisAi_Raster_Destroy.restype = None

        lib.GisAi_Raster_GetWidth.argtypes = [GisAiRasterPtr]
        lib.GisAi_Raster_GetWidth.restype = ctypes.c_int

        lib.GisAi_Raster_GetHeight.argtypes = [GisAiRasterPtr]
        lib.GisAi_Raster_GetHeight.restype = ctypes.c_int

        lib.GisAi_Raster_GetBandCount.argtypes = [GisAiRasterPtr]
        lib.GisAi_Raster_GetBandCount.restype = ctypes.c_int

        lib.GisAi_Raster_GetGeoTransform.argtypes = [GisAiRasterPtr, ctypes.POINTER(ctypes.c_double)]
        lib.GisAi_Raster_GetGeoTransform.restype = ctypes.c_int

        lib.GisAi_Raster_GetProjection.argtypes = [GisAiRasterPtr]
        lib.GisAi_Raster_GetProjection.restype = ctypes.c_char_p

        lib.GisAi_Raster_GetBandData.argtypes = [GisAiRasterPtr, ctypes.c_int, ctypes.POINTER(ctypes.c_float), ctypes.c_int]
        lib.GisAi_Raster_GetBandData.restype = ctypes.c_int

        lib.GisAi_Vector_Load.argtypes = [ctypes.c_char_p]
        lib.GisAi_Vector_Load.restype = GisAiVectorPtr

        lib.GisAi_Vector_Save.argtypes = [GisAiVectorPtr, ctypes.c_char_p]
        lib.GisAi_Vector_Save.restype = ctypes.c_int

        lib.GisAi_Vector_Destroy.argtypes = [GisAiVectorPtr]
        lib.GisAi_Vector_Destroy.restype = None

        lib.GisAi_Vector_GetFeatureCount.argtypes = [GisAiVectorPtr]
        lib.GisAi_Vector_GetFeatureCount.restype = ctypes.c_int

        lib.GisAi_Vector_GetProjection.argtypes = [GisAiVectorPtr]
        lib.GisAi_Vector_GetProjection.restype = ctypes.c_char_p

        lib.GisAi_PointCloud_Load.argtypes = [ctypes.c_char_p]
        lib.GisAi_PointCloud_Load.restype = GisAiPointCloudPtr

        lib.GisAi_PointCloud_Save.argtypes = [GisAiPointCloudPtr, ctypes.c_char_p]
        lib.GisAi_PointCloud_Save.restype = ctypes.c_int

        lib.GisAi_PointCloud_Destroy.argtypes = [GisAiPointCloudPtr]
        lib.GisAi_PointCloud_Destroy.restype = None

        lib.GisAi_PointCloud_GetPointCount.argtypes = [GisAiPointCloudPtr]
        lib.GisAi_PointCloud_GetPointCount.restype = ctypes.c_int

        lib.GisAi_Vector_Buffer.argtypes = [GisAiVectorPtr, ctypes.c_double]
        lib.GisAi_Vector_Buffer.restype = GisAiVectorPtr

        lib.GisAi_Vector_Intersect.argtypes = [GisAiVectorPtr, GisAiVectorPtr]
        lib.GisAi_Vector_Intersect.restype = GisAiVectorPtr

        lib.GisAi_Vector_Clip.argtypes = [GisAiVectorPtr, GisAiVectorPtr]
        lib.GisAi_Vector_Clip.restype = GisAiVectorPtr

        lib.GisAi_Vector_Simplify.argtypes = [GisAiVectorPtr, ctypes.c_double]
        lib.GisAi_Vector_Simplify.restype = GisAiVectorPtr

        lib.GisAi_Raster_Resample.argtypes = [GisAiRasterPtr, ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
        lib.GisAi_Raster_Resample.restype = GisAiRasterPtr

        lib.GisAi_Raster_Normalize.argtypes = [GisAiRasterPtr]
        lib.GisAi_Raster_Normalize.restype = GisAiRasterPtr

        lib.GisAi_Raster_Threshold.argtypes = [GisAiRasterPtr, ctypes.c_double]
        lib.GisAi_Raster_Threshold.restype = GisAiRasterPtr

        lib.GisAi_Model_Load.argtypes = [ctypes.c_char_p]
        lib.GisAi_Model_Load.restype = GisAiModelPtr

        lib.GisAi_Model_Unload.argtypes = [GisAiModelPtr]
        lib.GisAi_Model_Unload.restype = None

        lib.GisAi_AI_Infer.argtypes = [GisAiModelPtr, GisAiRasterPtr]
        lib.GisAi_AI_Infer.restype = GisAiRasterPtr

        lib.GisAi_RasterSeg_Create.argtypes = [ctypes.c_char_p]
        lib.GisAi_RasterSeg_Create.restype = GisAiRasterSegPtr

        lib.GisAi_RasterSeg_Run.argtypes = [GisAiRasterSegPtr, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
        lib.GisAi_RasterSeg_Run.restype = ctypes.c_int

        lib.GisAi_RasterSeg_Destroy.argtypes = [GisAiRasterSegPtr]
        lib.GisAi_RasterSeg_Destroy.restype = None

        lib.GisAi_TransformCoordinates.argtypes = [ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double), ctypes.c_char_p, ctypes.c_char_p]
        lib.GisAi_TransformCoordinates.restype = ctypes.c_int

    def init(self, config_path: Optional[str] = None) -> int:
        c_path = config_path.encode("utf-8") if config_path else None
        return self._lib.GisAi_Init(c_path)

    def shutdown(self):
        self._lib.GisAi_Shutdown()

    def get_last_error(self) -> str:
        err = self._lib.GisAi_GetLastError()
        return err.decode("utf-8") if err else ""

    def get_last_error_code(self) -> int:
        return self._lib.GisAi_GetLastErrorCode()

    def get_version(self) -> tuple:
        major = self._lib.GisAi_GetVersionMajor()
        minor = self._lib.GisAi_GetVersionMinor()
        patch = self._lib.GisAi_GetVersionPatch()
        return (major, minor, patch)

    def get_version_string(self) -> str:
        v = self._lib.GisAi_GetVersionString()
        return v.decode("utf-8") if v else ""

    def raster_load(self, path: str) -> GisAiRasterPtr:
        return self._lib.GisAi_Raster_Load(path.encode("utf-8"))

    def raster_save(self, raster: GisAiRasterPtr, path: str) -> int:
        return self._lib.GisAi_Raster_Save(raster, path.encode("utf-8"))

    def raster_destroy(self, raster: GisAiRasterPtr):
        self._lib.GisAi_Raster_Destroy(raster)

    def raster_get_width(self, raster: GisAiRasterPtr) -> int:
        return self._lib.GisAi_Raster_GetWidth(raster)

    def raster_get_height(self, raster: GisAiRasterPtr) -> int:
        return self._lib.GisAi_Raster_GetHeight(raster)

    def raster_get_band_count(self, raster: GisAiRasterPtr) -> int:
        return self._lib.GisAi_Raster_GetBandCount(raster)

    def raster_get_geotransform(self, raster: GisAiRasterPtr) -> list:
        gt = (ctypes.c_double * 6)()
        ret = self._lib.GisAi_Raster_GetGeoTransform(raster, gt)
        if ret != 0:
            return []
        return [gt[i] for i in range(6)]

    def raster_get_projection(self, raster: GisAiRasterPtr) -> str:
        proj = self._lib.GisAi_Raster_GetProjection(raster)
        return proj.decode("utf-8") if proj else ""

    def raster_get_band_data(self, raster: GisAiRasterPtr, band_index: int) -> list:
        w = self.raster_get_width(raster)
        h = self.raster_get_height(raster)
        size = w * h
        buf = (ctypes.c_float * size)()
        copied = self._lib.GisAi_Raster_GetBandData(raster, band_index, buf, size)
        if copied <= 0:
            return []
        return [buf[i] for i in range(copied)]

    def vector_load(self, path: str) -> GisAiVectorPtr:
        return self._lib.GisAi_Vector_Load(path.encode("utf-8"))

    def vector_save(self, vector: GisAiVectorPtr, path: str) -> int:
        return self._lib.GisAi_Vector_Save(vector, path.encode("utf-8"))

    def vector_destroy(self, vector: GisAiVectorPtr):
        self._lib.GisAi_Vector_Destroy(vector)

    def vector_get_feature_count(self, vector: GisAiVectorPtr) -> int:
        return self._lib.GisAi_Vector_GetFeatureCount(vector)

    def vector_get_projection(self, vector: GisAiVectorPtr) -> str:
        proj = self._lib.GisAi_Vector_GetProjection(vector)
        return proj.decode("utf-8") if proj else ""

    def pointcloud_load(self, path: str) -> GisAiPointCloudPtr:
        return self._lib.GisAi_PointCloud_Load(path.encode("utf-8"))

    def pointcloud_save(self, pc: GisAiPointCloudPtr, path: str) -> int:
        return self._lib.GisAi_PointCloud_Save(pc, path.encode("utf-8"))

    def pointcloud_destroy(self, pc: GisAiPointCloudPtr):
        self._lib.GisAi_PointCloud_Destroy(pc)

    def pointcloud_get_point_count(self, pc: GisAiPointCloudPtr) -> int:
        return self._lib.GisAi_PointCloud_GetPointCount(pc)

    def vector_buffer(self, vector: GisAiVectorPtr, distance: float) -> GisAiVectorPtr:
        return self._lib.GisAi_Vector_Buffer(vector, distance)

    def vector_intersect(self, a: GisAiVectorPtr, b: GisAiVectorPtr) -> GisAiVectorPtr:
        return self._lib.GisAi_Vector_Intersect(a, b)

    def vector_clip(self, target: GisAiVectorPtr, clipper: GisAiVectorPtr) -> GisAiVectorPtr:
        return self._lib.GisAi_Vector_Clip(target, clipper)

    def vector_simplify(self, vector: GisAiVectorPtr, tolerance: float) -> GisAiVectorPtr:
        return self._lib.GisAi_Vector_Simplify(vector, tolerance)

    def raster_resample(self, raster: GisAiRasterPtr, new_w: int, new_h: int, method: str = "nearest") -> GisAiRasterPtr:
        return self._lib.GisAi_Raster_Resample(raster, new_w, new_h, method.encode("utf-8"))

    def raster_normalize(self, raster: GisAiRasterPtr) -> GisAiRasterPtr:
        return self._lib.GisAi_Raster_Normalize(raster)

    def raster_threshold(self, raster: GisAiRasterPtr, threshold: float) -> GisAiRasterPtr:
        return self._lib.GisAi_Raster_Threshold(raster, threshold)

    def model_load(self, model_path: str) -> GisAiModelPtr:
        return self._lib.GisAi_Model_Load(model_path.encode("utf-8"))

    def model_unload(self, model: GisAiModelPtr):
        self._lib.GisAi_Model_Unload(model)

    def ai_infer(self, model: GisAiModelPtr, raster: GisAiRasterPtr) -> GisAiRasterPtr:
        return self._lib.GisAi_AI_Infer(model, raster)

    def raster_seg_create(self, model_path: str) -> GisAiRasterSegPtr:
        return self._lib.GisAi_RasterSeg_Create(model_path.encode("utf-8"))

    def raster_seg_run(self, seg: GisAiRasterSegPtr, input_tif: str, output_tif: str, output_shp: str = None) -> int:
        c_shp = output_shp.encode("utf-8") if output_shp else None
        return self._lib.GisAi_RasterSeg_Run(seg, input_tif.encode("utf-8"), output_tif.encode("utf-8"), c_shp)

    def raster_seg_destroy(self, seg: GisAiRasterSegPtr):
        self._lib.GisAi_RasterSeg_Destroy(seg)

    def transform_coordinates(self, x: float, y: float, from_crs: str, to_crs: str):
        cx = ctypes.c_double(x)
        cy = ctypes.c_double(y)
        ret = self._lib.GisAi_TransformCoordinates(
            ctypes.byref(cx), ctypes.byref(cy),
            from_crs.encode("utf-8"), to_crs.encode("utf-8")
        )
        if ret != 0:
            raise RuntimeError(f"Transform failed: {self.get_last_error()}")
        return cx.value, cy.value
