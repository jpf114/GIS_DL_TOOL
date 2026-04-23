#include <gtest/gtest.h>

#include "gis/vector_buffer.h"
#include "gis/vector_intersect.h"
#include "gis/vector_clip.h"
#include "gis/vector_simplify.h"
#include "gis/vector_topology.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_clip.h"
#include "gis/raster_threshold.h"
#include "gis/raster_mosaic.h"
#include "gis/pc_filter.h"
#include "gis/pc_downsample.h"
#include "gis/coord_transform.h"
#include "core/exception.h"
#include "core/logger.h"

#include <cmath>
#include <memory>

class GISAlgorithmTest : public ::testing::Test {
protected:
    void SetUp() override {
        gis_ai::Logger::Instance().Initialize("test_gis.log");
    }
};

static gis_ai::VectorData MakeSquarePolygon(double cx, double cy, double half_size) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Polygon;
    gis_ai::Feature f;
    f.type = gis_ai::FeatureType::Polygon;
    f.coordinates = {
        {cx - half_size, cy - half_size, 0.0},
        {cx + half_size, cy - half_size, 0.0},
        {cx + half_size, cy + half_size, 0.0},
        {cx - half_size, cy + half_size, 0.0},
        {cx - half_size, cy - half_size, 0.0}
    };
    data.features.push_back(f);
    return data;
}

static gis_ai::VectorData MakeLineString(const std::vector<gis_ai::Coordinate>& coords) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::LineString;
    gis_ai::Feature f;
    f.type = gis_ai::FeatureType::LineString;
    f.coordinates = coords;
    data.features.push_back(f);
    return data;
}

static gis_ai::RasterData MakeTestRaster(int width, int height, int bands, float fill_value = 1.0f) {
    gis_ai::RasterData data;
    data.width = width;
    data.height = height;
    data.band_count = bands;
    data.geotransform[0] = 100.0;
    data.geotransform[1] = 1.0;
    data.geotransform[2] = 0.0;
    data.geotransform[3] = 200.0;
    data.geotransform[4] = 0.0;
    data.geotransform[5] = -1.0;
    data.projection = "EPSG:4326";
    data.bands.resize(bands);
    for (int b = 0; b < bands; ++b) {
        data.bands[b].resize(static_cast<size_t>(width) * height, fill_value);
    }
    return data;
}

static gis_ai::PointCloudData MakeTestPointCloud(int count) {
    gis_ai::PointCloudData data;
    data.coordinate_system = "EPSG:4326";
    for (int i = 0; i < count; ++i) {
        gis_ai::Point pt;
        pt.x = static_cast<double>(i) * 0.1;
        pt.y = static_cast<double>(i) * 0.2;
        pt.z = static_cast<double>(i) * 0.3;
        pt.intensity = static_cast<float>(i);
        pt.classification = 1;
        data.points.push_back(pt);
    }
    return data;
}

// ==================== Vector Buffer Tests ====================

TEST_F(GISAlgorithmTest, VectorBuffer_PointBuffer) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Point;
    gis_ai::Feature f;
    f.type = gis_ai::FeatureType::Point;
    f.coordinates = {{0.0, 0.0, 0.0}};
    data.features.push_back(f);

    gis_ai::VectorBuffer buffer;
    auto result = buffer.Execute(data, 10.0);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->feature_type, gis_ai::FeatureType::Polygon);
    EXPECT_GE(result->features.size(), 1u);
}

TEST_F(GISAlgorithmTest, VectorBuffer_PolygonBuffer) {
    auto data = MakeSquarePolygon(0.0, 0.0, 5.0);

    gis_ai::VectorBuffer buffer;
    auto result = buffer.Execute(data, 2.0);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->feature_type, gis_ai::FeatureType::Polygon);
    EXPECT_GE(result->features.size(), 1u);
    EXPECT_GE(result->features[0].coordinates.size(), 4u);
}

