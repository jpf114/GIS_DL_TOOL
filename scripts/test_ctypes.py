"""
GIS AI Library - ctypes binding verification tests
Validates that the C API works correctly through Python ctypes.
"""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gis_ai_ctypes import GisAiLibrary, GisAiRasterPtr, GisAiVectorPtr

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_DATA_DIR = os.path.join(BASE_DIR, "test_data")


def ensure_test_data_root():
    if not os.path.isdir(TEST_DATA_DIR):
        raise RuntimeError(
            "未找到 test_data 目录。请先运行 scripts/generate_test_data.ps1 或 scripts/generate_test_data.sh。"
        )


class TestGisAiInit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = GisAiLibrary()

    def test_init(self):
        ret = self.lib.init()
        self.assertEqual(ret, 0, f"Init failed: {self.lib.get_last_error()}")

    def test_init_with_null_config(self):
        ret = self.lib.init(None)
        self.assertEqual(ret, 0)

    def test_shutdown(self):
        self.lib.shutdown()


class TestGisAiRasterIO(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ensure_test_data_root()
        cls.lib = GisAiLibrary()
        cls.lib.init()
        cls.raster_path = os.path.join(TEST_DATA_DIR, "raster", "test_100x100.tif")
        cls.skip_raster = not os.path.exists(cls.raster_path)

    @classmethod
    def tearDownClass(cls):
        cls.lib.shutdown()

    def test_raster_load(self):
        if self.skip_raster:
            self.skipTest("Test raster file not found")
        raster = self.lib.raster_load(self.raster_path)
        self.assertIsNotNone(raster, f"Load failed: {self.lib.get_last_error()}")
        self.lib.raster_destroy(raster)

    def test_raster_dimensions(self):
        if self.skip_raster:
            self.skipTest("Test raster file not found")
        raster = self.lib.raster_load(self.raster_path)
        self.assertIsNotNone(raster)
        try:
            w = self.lib.raster_get_width(raster)
            h = self.lib.raster_get_height(raster)
            b = self.lib.raster_get_band_count(raster)
            self.assertEqual(w, 100)
            self.assertEqual(h, 100)
            self.assertEqual(b, 3)
        finally:
            self.lib.raster_destroy(raster)

    def test_raster_save_and_reload(self):
        if self.skip_raster:
            self.skipTest("Test raster file not found")
        raster = self.lib.raster_load(self.raster_path)
        self.assertIsNotNone(raster)
        try:
            with tempfile.NamedTemporaryFile(suffix=".tif", delete=False) as f:
                out_path = f.name
            try:
                ret = self.lib.raster_save(raster, out_path)
                self.assertEqual(ret, 0, f"Save failed: {self.lib.get_last_error()}")

                raster2 = self.lib.raster_load(out_path)
                self.assertIsNotNone(raster2, f"Reload failed: {self.lib.get_last_error()}")
                self.assertEqual(self.lib.raster_get_width(raster2), 100)
                self.assertEqual(self.lib.raster_get_height(raster2), 100)
                self.lib.raster_destroy(raster2)
            finally:
                if os.path.exists(out_path):
                    os.unlink(out_path)
        finally:
            self.lib.raster_destroy(raster)

    def test_raster_resample(self):
        if self.skip_raster:
            self.skipTest("Test raster file not found")
        raster = self.lib.raster_load(self.raster_path)
        self.assertIsNotNone(raster)
        try:
            resampled = self.lib.raster_resample(raster, 50, 50, "bilinear")
            self.assertIsNotNone(resampled, f"Resample failed: {self.lib.get_last_error()}")
            self.assertEqual(self.lib.raster_get_width(resampled), 50)
            self.assertEqual(self.lib.raster_get_height(resampled), 50)
            self.lib.raster_destroy(resampled)
        finally:
            self.lib.raster_destroy(raster)

    def test_raster_normalize(self):
        if self.skip_raster:
            self.skipTest("Test raster file not found")
        raster = self.lib.raster_load(self.raster_path)
        self.assertIsNotNone(raster)
        try:
            normalized = self.lib.raster_normalize(raster)
            self.assertIsNotNone(normalized, f"Normalize failed: {self.lib.get_last_error()}")
            self.assertEqual(self.lib.raster_get_width(normalized), 100)
            self.lib.raster_destroy(normalized)
        finally:
            self.lib.raster_destroy(raster)

    def test_raster_threshold(self):
        if self.skip_raster:
            self.skipTest("Test raster file not found")
        raster = self.lib.raster_load(self.raster_path)
        self.assertIsNotNone(raster)
        try:
            result = self.lib.raster_threshold(raster, 128.0)
            self.assertIsNotNone(result, f"Threshold failed: {self.lib.get_last_error()}")
            self.lib.raster_destroy(result)
        finally:
            self.lib.raster_destroy(raster)


class TestGisAiVectorIO(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ensure_test_data_root()
        cls.lib = GisAiLibrary()
        cls.lib.init()
        cls.shp_path = os.path.join(TEST_DATA_DIR, "vector", "test_polygons.shp")
        cls.geojson_path = os.path.join(TEST_DATA_DIR, "vector", "test_points.geojson")
        cls.skip_shp = not os.path.exists(cls.shp_path)
        cls.skip_geojson = not os.path.exists(cls.geojson_path)

    @classmethod
    def tearDownClass(cls):
        cls.lib.shutdown()

    def test_vector_load_shapefile(self):
        if self.skip_shp:
            self.skipTest("Test shapefile not found")
        vector = self.lib.vector_load(self.shp_path)
        self.assertIsNotNone(vector, f"Load shapefile failed: {self.lib.get_last_error()}")
        self.lib.vector_destroy(vector)

    def test_vector_load_geojson(self):
        if self.skip_geojson:
            self.skipTest("Test GeoJSON not found")
        vector = self.lib.vector_load(self.geojson_path)
        self.assertIsNotNone(vector, f"Load GeoJSON failed: {self.lib.get_last_error()}")
        self.lib.vector_destroy(vector)

    def test_vector_buffer(self):
        if self.skip_shp:
            self.skipTest("Test shapefile not found")
        vector = self.lib.vector_load(self.shp_path)
        self.assertIsNotNone(vector)
        try:
            buffered = self.lib.vector_buffer(vector, 1.0)
            self.assertIsNotNone(buffered, f"Buffer failed: {self.lib.get_last_error()}")
            self.lib.vector_destroy(buffered)
        finally:
            self.lib.vector_destroy(vector)

    def test_vector_simplify(self):
        if self.skip_shp:
            self.skipTest("Test shapefile not found")
        vector = self.lib.vector_load(self.shp_path)
        self.assertIsNotNone(vector)
        try:
            simplified = self.lib.vector_simplify(vector, 0.5)
            self.assertIsNotNone(simplified, f"Simplify failed: {self.lib.get_last_error()}")
            self.lib.vector_destroy(simplified)
        finally:
            self.lib.vector_destroy(vector)

    def test_vector_save_and_reload(self):
        if self.skip_geojson:
            self.skipTest("Test GeoJSON not found")
        vector = self.lib.vector_load(self.geojson_path)
        self.assertIsNotNone(vector)
        try:
            out_dir = tempfile.mkdtemp()
            out_path = os.path.join(out_dir, "test_output.geojson")
            try:
                ret = self.lib.vector_save(vector, out_path)
                self.assertEqual(ret, 0, f"Save failed: {self.lib.get_last_error()}")

                vector2 = self.lib.vector_load(out_path)
                self.assertIsNotNone(vector2, f"Reload failed: {self.lib.get_last_error()}")
                self.lib.vector_destroy(vector2)
            finally:
                import shutil
                shutil.rmtree(out_dir, ignore_errors=True)
        finally:
            self.lib.vector_destroy(vector)


class TestGisAiCoordTransform(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = GisAiLibrary()
        cls.lib.init()

    @classmethod
    def tearDownClass(cls):
        cls.lib.shutdown()

    def test_transform_wgs84_to_utm(self):
        try:
            x, y = self.lib.transform_coordinates(116.4, 39.9, "EPSG:4326", "EPSG:32650")
            self.assertIsInstance(x, float)
            self.assertIsInstance(y, float)
            self.assertGreater(x, 400000)
            self.assertGreater(y, 4000000)
        except Exception as e:
            if "proj" in str(e).lower() or "PROJ" in str(e):
                self.skipTest(f"PROJ database not available: {e}")
            raise

    def test_transform_identity(self):
        try:
            x, y = self.lib.transform_coordinates(116.4, 39.9, "EPSG:4326", "EPSG:4326")
            self.assertAlmostEqual(x, 116.4, places=5)
            self.assertAlmostEqual(y, 39.9, places=5)
        except Exception as e:
            if "proj" in str(e).lower():
                self.skipTest(f"PROJ database not available: {e}")
            raise


class TestGisAiAI(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ensure_test_data_root()
        cls.lib = GisAiLibrary()
        cls.lib.init()
        cls.model_path = os.path.join(TEST_DATA_DIR, "models", "test_seg_model.onnx")
        cls.raster_path = os.path.join(TEST_DATA_DIR, "raster", "test_100x100.tif")
        cls.skip_model = not os.path.exists(cls.model_path)
        cls.skip_raster = not os.path.exists(cls.raster_path)

    @classmethod
    def tearDownClass(cls):
        cls.lib.shutdown()

    def test_model_load_unload(self):
        if self.skip_model:
            self.skipTest("ONNX model not found")
        model = self.lib.model_load(self.model_path)
        self.assertIsNotNone(model, f"Model load failed: {self.lib.get_last_error()}")
        self.lib.model_unload(model)

    def test_ai_infer(self):
        if self.skip_model or self.skip_raster:
            self.skipTest("Model or raster file not found")
        model = self.lib.model_load(self.model_path)
        self.assertIsNotNone(model)
        try:
            raster = self.lib.raster_load(self.raster_path)
            self.assertIsNotNone(raster)
            try:
                result = self.lib.ai_infer(model, raster)
                self.assertIsNotNone(result, f"Inference failed: {self.lib.get_last_error()}")
                self.lib.raster_destroy(result)
            finally:
                self.lib.raster_destroy(raster)
        finally:
            self.lib.model_unload(model)

    def test_raster_seg(self):
        if self.skip_model or self.skip_raster:
            self.skipTest("Model or raster file not found")
        seg = self.lib.raster_seg_create(self.model_path)
        self.assertIsNotNone(seg, f"RasterSeg create failed: {self.lib.get_last_error()}")
        try:
            with tempfile.NamedTemporaryFile(suffix=".tif", delete=False) as f:
                out_tif = f.name
            try:
                ret = self.lib.raster_seg_run(seg, self.raster_path, out_tif)
                self.assertEqual(ret, 0, f"RasterSeg run failed: {self.lib.get_last_error()}")
                self.assertTrue(os.path.exists(out_tif))
            finally:
                if os.path.exists(out_tif):
                    os.unlink(out_tif)
        finally:
            self.lib.raster_seg_destroy(seg)


if __name__ == "__main__":
    unittest.main(verbosity=2)
