#include <gtest/gtest.h>
#include "core/logger.h"
#include "core/exception.h"
#include "ai/ort_context.h"
#include "ai/model_manager.h"
#include "ai/inference_engine.h"
#include "ai/preprocess.h"
#include "ai/postprocess.h"
#include "ai/mask_to_polygon.h"
#include "fusion/raster_seg.h"
#include "io/raster_io.h"

#include <cmath>
#include <filesystem>

class AIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        gis_ai::Logger::Instance().Initialize("test_ai_integration.log");
    }

    static bool TestDataRootExists() { return std::filesystem::exists("test_data"); }
    static void SkipIfTestDataMissing() {
        if (!TestDataRootExists()) {
            GTEST_SKIP() << "test_data directory not found; run scripts/generate_test_data.ps1 or scripts/generate_test_data.sh first";
        }
    }

    static std::string ModelPath() { return "test_data/models/test_seg_model.onnx"; }
    static std::string RasterPath() { return "test_data/raster/test_100x100.tif"; }
    static bool ModelExists() { return std::filesystem::exists(ModelPath()); }
    static bool RasterExists() { return std::filesystem::exists(RasterPath()); }
};

TEST_F(AIIntegrationTest, ModelManagerLoadUnload) {
    SkipIfTestDataMissing();
    if (!ModelExists()) GTEST_SKIP() << "ONNX model not found";

    gis_ai::ModelManager manager;
    ASSERT_NO_THROW(manager.LoadModel(ModelPath(), "test_model"));

    EXPECT_TRUE(manager.HasModel("test_model"));
    auto* info = manager.GetModel("test_model");
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->session_handle, nullptr);
    EXPECT_EQ(info->input_names.size(), 1u);
    EXPECT_EQ(info->output_names.size(), 1u);

    auto models = manager.GetLoadedModels();
    EXPECT_EQ(models.size(), 1u);
    EXPECT_EQ(models[0], "test_model");

    manager.UnloadModel("test_model");
    EXPECT_FALSE(manager.HasModel("test_model"));
}

TEST_F(AIIntegrationTest, InferenceEngineRun) {
    SkipIfTestDataMissing();
    if (!ModelExists()) GTEST_SKIP() << "ONNX model not found";

    gis_ai::ModelManager manager;
    manager.LoadModel(ModelPath(), "test_model");

    gis_ai::InferenceEngine engine(manager);

    std::vector<float> input_data(1 * 3 * 512 * 512, 0.5f);
    std::vector<int64_t> input_shape = {1, 3, 512, 512};

    auto result = engine.Run("test_model", input_data, input_shape);

    EXPECT_EQ(result.outputs.size(), 1u);
    EXPECT_EQ(result.shapes.size(), 1u);
    EXPECT_GT(result.inference_time_ms, 0.0);
    EXPECT_EQ(result.shapes[0].size(), 4u);
    EXPECT_EQ(result.shapes[0][0], 1);
    EXPECT_EQ(result.shapes[0][1], 2);
    EXPECT_EQ(result.shapes[0][2], 512);
    EXPECT_EQ(result.shapes[0][3], 512);

    size_t total = 1 * 2 * 512 * 512;
    EXPECT_EQ(result.outputs[0].size(), total);
}

TEST_F(AIIntegrationTest, PreprocessRasterToTensor) {
    SkipIfTestDataMissing();
    if (!RasterExists()) GTEST_SKIP() << "Test raster not found";

    gis_ai::RasterIO io;
    auto raster = io.Load(RasterPath());
    ASSERT_NE(raster, nullptr);

    gis_ai::Preprocess preprocess;
    gis_ai::PreprocessConfig config;
    config.target_width = 512;
    config.target_height = 512;

    auto tensor = preprocess.RasterToTensor(*raster, config);
    EXPECT_EQ(tensor.size(), static_cast<size_t>(3 * 512 * 512));

    auto shape = gis_ai::Preprocess::GetInputShape(config);
    EXPECT_EQ(shape.size(), 4u);
    EXPECT_EQ(shape[0], 1);
    EXPECT_EQ(shape[1], 3);
    EXPECT_EQ(shape[2], 512);
    EXPECT_EQ(shape[3], 512);
}

TEST_F(AIIntegrationTest, PostprocessSigmoidArgmax) {
    std::vector<float> output_data(2 * 4 * 4, 0.0f);

    for (int i = 0; i < 16; ++i) {
        output_data[i] = 5.0f;
        output_data[16 + i] = -5.0f;
    }

    gis_ai::Postprocess postprocess;
    auto mask = postprocess.SigmoidArgmax(output_data, 4, 4, 2);

    EXPECT_EQ(mask.size(), 16u);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(mask[i], 0);
    }

    for (int i = 0; i < 16; ++i) {
        output_data[i] = -5.0f;
        output_data[16 + i] = 5.0f;
    }

    auto mask2 = postprocess.SigmoidArgmax(output_data, 4, 4, 2);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(mask2[i], 1);
    }
}

