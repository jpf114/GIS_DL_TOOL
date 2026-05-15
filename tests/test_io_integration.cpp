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

    static bool TestDataFileExists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    static std::string MissingDataMessage(const std::string& path) {
        return "未找到测试数据文件: " + path +
               "，请先构建 generate_test_data 工具并运行以生成测试数据";
    }

    static std::string RasterPath() { return "test_data/raster/test_100x100.tif"; }
    static std::string ShpPath() { return "test_data/vector/test_polygons.shp"; }
    static std::string GeoJsonPath() { return "test_data/vector/test_points.geojson"; }

    static void ExpectFiniteCoordinates(const gis_ai::Feature& feature) {
        for (const auto& coord : feature.coordinates) {
            EXPECT_TRUE(std::isfinite(coord.x));
            EXPECT_TRUE(std::isfinite(coord.y));
            EXPECT_TRUE(std::isfinite(coord.z));
        }
        for (const auto& ring : feature.inner_rings) {
            for (const auto& coord : ring) {
                EXPECT_TRUE(std::isfinite(coord.x));
                EXPECT_TRUE(std::isfinite(coord.y));
                EXPECT_TRUE(std::isfinite(coord.z));
            }
        }
    }
};

TEST_F(IOIntegrationTest, RasterLoadSave) {
    if (!TestDataFileExists(RasterPath())) {
        GTEST_SKIP() << MissingDataMessage(RasterPath());
        return;
    }

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
    if (!TestDataFileExists(RasterPath())) {
        GTEST_SKIP() << MissingDataMessage(RasterPath());
        return;
    }

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
    if (!TestDataFileExists(RasterPath())) {
        GTEST_SKIP() << MissingDataMessage(RasterPath());
        return;
    }

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
    if (!TestDataFileExists(RasterPath())) {
        GTEST_SKIP() << MissingDataMessage(RasterPath());
        return;
    }

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
    if (!TestDataFileExists(RasterPath())) {
        GTEST_SKIP() << MissingDataMessage(RasterPath());
        return;
    }

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
    if (!TestDataFileExists(ShpPath())) {
        GTEST_SKIP() << MissingDataMessage(ShpPath());
        return;
    }

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
    if (!TestDataFileExists(GeoJsonPath())) {
        GTEST_SKIP() << MissingDataMessage(GeoJsonPath());
        return;
    }

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
    if (!TestDataFileExists(ShpPath())) {
        GTEST_SKIP() << MissingDataMessage(ShpPath());
        return;
    }

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);

    io.Save(*data, "test_data/vector/test_output.shp");

    auto data2 = io.Load("test_data/vector/test_output.shp");
    ASSERT_NE(data2, nullptr);
    EXPECT_EQ(data2->features.size(), data->features.size());
}

TEST_F(IOIntegrationTest, VectorBufferPipeline) {
    if (!TestDataFileExists(ShpPath())) {
        GTEST_SKIP() << MissingDataMessage(ShpPath());
        return;
    }

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);

    gis_ai::VectorBuffer buffer;
    auto result = buffer.Execute(*data, 0.01);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->feature_type, gis_ai::FeatureType::Polygon);
    EXPECT_GE(result->features.size(), 2u);
    for (const auto& feature : result->features) {
        ExpectFiniteCoordinates(feature);
    }

    io.Save(*result, "test_data/vector/test_buffer.shp");
}

TEST_F(IOIntegrationTest, VectorIntersectPipeline) {
    if (!TestDataFileExists(ShpPath())) {
        GTEST_SKIP() << MissingDataMessage(ShpPath());
        return;
    }

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
    if (!TestDataFileExists(ShpPath())) {
        GTEST_SKIP() << MissingDataMessage(ShpPath());
        return;
    }

    gis_ai::VectorIO io;
    auto data = io.Load(ShpPath());
    ASSERT_NE(data, nullptr);

    gis_ai::VectorSimplify simplify;
    auto result = simplify.Execute(*data, 0.001);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->features.size(), data->features.size());
}

TEST_F(IOIntegrationTest, CoordTransformPipeline) {
    if (!TestDataFileExists(RasterPath())) {
        GTEST_SKIP() << MissingDataMessage(RasterPath());
        return;
    }

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
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.cog"), gis_ai::IOFormat::Raster);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.shp"), gis_ai::IOFormat::Vector);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.geojson"), gis_ai::IOFormat::Vector);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.gpkg"), gis_ai::IOFormat::Vector);
    EXPECT_EQ(gis_ai::IOFactory::DetectFormat("test.las"), gis_ai::IOFormat::PointCloud);
}