TEST_F(GISAlgorithmTest, VectorBuffer_EmptyInput) {
    gis_ai::VectorData data;
    gis_ai::VectorBuffer buffer;
    EXPECT_THROW(buffer.Execute(data, 1.0), gis_ai::GisAiAlgorithmException);
}

// ==================== Vector Intersect Tests ====================

TEST_F(GISAlgorithmTest, VectorIntersect_OverlappingPolygons) {
    auto a = MakeSquarePolygon(0.0, 0.0, 5.0);
    auto b = MakeSquarePolygon(3.0, 3.0, 5.0);

    gis_ai::VectorIntersect intersect;
    auto result = intersect.Execute(a, b);

    ASSERT_NE(result, nullptr);
    EXPECT_GE(result->features.size(), 1u);
}

TEST_F(GISAlgorithmTest, VectorIntersect_NonOverlapping) {
    auto a = MakeSquarePolygon(0.0, 0.0, 2.0);
    auto b = MakeSquarePolygon(100.0, 100.0, 2.0);

    gis_ai::VectorIntersect intersect;
    auto result = intersect.Execute(a, b);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->features.size(), 0u);
}

TEST_F(GISAlgorithmTest, VectorIntersect_IntersectsCheck) {
    auto a = MakeSquarePolygon(0.0, 0.0, 5.0);
    auto b = MakeSquarePolygon(3.0, 3.0, 5.0);

    gis_ai::VectorIntersect intersect;
    EXPECT_TRUE(intersect.Intersects(a, b));

    auto c = MakeSquarePolygon(100.0, 100.0, 2.0);
    EXPECT_FALSE(intersect.Intersects(a, c));
}

TEST_F(GISAlgorithmTest, VectorIntersect_EmptyInput) {
    gis_ai::VectorData empty;
    auto a = MakeSquarePolygon(0.0, 0.0, 5.0);

    gis_ai::VectorIntersect intersect;
    EXPECT_THROW(intersect.Execute(empty, a), gis_ai::GisAiAlgorithmException);
}

// ==================== Vector Clip Tests ====================

TEST_F(GISAlgorithmTest, VectorClip_PolygonClip) {
    auto target = MakeSquarePolygon(0.0, 0.0, 10.0);
    auto clipper = MakeSquarePolygon(5.0, 5.0, 5.0);

    gis_ai::VectorClip clip;
    auto result = clip.Execute(target, clipper);

    ASSERT_NE(result, nullptr);
    EXPECT_GE(result->features.size(), 1u);
}

TEST_F(GISAlgorithmTest, VectorClip_NoIntersection) {
    auto target = MakeSquarePolygon(0.0, 0.0, 5.0);
    auto clipper = MakeSquarePolygon(100.0, 100.0, 5.0);

    gis_ai::VectorClip clip;
    auto result = clip.Execute(target, clipper);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->features.size(), 0u);
}

TEST_F(GISAlgorithmTest, VectorClip_EmptyTarget) {
    gis_ai::VectorData empty;
    auto clipper = MakeSquarePolygon(0.0, 0.0, 5.0);

    gis_ai::VectorClip clip;
    EXPECT_THROW(clip.Execute(empty, clipper), gis_ai::GisAiAlgorithmException);
}

// ==================== Vector Simplify Tests ====================

TEST_F(GISAlgorithmTest, VectorSimplify_PolygonSimplify) {
    auto data = MakeSquarePolygon(0.0, 0.0, 10.0);

    gis_ai::VectorSimplify simplify;
    auto result = simplify.Execute(data, 1.0);

    ASSERT_NE(result, nullptr);
    EXPECT_GE(result->features.size(), 1u);
    EXPECT_LE(result->features[0].coordinates.size(), data.features[0].coordinates.size());
}

TEST_F(GISAlgorithmTest, VectorSimplify_ZeroTolerance) {
    auto data = MakeSquarePolygon(0.0, 0.0, 10.0);

    gis_ai::VectorSimplify simplify;
    auto result = simplify.Execute(data, 0.0);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->features[0].coordinates.size(), data.features[0].coordinates.size());
}