TEST_F(AIIntegrationTest, PostprocessThresholdMask) {
    std::vector<float> mask = {0.1f, 0.5f, 0.9f, 0.3f, 0.7f, 0.2f};

    gis_ai::Postprocess postprocess;
    auto result = postprocess.ThresholdMask(mask, 0.5f);

    EXPECT_EQ(result.size(), 6u);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[1], 1);
    EXPECT_EQ(result[2], 1);
    EXPECT_EQ(result[3], 0);
    EXPECT_EQ(result[4], 1);
    EXPECT_EQ(result[5], 0);
}

TEST_F(AIIntegrationTest, MaskToPolygonExecute) {
    std::vector<uint8_t> mask = {
        0, 0, 0, 0, 0,
        0, 1, 1, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 1, 1, 0,
        0, 0, 0, 0, 0
    };

    double gt[6] = {0.0, 1.0, 0.0, 5.0, 0.0, -1.0};

    gis_ai::MaskToPolygon m2p;
    auto result = m2p.Execute(mask, 5, 5, gt, 1);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->feature_type, gis_ai::FeatureType::Polygon);
}

TEST_F(AIIntegrationTest, EndToEndInference) {
    SkipIfTestDataMissing();
    if (!ModelExists() || !RasterExists()) GTEST_SKIP() << "Test data not found";

    gis_ai::RasterIO rio;
    auto raster = rio.Load(RasterPath());
    ASSERT_NE(raster, nullptr);

    gis_ai::ModelManager manager;
    manager.LoadModel(ModelPath(), "test_model");

    gis_ai::InferenceEngine engine(manager);
    gis_ai::Preprocess preprocess;
    gis_ai::Postprocess postprocess;

    gis_ai::PreprocessConfig config;
    config.target_width = 512;
    config.target_height = 512;

    auto tensor = preprocess.RasterToTensor(*raster, config);
    auto shape = gis_ai::Preprocess::GetInputShape(config);

    auto result = engine.Run("test_model", tensor, shape);

    EXPECT_EQ(result.outputs.size(), 1u);
    EXPECT_GT(result.inference_time_ms, 0.0);

    auto& output = result.outputs[0];
    auto& out_shape = result.shapes[0];

    int64_t num_classes = out_shape[1];
    int64_t out_h = out_shape[2];
    int64_t out_w = out_shape[3];

    auto mask = postprocess.SigmoidArgmax(output, out_h, out_w, num_classes);
    auto result_raster = postprocess.MaskToRaster(mask, static_cast<int>(out_w), static_cast<int>(out_h),
        raster->geotransform, raster->projection);

    EXPECT_EQ(result_raster.width, 512);
    EXPECT_EQ(result_raster.height, 512);
    EXPECT_EQ(result_raster.band_count, 1);

    rio.Save(result_raster, "test_data/raster/test_seg_output.tif");
    EXPECT_TRUE(std::filesystem::exists("test_data/raster/test_seg_output.tif"));

    std::cout << "End-to-end inference time: " << result.inference_time_ms << " ms" << std::endl;
}

TEST_F(AIIntegrationTest, RasterSegEndToEnd) {
    SkipIfTestDataMissing();
    if (!ModelExists() || !RasterExists()) GTEST_SKIP() << "Test data not found";

    gis_ai::RasterSeg seg(ModelPath());

    gis_ai::RasterIO rio;
    auto raster = rio.Load(RasterPath());
    ASSERT_NE(raster, nullptr);

    auto result = seg.Segment(*raster);
    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->width, 0);
    EXPECT_GT(result->height, 0);

    std::cout << "RasterSeg inference time: " << seg.GetLastInferenceTimeMs() << " ms" << std::endl;
}

TEST_F(AIIntegrationTest, RasterSegSegmentToFile) {
    SkipIfTestDataMissing();
    if (!ModelExists() || !RasterExists()) GTEST_SKIP() << "Test data not found";

    gis_ai::RasterSeg seg(ModelPath());

    int ret = seg.SegmentToFile(RasterPath(), "test_data/raster/test_seg_e2e.tif", "test_data/vector/test_seg_e2e.shp");
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(std::filesystem::exists("test_data/raster/test_seg_e2e.tif"));

    std::cout << "SegmentToFile inference time: " << seg.GetLastInferenceTimeMs() << " ms" << std::endl;
}
