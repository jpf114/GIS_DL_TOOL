#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <filesystem>

#include "fusion/raster_seg.h"
#include "fusion/batch_processor.h"
#include "fusion/large_image_seg.h"
#include "ai/preprocess.h"
#include "ai/postprocess.h"
#include "ai/mask_to_polygon.h"
#include "io/raster_io.h"
#include "io/vector_io.h"

namespace fs = std::filesystem;

using namespace gis_ai;

class RasterSegConfigTest : public ::testing::Test {};

TEST_F(RasterSegConfigTest, DefaultConfig) {
    RasterSegConfig config;
    EXPECT_EQ(config.input_size, 512);
    EXPECT_FLOAT_EQ(config.mean_r, 0.485f);
    EXPECT_FLOAT_EQ(config.mean_g, 0.456f);
    EXPECT_FLOAT_EQ(config.mean_b, 0.406f);
    EXPECT_FLOAT_EQ(config.std_r, 0.229f);
    EXPECT_FLOAT_EQ(config.std_g, 0.224f);
    EXPECT_FLOAT_EQ(config.std_b, 0.225f);
    EXPECT_FLOAT_EQ(config.mask_threshold, 0.5f);
    EXPECT_EQ(config.target_class, 1);
}

TEST_F(RasterSegConfigTest, CustomConfig) {
    RasterSegConfig config;
    config.input_size = 256;
    config.mean_r = 0.5f;
    config.mean_g = 0.5f;
    config.mean_b = 0.5f;
    config.std_r = 0.5f;
    config.std_g = 0.5f;
    config.std_b = 0.5f;
    config.mask_threshold = 0.3f;
    config.target_class = 2;

    EXPECT_EQ(config.input_size, 256);
    EXPECT_FLOAT_EQ(config.mean_r, 0.5f);
    EXPECT_EQ(config.target_class, 2);
}

class BatchResultTest : public ::testing::Test {};

TEST_F(BatchResultTest, DefaultValues) {
    BatchResult result;
    EXPECT_TRUE(result.input_path.empty());
    EXPECT_TRUE(result.output_tif_path.empty());
    EXPECT_TRUE(result.output_shp_path.empty());
    EXPECT_FALSE(result.success);
    EXPECT_DOUBLE_EQ(result.inference_time_ms, 0.0);
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(BatchResultTest, SetValues) {
    BatchResult result;
    result.input_path = "input.tif";
    result.output_tif_path = "output.tif";
    result.output_shp_path = "output.shp";
    result.success = true;
    result.inference_time_ms = 42.5;
    result.error_message = "no error";

    EXPECT_EQ(result.input_path, "input.tif");
    EXPECT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.inference_time_ms, 42.5);
}

class FusionDataFlowTest : public ::testing::Test {
protected:
    RasterData MakeTestRaster(int w, int h, int bands) {
        RasterData r;
        r.width = w;
        r.height = h;
        r.band_count = bands;
        r.geotransform[0] = 0.0; r.geotransform[1] = 1.0; r.geotransform[2] = 0.0;
        r.geotransform[3] = 0.0; r.geotransform[4] = 0.0; r.geotransform[5] = -1.0;
        r.projection = "EPSG:4326";
        for (int b = 0; b < bands; ++b) {
            std::vector<float> band_data(w * h);
            for (int i = 0; i < w * h; ++i) {
                band_data[i] = static_cast<float>(rand() % 256) / 255.0f;
            }
            r.bands.push_back(std::move(band_data));
        }
        return r;
    }
};

TEST_F(FusionDataFlowTest, PreprocessToPostprocessPipeline) {
    auto raster = MakeTestRaster(10, 10, 3);

    Preprocess preprocess;
    PreprocessConfig config;
    config.target_width = 10;
    config.target_height = 10;
    config.normalize_mode = NormalizeMode::ImageNet;
    config.input_is_uint8 = false;

    auto tensor = preprocess.RasterToTensor(raster, config);
    EXPECT_EQ(tensor.size(), 300u);

    auto shape = Preprocess::GetInputShape(config);
    EXPECT_EQ(shape.size(), 4u);

    std::vector<float> fake_output(2 * 10 * 10, 0.5f);
    for (int i = 0; i < 100; ++i) {
        fake_output[i] = 1.0f;
        fake_output[100 + i] = 0.0f;
    }

    Postprocess postprocess;
    auto mask = postprocess.SigmoidArgmax(fake_output, 10, 10, 2);
    EXPECT_EQ(mask.size(), 100u);

    for (auto val : mask) {
        EXPECT_EQ(val, 0);
    }
}

