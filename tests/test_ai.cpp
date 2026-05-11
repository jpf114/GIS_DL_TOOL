#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <numeric>

#include "ai/preprocess.h"
#include "ai/postprocess.h"
#include "ai/mask_to_polygon.h"
#include "io/raster_io.h"

using namespace gis_ai;

class PreprocessTest : public ::testing::Test {
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
            r.bands.push_back(std::vector<float>(w * h, static_cast<float>(b + 1) * 0.5f));
        }
        return r;
    }
};

TEST_F(PreprocessTest, RasterToTensorBasic) {
    auto raster = MakeTestRaster(10, 10, 3);
    Preprocess preprocess;
    PreprocessConfig config;
    config.target_width = 10;
    config.target_height = 10;
    config.normalize_mode = NormalizeMode::None;

    auto tensor = preprocess.RasterToTensor(raster, config);
    EXPECT_EQ(tensor.size(), static_cast<size_t>(10 * 10 * 3));
}

TEST_F(PreprocessTest, RasterToTensorWithResize) {
    auto raster = MakeTestRaster(10, 10, 3);
    Preprocess preprocess;
    PreprocessConfig config;
    config.target_width = 20;
    config.target_height = 20;
    config.normalize_mode = NormalizeMode::None;

    auto tensor = preprocess.RasterToTensor(raster, config);
    EXPECT_EQ(tensor.size(), static_cast<size_t>(20 * 20 * 3));
}

TEST_F(PreprocessTest, RasterToTensorSingleBandPadsToThreeChannels) {
    auto raster = MakeTestRaster(10, 10, 1);
    Preprocess preprocess;
    PreprocessConfig config;
    config.target_width = 10;
    config.target_height = 10;
    config.normalize_mode = NormalizeMode::None;

    auto tensor = preprocess.RasterToTensor(raster, config);
    ASSERT_EQ(tensor.size(), static_cast<size_t>(10 * 10 * 3));

    EXPECT_FLOAT_EQ(tensor[0], 0.5f);
    EXPECT_FLOAT_EQ(tensor[99], 0.5f);
    EXPECT_FLOAT_EQ(tensor[100], 0.0f);
    EXPECT_FLOAT_EQ(tensor[199], 0.0f);
    EXPECT_FLOAT_EQ(tensor[200], 0.0f);
    EXPECT_FLOAT_EQ(tensor[299], 0.0f);
}

TEST_F(PreprocessTest, RasterToTensorWithNormalization) {
    auto raster = MakeTestRaster(10, 10, 3);
    Preprocess preprocess;
    PreprocessConfig config;
    config.target_width = 10;
    config.target_height = 10;
    config.normalize_mode = NormalizeMode::ImageNet;
    config.input_is_uint8 = false;

    auto tensor = preprocess.RasterToTensor(raster, config);
    EXPECT_EQ(tensor.size(), static_cast<size_t>(10 * 10 * 3));

    bool has_transformed = false;
    for (auto v : tensor) {
        if (std::abs(v - 0.5f) > 0.01f) { has_transformed = true; break; }
    }
    EXPECT_TRUE(has_transformed);
}

TEST_F(PreprocessTest, RasterToTensorMinMaxNorm) {
    auto raster = MakeTestRaster(10, 10, 3);
    Preprocess preprocess;
    PreprocessConfig config;
    config.target_width = 10;
    config.target_height = 10;
    config.normalize_mode = NormalizeMode::MinMax01;

    auto tensor = preprocess.RasterToTensor(raster, config);
    EXPECT_EQ(tensor.size(), static_cast<size_t>(10 * 10 * 3));

    for (auto v : tensor) {
        EXPECT_GE(v, -0.01f);
        EXPECT_LE(v, 1.01f);
    }
}

TEST_F(PreprocessTest, RasterBandToTensor) {
    auto raster = MakeTestRaster(10, 10, 3);
    Preprocess preprocess;

    auto tensor = preprocess.RasterBandToTensor(raster, 0, 20);
    EXPECT_EQ(tensor.size(), static_cast<size_t>(20 * 20));
}

TEST_F(PreprocessTest, GetInputShape) {
    PreprocessConfig config;
    config.target_width = 512;
    config.target_height = 256;

    auto shape = Preprocess::GetInputShape(config);
    ASSERT_EQ(shape.size(), 4u);
    EXPECT_EQ(shape[0], 1);
    EXPECT_EQ(shape[1], 3);
    EXPECT_EQ(shape[2], 256);
    EXPECT_EQ(shape[3], 512);
}

class PostprocessTest : public ::testing::Test {
protected:
    std::vector<float> MakeLogits(int h, int w, int classes) {
        std::vector<float> logits(1 * classes * h * w);
        for (int i = 0; i < h * w; ++i) {
            for (int c = 0; c < classes; ++c) {
                logits[i * classes + c] = static_cast<float>(c) * 0.1f;
            }
        }
        return logits;
    }
};