TEST_F(GISAlgorithmTest, VectorSimplify_NegativeTolerance) {
    auto data = MakeSquarePolygon(0.0, 0.0, 10.0);
    gis_ai::VectorSimplify simplify;
    EXPECT_THROW(simplify.Execute(data, -1.0), gis_ai::GisAiAlgorithmException);
}

// ==================== Vector Topology Tests ====================

TEST_F(GISAlgorithmTest, VectorTopology_ValidPolygon) {
    auto data = MakeSquarePolygon(0.0, 0.0, 10.0);

    gis_ai::VectorTopology topo;
    auto issues = topo.CheckValidity(data);

    EXPECT_EQ(issues.size(), 0u);
}

TEST_F(GISAlgorithmTest, VectorTopology_OverlappingPolygons) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Polygon;

    auto sq1 = MakeSquarePolygon(0.0, 0.0, 5.0);
    auto sq2 = MakeSquarePolygon(3.0, 3.0, 5.0);

    data.features.push_back(sq1.features[0]);
    data.features.push_back(sq2.features[0]);

    gis_ai::VectorTopology topo;
    auto issues = topo.CheckOverlaps(data);

    EXPECT_GE(issues.size(), 1u);
}

TEST_F(GISAlgorithmTest, VectorTopology_FullCheck) {
    auto data = MakeSquarePolygon(0.0, 0.0, 10.0);

    gis_ai::VectorTopology topo;
    auto issues = topo.FullCheck(data);

    EXPECT_EQ(issues.size(), 0u);
}

// ==================== Raster Resample Tests ====================

TEST_F(GISAlgorithmTest, RasterResample_Nearest) {
    auto data = MakeTestRaster(100, 100, 1, 42.0f);

    gis_ai::RasterResample resample;
    auto result = resample.Execute(data, 50, 50, gis_ai::ResampleMethod::Nearest);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 50);
    EXPECT_EQ(result->height, 50);
    EXPECT_EQ(result->band_count, 1);
    EXPECT_FLOAT_EQ(result->bands[0][0], 42.0f);
}

TEST_F(GISAlgorithmTest, RasterResample_Bilinear) {
    auto data = MakeTestRaster(100, 100, 1, 42.0f);

    gis_ai::RasterResample resample;
    auto result = resample.Execute(data, 50, 50, gis_ai::ResampleMethod::Bilinear);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 50);
    EXPECT_EQ(result->height, 50);
    EXPECT_FLOAT_EQ(result->bands[0][0], 42.0f);
}

TEST_F(GISAlgorithmTest, RasterResample_InvalidDimensions) {
    auto data = MakeTestRaster(100, 100, 1);
    gis_ai::RasterResample resample;
    EXPECT_THROW(resample.Execute(data, 0, 50), gis_ai::GisAiAlgorithmException);
    EXPECT_THROW(resample.Execute(data, 50, -1), gis_ai::GisAiAlgorithmException);
}

TEST_F(GISAlgorithmTest, RasterResample_GeoTransformUpdate) {
    auto data = MakeTestRaster(100, 100, 1);

    gis_ai::RasterResample resample;
    auto result = resample.Execute(data, 50, 50);

    ASSERT_NE(result, nullptr);
    EXPECT_DOUBLE_EQ(result->geotransform[0], data.geotransform[0]);
    EXPECT_DOUBLE_EQ(result->geotransform[1], data.geotransform[1] * 2.0);
    EXPECT_DOUBLE_EQ(result->geotransform[5], data.geotransform[5] * 2.0);
}

// ==================== Raster Normalize Tests ====================