TEST_F(FusionDataFlowTest, MaskToPolygonFromSyntheticMask) {
    int size = 20;
    std::vector<uint8_t> mask(size * size, 0);
    for (int y = 5; y < 15; ++y) {
        for (int x = 5; x < 15; ++x) {
            mask[y * size + x] = 1;
        }
    }

    MaskToPolygon m2p;
    double geotransform[6] = {100.0, 0.5, 0.0, 50.0, 0.0, -0.5};
    auto result = m2p.Execute(mask, size, size, geotransform, 1);

    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->features.size(), 0u);

    for (const auto& feat : result->features) {
        EXPECT_EQ(feat.type, FeatureType::Polygon);
        EXPECT_GT(feat.coordinates.size(), 0u);
    }
}

TEST_F(FusionDataFlowTest, FullPipelineWithoutModel) {
    auto raster = MakeTestRaster(10, 10, 3);

    Preprocess preprocess;
    Postprocess postprocess;

    PreprocessConfig config;
    config.target_width = 10;
    config.target_height = 10;
    config.normalize_mode = NormalizeMode::None;

    auto tensor = preprocess.RasterToTensor(raster, config);
    auto shape = Preprocess::GetInputShape(config);

    std::vector<float> fake_output(2 * 10 * 10);
    for (int i = 0; i < 200; ++i) {
        fake_output[i] = (i % 2 == 0) ? 2.0f : -2.0f;
    }

    auto mask = postprocess.SigmoidArgmax(fake_output, 10, 10, 2);
    EXPECT_EQ(mask.size(), 100u);

    auto raster_result = postprocess.MaskToRaster(mask, 10, 10, raster.geotransform, raster.projection);
    EXPECT_EQ(raster_result.width, 10);
    EXPECT_EQ(raster_result.height, 10);

    MaskToPolygon m2p;
    auto vector_result = m2p.Execute(mask, 10, 10, raster.geotransform, 0);
    ASSERT_NE(vector_result, nullptr);
}

class LargeImageSegConfigTest : public ::testing::Test {};

TEST_F(LargeImageSegConfigTest, DefaultConfig) {
    LargeImageSegConfig config;
    EXPECT_EQ(config.tile_size, 512);
    EXPECT_EQ(config.stride, 384);
    EXPECT_EQ(config.target_channels, 3);
    EXPECT_FLOAT_EQ(config.mask_threshold, 0.5f);
    EXPECT_EQ(config.target_class, 1);
    EXPECT_EQ(config.blend_mode, BlendMode::Gaussian);
    EXPECT_TRUE(config.skip_nodata);
    EXPECT_GT(config.min_polygon_area, 0.0);
    EXPECT_GT(config.simplify_tolerance, 0.0);
    EXPECT_TRUE(config.fix_topology);
}

TEST_F(LargeImageSegConfigTest, CustomConfig) {
    LargeImageSegConfig config;
    config.tile_size = 256;
    config.stride = 128;
    config.blend_mode = BlendMode::Linear;
    config.skip_nodata = false;
    config.min_polygon_area = 50.0;
    config.simplify_tolerance = 0.5;

    EXPECT_EQ(config.tile_size, 256);
    EXPECT_EQ(config.stride, 128);
    EXPECT_EQ(config.blend_mode, BlendMode::Linear);
    EXPECT_FALSE(config.skip_nodata);
}

class SegmentationStatsTest : public ::testing::Test {};

TEST_F(SegmentationStatsTest, DefaultValues) {
    SegmentationStats stats;
    EXPECT_EQ(stats.total_tiles, 0);
    EXPECT_EQ(stats.skipped_tiles, 0);
    EXPECT_EQ(stats.inferred_tiles, 0);
    EXPECT_DOUBLE_EQ(stats.total_inference_time_ms, 0.0);
    EXPECT_DOUBLE_EQ(stats.total_time_ms, 0.0);
    EXPECT_EQ(stats.polygon_count, 0);
    EXPECT_DOUBLE_EQ(stats.total_polygon_area, 0.0);
}

class BlendWeightsTest : public ::testing::Test {};

TEST_F(BlendWeightsTest, BlendModeEnum) {
    EXPECT_NE(BlendMode::None, BlendMode::Linear);
    EXPECT_NE(BlendMode::Linear, BlendMode::Gaussian);
    EXPECT_NE(BlendMode::None, BlendMode::Gaussian);
}