TEST_F(PostprocessTest, SigmoidArgmax) {
    Postprocess postprocess;
    int h = 4, w = 4, classes = 3;
    auto logits = MakeLogits(h, w, classes);

    auto mask = postprocess.SigmoidArgmax(logits, h, w, classes);
    EXPECT_EQ(mask.size(), static_cast<size_t>(h * w));

    for (auto val : mask) {
        EXPECT_GE(val, 0);
        EXPECT_LT(val, classes);
    }
}

TEST_F(PostprocessTest, SigmoidArgmaxTwoClasses) {
    Postprocess postprocess;
    int h = 2, w = 2, classes = 2;
    size_t total = h * w;
    std::vector<float> logits(classes * total);
    logits[0 * total + 0] = 10.0f;
    logits[0 * total + 1] = -10.0f;
    logits[0 * total + 2] = 10.0f;
    logits[0 * total + 3] = -10.0f;
    logits[1 * total + 0] = -10.0f;
    logits[1 * total + 1] = 10.0f;
    logits[1 * total + 2] = -10.0f;
    logits[1 * total + 3] = 10.0f;

    auto mask = postprocess.SigmoidArgmax(logits, h, w, classes);
    EXPECT_EQ(mask.size(), 4u);
    EXPECT_EQ(mask[0], 0);
    EXPECT_EQ(mask[1], 1);
    EXPECT_EQ(mask[2], 0);
    EXPECT_EQ(mask[3], 1);
}

TEST_F(PostprocessTest, Sigmoid) {
    Postprocess postprocess;
    std::vector<float> logits = {0.0f, 1.0f, -1.0f, 10.0f, -10.0f};

    auto sigmoid = postprocess.Sigmoid(logits);
    EXPECT_EQ(sigmoid.size(), logits.size());

    EXPECT_NEAR(sigmoid[0], 0.5f, 0.01f);
    EXPECT_GT(sigmoid[1], 0.5f);
    EXPECT_LT(sigmoid[2], 0.5f);
    EXPECT_NEAR(sigmoid[3], 1.0f, 0.01f);
    EXPECT_NEAR(sigmoid[4], 0.0f, 0.01f);
}

TEST_F(PostprocessTest, ThresholdMask) {
    Postprocess postprocess;
    std::vector<float> mask = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};

    auto result = postprocess.ThresholdMask(mask, 0.5f);
    EXPECT_EQ(result.size(), mask.size());
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[1], 0);
    EXPECT_EQ(result[2], 1);
    EXPECT_EQ(result[3], 1);
    EXPECT_EQ(result[4], 1);
}

TEST_F(PostprocessTest, MaskToRaster) {
    Postprocess postprocess;
    std::vector<uint8_t> mask = {0, 1, 0, 1, 1, 0, 0, 1, 1};
    double geotransform[6] = {0.0, 1.0, 0.0, 0.0, 0.0, -1.0};

    auto raster = postprocess.MaskToRaster(mask, 3, 3, geotransform, "EPSG:4326");
    EXPECT_EQ(raster.width, 3);
    EXPECT_EQ(raster.height, 3);
    EXPECT_EQ(raster.band_count, 1);
    EXPECT_EQ(raster.bands[0].size(), 9u);
}

class MaskToPolygonTest : public ::testing::Test {
protected:
    std::vector<uint8_t> MakeSquareMask(int size) {
        std::vector<uint8_t> mask(size * size, 0);
        int margin = size / 4;
        for (int y = margin; y < size - margin; ++y) {
            for (int x = margin; x < size - margin; ++x) {
                mask[y * size + x] = 1;
            }
        }
        return mask;
    }
};

TEST_F(MaskToPolygonTest, ExecuteBasic) {
    MaskToPolygon m2p;
    auto mask = MakeSquareMask(20);
    double geotransform[6] = {0.0, 1.0, 0.0, 0.0, 0.0, -1.0};

    auto result = m2p.Execute(mask, 20, 20, geotransform, 1);
    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->features.size(), 0u);
}

TEST_F(MaskToPolygonTest, ExecuteEmptyMask) {
    MaskToPolygon m2p;
    std::vector<uint8_t> mask(10 * 10, 0);
    double geotransform[6] = {0.0, 1.0, 0.0, 0.0, 0.0, -1.0};

    auto result = m2p.Execute(mask, 10, 10, geotransform, 1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->features.size(), 0u);
}

TEST_F(MaskToPolygonTest, ExecuteFromRaster) {
    MaskToPolygon m2p;
    RasterData raster;
    raster.width = 20;
    raster.height = 20;
    raster.band_count = 1;
    raster.geotransform[0] = 0.0; raster.geotransform[1] = 1.0; raster.geotransform[2] = 0.0;
    raster.geotransform[3] = 0.0; raster.geotransform[4] = 0.0; raster.geotransform[5] = -1.0;
    raster.bands.push_back(std::vector<float>(20 * 20, 0.0f));

    for (int y = 5; y < 15; ++y) {
        for (int x = 5; x < 15; ++x) {
            raster.bands[0][y * 20 + x] = 1.0f;
        }
    }

    auto result = m2p.ExecuteFromRaster(raster, 1);
    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->features.size(), 0u);
}