TEST_F(GISAlgorithmTest, RasterNormalize_BasicNormalize) {
    auto data = MakeTestRaster(10, 10, 1, 0.0f);
    for (size_t i = 0; i < data.bands[0].size(); ++i) {
        data.bands[0][i] = static_cast<float>(i);
    }

    gis_ai::RasterNormalize normalize;
    auto result = normalize.Execute(data);

    ASSERT_NE(result, nullptr);
    EXPECT_FLOAT_EQ(result->bands[0][0], 0.0f);

    float max_val = *std::max_element(result->bands[0].begin(), result->bands[0].end());
    EXPECT_NEAR(max_val, 1.0f, 1e-5f);
}

TEST_F(GISAlgorithmTest, RasterNormalize_MinMaxNormalize) {
    auto data = MakeTestRaster(10, 10, 1, 50.0f);

    gis_ai::RasterNormalize normalize;
    auto result = normalize.ExecuteMinMax(data, 0.0f, 100.0f);

    ASSERT_NE(result, nullptr);
    EXPECT_FLOAT_EQ(result->bands[0][0], 0.5f);
}

TEST_F(GISAlgorithmTest, RasterNormalize_InvalidRange) {
    auto data = MakeTestRaster(10, 10, 1);
    gis_ai::RasterNormalize normalize;
    EXPECT_THROW(normalize.ExecuteMinMax(data, 100.0f, 0.0f), gis_ai::GisAiAlgorithmException);
}

// ==================== Raster Clip Tests ====================

TEST_F(GISAlgorithmTest, RasterClip_ByPixel) {
    auto data = MakeTestRaster(100, 100, 1, 42.0f);

    gis_ai::RasterClip clip;
    auto result = clip.ExecuteByPixel(data, 10, 10, 50, 50);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 50);
    EXPECT_EQ(result->height, 50);
    EXPECT_FLOAT_EQ(result->bands[0][0], 42.0f);
}

TEST_F(GISAlgorithmTest, RasterClip_ByBoundingBox) {
    auto data = MakeTestRaster(100, 100, 1, 42.0f);

    gis_ai::BoundingBox bbox;
    bbox.min_x = 110.0;
    bbox.max_x = 150.0;
    bbox.min_y = 150.0;
    bbox.max_y = 190.0;

    gis_ai::RasterClip clip;
    auto result = clip.Execute(data, bbox);

    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->width, 0);
    EXPECT_GT(result->height, 0);
}

TEST_F(GISAlgorithmTest, RasterClip_OutOfBounds) {
    auto data = MakeTestRaster(100, 100, 1);
    gis_ai::RasterClip clip;
    EXPECT_THROW(clip.ExecuteByPixel(data, 80, 80, 50, 50), gis_ai::GisAiAlgorithmException);
}

// ==================== Raster Threshold Tests ====================

TEST_F(GISAlgorithmTest, RasterThreshold_BasicThreshold) {
    auto data = MakeTestRaster(10, 10, 1, 0.0f);
    for (size_t i = 0; i < data.bands[0].size(); ++i) {
        data.bands[0][i] = static_cast<float>(i);
    }

    gis_ai::RasterThreshold threshold;
    auto result = threshold.Execute(data, 50.0);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->band_count, 1);

    int ones = 0, zeros = 0;
    for (float v : result->bands[0]) {
        if (v == 1.0f) ones++;
        else zeros++;
    }
    EXPECT_GT(ones, 0);
    EXPECT_GT(zeros, 0);
}

TEST_F(GISAlgorithmTest, RasterThreshold_RangeThreshold) {
    auto data = MakeTestRaster(10, 10, 1, 0.0f);
    for (size_t i = 0; i < data.bands[0].size(); ++i) {
        data.bands[0][i] = static_cast<float>(i);
    }

    gis_ai::RasterThreshold threshold;
    auto result = threshold.ExecuteRange(data, 20.0, 60.0);

    ASSERT_NE(result, nullptr);
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_FLOAT_EQ(result->bands[0][i], 0.0f);
    }
    for (size_t i = 20; i <= 60; ++i) {
        EXPECT_FLOAT_EQ(result->bands[0][i], 1.0f);
    }
}