class LargeImageSegIntegrationTest : public ::testing::Test {
protected:
    static std::string FindTestModel() {
        std::vector<std::string> candidates = {
            "scripts/test_e2e_data/test_seg_model.onnx",
            "../scripts/test_e2e_data/test_seg_model.onnx",
            "../../scripts/test_e2e_data/test_seg_model.onnx",
        };
        for (const auto& c : candidates) {
            if (fs::exists(c)) return c;
        }
        return "";
    }

    static std::string FindTestTiff() {
        std::vector<std::string> candidates = {
            "scripts/test_e2e_data/test_input.tif",
            "../scripts/test_e2e_data/test_input.tif",
            "../../scripts/test_e2e_data/test_input.tif",
        };
        for (const auto& c : candidates) {
            if (fs::exists(c)) return c;
        }
        return "";
    }
};

TEST_F(LargeImageSegIntegrationTest, SegmentWithTestModel) {
    std::string model_path = FindTestModel();
    std::string tiff_path = FindTestTiff();

    if (model_path.empty() || tiff_path.empty()) {
        GTEST_SKIP() << "Test model/data not found, skipping integration test";
    }

    RasterIO rio;
    auto raster = rio.Load(tiff_path);
    ASSERT_NE(raster, nullptr);

    LargeImageSegConfig config;
    config.tile_size = 512;
    config.stride = 256;
    config.blend_mode = BlendMode::Gaussian;
    config.target_class = 1;
    config.skip_nodata = false;

    LargeImageSeg seg(model_path);

    auto result = seg.Segment(*raster, config);
    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->width, 0);
    EXPECT_GT(result->height, 0);
    EXPECT_GT(result->band_count, 0);
    EXPECT_FALSE(result->projection.empty());
}

TEST_F(LargeImageSegIntegrationTest, SegmentToPolygonWithTestModel) {
    std::string model_path = FindTestModel();
    std::string tiff_path = FindTestTiff();

    if (model_path.empty() || tiff_path.empty()) {
        GTEST_SKIP() << "Test model/data not found, skipping integration test";
    }

    RasterIO rio;
    auto raster = rio.Load(tiff_path);
    ASSERT_NE(raster, nullptr);

    LargeImageSegConfig config;
    config.tile_size = 512;
    config.stride = 256;
    config.blend_mode = BlendMode::Linear;
    config.target_class = 1;
    config.skip_nodata = false;

    LargeImageSeg seg(model_path);

    auto result = seg.SegmentToPolygon(*raster, config);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->feature_type, FeatureType::Polygon);
}

TEST_F(LargeImageSegIntegrationTest, SegmentToFileWithTestModel) {
    std::string model_path = FindTestModel();
    std::string tiff_path = FindTestTiff();

    if (model_path.empty() || tiff_path.empty()) {
        GTEST_SKIP() << "Test model/data not found, skipping integration test";
    }

    LargeImageSegConfig config;
    config.tile_size = 512;
    config.stride = 512;
    config.blend_mode = BlendMode::None;
    config.target_class = 1;
    config.skip_nodata = false;

    LargeImageSeg seg(model_path);

    std::string output_tif = "test_data/largeseg_output.tif";
    std::string output_shp = "test_data/largeseg_output.shp";

    int result = seg.SegmentToFile(tiff_path, output_tif, output_shp, config);
    EXPECT_EQ(result, 0);

    auto stats = seg.GetLastStats();
    EXPECT_GT(stats.total_tiles, 0);
    EXPECT_GT(stats.inferred_tiles, 0);
    EXPECT_GT(stats.total_time_ms, 0.0);

    EXPECT_TRUE(fs::exists(output_tif));
    EXPECT_TRUE(fs::exists(output_shp));

    RasterIO rio;
    auto loaded_tif = rio.Load(output_tif);
    ASSERT_NE(loaded_tif, nullptr);
    EXPECT_GT(loaded_tif->width, 0);

    VectorIO vio;
    auto loaded_shp = vio.Load(output_shp);
    ASSERT_NE(loaded_shp, nullptr);
    EXPECT_EQ(loaded_shp->feature_type, FeatureType::Polygon);
}

TEST_F(LargeImageSegIntegrationTest, GetStatsBeforeSegment) {
    std::string model_path = FindTestModel();
    if (model_path.empty()) {
        GTEST_SKIP() << "Test model not found, skipping integration test";
    }

    LargeImageSeg seg(model_path);
    auto stats = seg.GetLastStats();
    EXPECT_EQ(stats.total_tiles, 0);
    EXPECT_EQ(stats.inferred_tiles, 0);
    EXPECT_DOUBLE_EQ(stats.total_time_ms, 0.0);
}
