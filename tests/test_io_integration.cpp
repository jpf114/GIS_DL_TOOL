#include <gtest/gtest.h>
#include "core/logger.h"
#include "core/exception.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "io/pointcloud_io.h"
#include "io/io_factory.h"
#include "gis/vector_buffer.h"
#include "gis/vector_intersect.h"
#include "gis/vector_clip.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_clip.h"
#include "gis/raster_threshold.h"
#include "gis/raster_mosaic.h"
#include "gis/vector_simplify.h"
#include "gis/coord_transform.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

class IOIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        gis_ai::Logger::Instance().Initialize("test_io_integration.log");
    }

    static bool TestDataRootExists() { return std::filesystem::exists("test_data"); }
    static void SkipIfTestDataMissing() {
        if (!TestDataRootExists()) {
            GTEST_SKIP() << "未找到 test_data 目录，请先运行 scripts/generate_test_data.ps1 或 scripts/generate_test_data.sh";
        }
    }

    static std::string RasterPath() { return "test_data/raster/test_100x100.tif"; }
    static std::string ShpPath() { return "test_data/vector/test_polygons.shp"; }
    static std::string GeoJsonPath() { return "test_data/vector/test_points.geojson"; }
};

TEST_F(IOIntegrationTest, RasterLoadSave) {
    SkipIfTestDataMissing();

    gis_ai::RasterIO io;
    auto data = io.Load(RasterPath());

    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->width, 100);
    EXPECT_EQ(data->height, 100);
    EXPECT_EQ(data->band_count, 3);
    EXPECT_NE(data->projection.empty(), true);
    EXPECT_DOUBLE_EQ(data->geotransform[0], 116.0);
    EXPECT_DOUBLE_EQ(data->geotransform[1], 0.001);
    EXPECT_DOUBLE_EQ(data->geotransform[5], -0.001);

    io.Save(*data, "test_data/raster/test_output.tif");

    auto data2 = io.Load("test_data/raster/test_output.tif");
    ASSERT_NE(data2, nullptr);
    EXPECT_EQ(data2->width, 100);
    EXPECT_EQ(data2->height, 100);
    EXPECT_EQ(data2->band_count, 3);
}

TEST_F(IOIntegrationTest, RasterResamplePipeline) {
    SkipIfTestDataMissing();

    gis_ai::RasterIO io;
    auto data = io.Load(RasterPath());
    ASSERT_NE(data, nullptr);

    gis_ai::RasterResample resample;
    auto resampled = resample.Execute(*data, 50, 50, gis_ai::ResampleMethod::Bilinear);
    ASSERT_NE(resampled, nullptr);
    EXPECT_EQ(resampled->width, 50);
    EXPECT_EQ(resampled->height, 50);

    io.Save(*resampled, "test_data/raster/test_resampled.tif");
    auto verify = io.Load("test_data/raster/test_resampled.tif");
    ASSERT_NE(verify, nullptr);
    EXPECT_EQ(verify->width, 50);
}

TEST_F(IOIntegrationTest, RasterNormalizePipeline) {
    SkipIfTestDataMissing();

    gis_ai::RasterIO io;
    auto data = io.Load(RasterPath());
    ASSERT_NE(data, nullptr);

    gis_ai::RasterNormalize normalize;
    auto norm = normalize.Execute(*data);
    ASSERT_NE(norm, nullptr);
    EXPECT_EQ(norm->width, 100);
    EXPECT_EQ(norm->height, 100);

    for (int b = 0; b < norm->band_count; ++b) {
        float min_val = *std::min_element(norm->bands[b].begin(), norm->bands[b].end());
        EXPECT_GE(min_val, -0.01f);
        float max_val = *std::max_element(norm->bands[b].begin(), norm->bands[b].end());
        EXPECT_LE(max_val, 1.01f);
    }
}

TEST_F(IOIntegrationTest, RasterClipPipeline) {
    SkipIfTestDataMissing();

    gis_ai::RasterIO io;
    auto data = io.Load(RasterPath());
    ASSERT_NE(data, nullptr);

    gis_ai::RasterClip clip;
    auto clipped = clip.ExecuteByPixel(*data, 10, 10, 50, 50);
    ASSERT_NE(clipped, nullptr);
    EXPECT_EQ(clipped->width, 50);
    EXPECT_EQ(clipped->height, 50);

    io.Save(*clipped, "test_data/raster/test_clipped.tif");
}