TEST_F(IOIntegrationTest, GeoPackageSaveAndLoad) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Polygon;
    data.projection =
        "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]";

    gis_ai::Feature f1;
    f1.type = gis_ai::FeatureType::Polygon;
    f1.coordinates = {{116.0, 39.0, 0}, {116.1, 39.0, 0}, {116.1, 39.1, 0}, {116.0, 39.1, 0}, {116.0, 39.0, 0}};
    f1.attributes["name"] = std::string("poly1");
    f1.attributes["area"] = 42.5;

    gis_ai::Feature f2;
    f2.type = gis_ai::FeatureType::Polygon;
    f2.coordinates = {{117.0, 40.0, 0}, {117.1, 40.0, 0}, {117.1, 40.1, 0}, {117.0, 40.1, 0}, {117.0, 40.0, 0}};
    f2.attributes["name"] = std::string("poly2");
    f2.attributes["area"] = 55.3;

    data.features.push_back(f1);
    data.features.push_back(f2);

    gis_ai::VectorIO io;
    io.Save(data, "test_data/vector/test_output.gpkg");

    auto loaded = io.Load("test_data/vector/test_output.gpkg");
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->feature_type, gis_ai::FeatureType::Polygon);
    EXPECT_EQ(loaded->features.size(), 2u);
    EXPECT_FALSE(loaded->projection.empty());

    for (const auto& feat : loaded->features) {
        EXPECT_EQ(feat.type, gis_ai::FeatureType::Polygon);
        EXPECT_GE(feat.coordinates.size(), 5u);
    }
}

TEST_F(IOIntegrationTest, GeoPackageListLayers) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Point;
    data.projection =
        "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]";

    gis_ai::Feature f;
    f.type = gis_ai::FeatureType::Point;
    f.coordinates = {{116.0, 39.0, 0}};
    data.features.push_back(f);

    gis_ai::VectorIO io;
    io.Save(data, "test_data/vector/test_layers.gpkg");

    auto layers = io.ListLayers("test_data/vector/test_layers.gpkg");
    EXPECT_GE(layers.size(), 1u);
    EXPECT_EQ(layers[0].feature_count, 1);
}

TEST_F(IOIntegrationTest, GeoPackageLoadByName) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Point;
    data.projection =
        "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]";

    gis_ai::Feature f;
    f.type = gis_ai::FeatureType::Point;
    f.coordinates = {{116.0, 39.0, 0}};
    data.features.push_back(f);

    gis_ai::VectorIO io;
    io.Save(data, "test_data/vector/test_loadbyname.gpkg");

    auto layers = io.ListLayers("test_data/vector/test_loadbyname.gpkg");
    ASSERT_GE(layers.size(), 1u);

    auto loaded = io.Load("test_data/vector/test_loadbyname.gpkg", layers[0].name);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->features.size(), 1u);
}

TEST_F(IOIntegrationTest, RasterSaveCOG) {
    gis_ai::RasterData data;
    data.width = 256;
    data.height = 256;
    data.band_count = 1;
    data.geotransform[0] = 116.0;
    data.geotransform[1] = 0.001;
    data.geotransform[3] = 40.0;
    data.geotransform[5] = -0.001;
    data.projection =
        "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]";

    data.bands.resize(1);
    data.bands[0].resize(256 * 256, 100.0f);

    gis_ai::BandInfo bi;
    bi.data_type = gis_ai::RasterDataType::Float32;
    bi.nodata_value = -9999.0f;
    data.band_infos.push_back(bi);

    gis_ai::RasterIO io;
    io.Save(data, "test_data/raster/test_cog.tif", gis_ai::RasterOutputFormat::COG);

    auto loaded = io.Load("test_data/raster/test_cog.tif");
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->width, 256);
    EXPECT_EQ(loaded->height, 256);
    EXPECT_EQ(loaded->band_count, 1);
    EXPECT_FALSE(loaded->projection.empty());
}

TEST_F(IOIntegrationTest, RasterSaveAutoFormat) {
    gis_ai::RasterData data;
    data.width = 64;
    data.height = 64;
    data.band_count = 1;
    data.geotransform[0] = 116.0;
    data.geotransform[1] = 0.001;
    data.geotransform[3] = 40.0;
    data.geotransform[5] = -0.001;
    data.projection =
        "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]";

    data.bands.resize(1);
    data.bands[0].resize(64 * 64, 50.0f);

    gis_ai::RasterIO io;

    io.Save(data, "test_data/raster/test_auto_gtiff.tif", gis_ai::RasterOutputFormat::Auto);
    auto loaded = io.Load("test_data/raster/test_auto_gtiff.tif");
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->width, 64);
    EXPECT_EQ(loaded->height, 64);
}