TEST_F(GISAlgorithmTest, RasterThreshold_InvalidBand) {
    auto data = MakeTestRaster(10, 10, 1);
    gis_ai::RasterThreshold threshold;
    EXPECT_THROW(threshold.Execute(data, 0.5, 5), gis_ai::GisAiAlgorithmException);
}

// ==================== Raster Mosaic Tests ====================

TEST_F(GISAlgorithmTest, RasterMosaic_TwoRasters) {
    auto r1 = MakeTestRaster(100, 100, 1, 10.0f);
    auto r2 = MakeTestRaster(100, 100, 1, 20.0f);
    r2.geotransform[0] = 200.0;

    gis_ai::RasterMosaic mosaic;
    std::vector<std::reference_wrapper<const gis_ai::RasterData>> rasters = {r1, r2};
    auto result = mosaic.Execute(rasters);

    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->width, 100);
    EXPECT_GT(result->height, 0);
}

TEST_F(GISAlgorithmTest, RasterMosaic_SingleRaster) {
    auto r1 = MakeTestRaster(100, 100, 1, 42.0f);

    gis_ai::RasterMosaic mosaic;
    std::vector<std::reference_wrapper<const gis_ai::RasterData>> rasters = {r1};
    auto result = mosaic.Execute(rasters);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width, 100);
    EXPECT_EQ(result->height, 100);
}

TEST_F(GISAlgorithmTest, RasterMosaic_EmptyInput) {
    gis_ai::RasterMosaic mosaic;
    std::vector<std::reference_wrapper<const gis_ai::RasterData>> rasters;
    EXPECT_THROW(mosaic.Execute(rasters), gis_ai::GisAiAlgorithmException);
}

// ==================== Point Cloud Filter Tests ====================

TEST_F(GISAlgorithmTest, PcFilter_PassThrough) {
    auto data = MakeTestPointCloud(100);

    gis_ai::PassFilterParams params;
    params.axis = "z";
    params.min_val = 5.0;
    params.max_val = 20.0;

    gis_ai::PcFilter filter;
    auto result = filter.PassThrough(data, params);

    ASSERT_NE(result, nullptr);
    EXPECT_LT(result->points.size(), data.points.size());
    EXPECT_GT(result->points.size(), 0u);

    for (const auto& pt : result->points) {
        EXPECT_GE(pt.z, 5.0);
        EXPECT_LE(pt.z, 20.0);
    }
}

TEST_F(GISAlgorithmTest, PcFilter_PassThroughXAxis) {
    auto data = MakeTestPointCloud(50);

    gis_ai::PassFilterParams params;
    params.axis = "x";
    params.min_val = 0.0;
    params.max_val = 2.0;

    gis_ai::PcFilter filter;
    auto result = filter.PassThrough(data, params);

    ASSERT_NE(result, nullptr);
    for (const auto& pt : result->points) {
        EXPECT_GE(pt.x, 0.0);
        EXPECT_LE(pt.x, 2.0);
    }
}

TEST_F(GISAlgorithmTest, PcFilter_InvalidAxis) {
    auto data = MakeTestPointCloud(10);
    gis_ai::PassFilterParams params;
    params.axis = "w";

    gis_ai::PcFilter filter;
    EXPECT_THROW(filter.PassThrough(data, params), gis_ai::GisAiAlgorithmException);
}

TEST_F(GISAlgorithmTest, PcFilter_EmptyInput) {
    gis_ai::PointCloudData empty;
    gis_ai::PassFilterParams params;
    gis_ai::PcFilter filter;
    EXPECT_THROW(filter.PassThrough(empty, params), gis_ai::GisAiAlgorithmException);
}

// ==================== Point Cloud Downsample Tests ====================

TEST_F(GISAlgorithmTest, PcDownsample_VoxelGrid) {
    auto data = MakeTestPointCloud(100);

    gis_ai::PcDownsample downsample;
    auto result = downsample.VoxelGrid(data, 1.0);

    ASSERT_NE(result, nullptr);
    EXPECT_LT(result->points.size(), data.points.size());
    EXPECT_GT(result->points.size(), 0u);
}