TEST_F(IOIntegrationTest, RasterThresholdPipeline) {
    SkipIfTestDataMissing();

    gis_ai::RasterIO io;
    auto data = io.Load(RasterPath());
    ASSERT_NE(data, nullptr);

    gis_ai::RasterThreshold thresh;
    auto result = thresh.Execute(*data, 50.0, 0);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->band_count, 1);

    io.Save(*result, "test_data/raster/test_threshold.tif");
}

TEST_F(IOIntegrationTest, VectorLoadShapefile) {
    SkipIfTestDataMissing();

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());

    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->feature_type, gis_ai::FeatureType::Polygon);
    EXPECT_EQ(data->features.size(), 2u);
    EXPECT_NE(data->projection.empty(), true);

    for (const auto& feat : data->features) {
        EXPECT_EQ(feat.type, gis_ai::FeatureType::Polygon);
        EXPECT_GE(feat.coordinates.size(), 5u);
    }
}

TEST_F(IOIntegrationTest, VectorLoadGeoJSON) {
    SkipIfTestDataMissing();

    gis_ai::VectorIO io;
    auto data = io.Load(GeoJsonPath());

    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->feature_type, gis_ai::FeatureType::Point);
    EXPECT_EQ(data->features.size(), 10u);

    for (const auto& feat : data->features) {
        EXPECT_EQ(feat.type, gis_ai::FeatureType::Point);
        EXPECT_EQ(feat.coordinates.size(), 1u);
    }
}

TEST_F(IOIntegrationTest, VectorSaveAndReload) {
    SkipIfTestDataMissing();

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);

    io.Save(*data, "test_data/vector/test_output.shp");

    auto data2 = io.Load("test_data/vector/test_output.shp");
    ASSERT_NE(data2, nullptr);
    EXPECT_EQ(data2->features.size(), data->features.size());
}

TEST_F(IOIntegrationTest, VectorBufferPipeline) {
    SkipIfTestDataMissing();

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);

    gis_ai::VectorBuffer buffer;
    auto result = buffer.Execute(*data, 0.01);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->feature_type, gis_ai::FeatureType::Polygon);
    EXPECT_GE(result->features.size(), 2u);

    io.Save(*result, "test_data/vector/test_buffer.shp");
}

TEST_F(IOIntegrationTest, VectorIntersectPipeline) {
    SkipIfTestDataMissing();

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);
    EXPECT_GE(data->features.size(), 2u);

    gis_ai::VectorData a, b;
    a.feature_type = gis_ai::FeatureType::Polygon;
    a.projection = data->projection;
    a.features.push_back(data->features[0]);
    b.feature_type = gis_ai::FeatureType::Polygon;
    b.projection = data->projection;
    b.features.push_back(data->features[1]);

    gis_ai::VectorIntersect intersect;
    EXPECT_TRUE(intersect.Intersects(a, b));

    auto result = intersect.Execute(a, b);
    ASSERT_NE(result, nullptr);
}

TEST_F(IOIntegrationTest, VectorSimplifyPipeline) {
    SkipIfTestDataMissing();

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);

    gis_ai::VectorSimplify simplify;
    auto result = simplify.Execute(*data, 0.001);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->features.size(), data->features.size());
}

TEST_F(IOIntegrationTest, CoordTransformPipeline) {
    SkipIfTestDataMissing();

    gis_ai::CoordTransform transform;
    auto result = transform.Transform(116.0, 39.0, 0.0, "EPSG:4326", "EPSG:3857");

    EXPECT_GT(std::abs(result.x), 1e6);
    EXPECT_GT(std::abs(result.y), 1e6);

    auto back = transform.Transform(result.x, result.y, result.z, "EPSG:3857", "EPSG:4326");
    EXPECT_NEAR(back.x, 116.0, 1e-6);
    EXPECT_NEAR(back.y, 39.0, 1e-6);
}

TEST_F(IOIntegrationTest, IOFactoryDetect) {
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.tif"), gis_ai::IOFormat::Raster);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.tiff"), gis_ai::IOFormat::Raster);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.shp"), gis_ai::IOFormat::Vector);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.geojson"), gis_ai::IOFormat::Vector);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.las"), gis_ai::IOFormat::PointCloud);
}