TEST_F(GISAlgorithmTest, PcDownsample_VoxelGridSmallSize) {
    auto data = MakeTestPointCloud(100);

    gis_ai::PcDownsample downsample;
    auto result = downsample.VoxelGrid(data, 0.001);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->points.size(), data.points.size());
}

TEST_F(GISAlgorithmTest, PcDownsample_RandomDownsample) {
    auto data = MakeTestPointCloud(1000);

    gis_ai::PcDownsample downsample;
    auto result = downsample.RandomDownsample(data, 0.5);

    ASSERT_NE(result, nullptr);
    EXPECT_NEAR(result->points.size(), 500u, 50u);
}

TEST_F(GISAlgorithmTest, PcDownsample_InvalidVoxelSize) {
    auto data = MakeTestPointCloud(10);
    gis_ai::PcDownsample downsample;
    EXPECT_THROW(downsample.VoxelGrid(data, -1.0), gis_ai::GisAiAlgorithmException);
}

TEST_F(GISAlgorithmTest, PcDownsample_InvalidRatio) {
    auto data = MakeTestPointCloud(10);
    gis_ai::PcDownsample downsample;
    EXPECT_THROW(downsample.RandomDownsample(data, 0.0), gis_ai::GisAiAlgorithmException);
    EXPECT_THROW(downsample.RandomDownsample(data, 1.5), gis_ai::GisAiAlgorithmException);
}

// ==================== Coord Transform Tests ====================

TEST_F(GISAlgorithmTest, CoordTransform_SinglePoint) {
    gis_ai::CoordTransform transform;
    auto result = transform.Transform(116.0, 39.0, 0.0, "EPSG:4326", "EPSG:3857");

    EXPECT_GT(std::abs(result.x), 0.0);
    EXPECT_GT(std::abs(result.y), 0.0);
}

TEST_F(GISAlgorithmTest, CoordTransform_IdentityTransform) {
    gis_ai::CoordTransform transform;
    auto result = transform.Transform(116.0, 39.0, 0.0, "EPSG:4326", "EPSG:4326");

    EXPECT_NEAR(result.x, 116.0, 1e-8);
    EXPECT_NEAR(result.y, 39.0, 1e-8);
}

TEST_F(GISAlgorithmTest, CoordTransform_BatchTransform) {
    std::vector<gis_ai::CoordPair> coords = {
        {116.0, 39.0, 0.0},
        {117.0, 40.0, 0.0},
        {118.0, 41.0, 0.0}
    };

    gis_ai::CoordTransform transform;
    auto results = transform.TransformBatch(coords, "EPSG:4326", "EPSG:3857");

    EXPECT_EQ(results.size(), 3u);
    for (const auto& r : results) {
        EXPECT_GT(std::abs(r.x), 0.0);
        EXPECT_GT(std::abs(r.y), 0.0);
    }
}

TEST_F(GISAlgorithmTest, CoordTransform_InvalidCRS) {
    gis_ai::CoordTransform transform;
    EXPECT_THROW(transform.Transform(0.0, 0.0, 0.0, "INVALID:CRS", "EPSG:4326"),
        gis_ai::GisAiAlgorithmException);
}

TEST_F(GISAlgorithmTest, CoordTransform_PointCloudTransform) {
    auto pc = MakeTestPointCloud(10);
    pc.coordinate_system = "EPSG:4326";

    gis_ai::CoordTransform transform;
    auto result = transform.TransformPointCloud(pc, "EPSG:3857");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->points.size(), pc.points.size());
    EXPECT_EQ(result->coordinate_system, "EPSG:3857");
}

TEST_F(GISAlgorithmTest, CoordTransform_PointCloudNoCRS) {
    gis_ai::PointCloudData pc;
    gis_ai::CoordTransform transform;
    EXPECT_THROW(transform.TransformPointCloud(pc, "EPSG:3857"),
        gis_ai::GisAiAlgorithmException);
}
