#include <gtest/gtest.h>
#include "core/exception.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "io/pointcloud_io.h"
#include "io/io_factory.h"

using namespace gis_ai;

TEST(IOTest, DetectFormatRaster) {
    EXPECT_EQ(IOFactory::DetectFormat("test.tif"), IOFormat::Raster);
    EXPECT_EQ(IOFactory::DetectFormat("test.tiff"), IOFormat::Raster);
}

TEST(IOTest, DetectFormatVector) {
    EXPECT_EQ(IOFactory::DetectFormat("test.shp"), IOFormat::Vector);
    EXPECT_EQ(IOFactory::DetectFormat("test.geojson"), IOFormat::Vector);
}

TEST(IOTest, DetectFormatPointCloud) {
    EXPECT_EQ(IOFactory::DetectFormat("test.las"), IOFormat::PointCloud);
    EXPECT_EQ(IOFactory::DetectFormat("test.laz"), IOFormat::PointCloud);
}

TEST(IOTest, DetectFormatUnknown) {
    EXPECT_EQ(IOFactory::DetectFormat("test.xyz"), IOFormat::Unknown);
}

TEST(IOTest, CreateRasterIO) {
    auto io = IOFactory::CreateRasterIO();
    EXPECT_NE(io.get(), nullptr);
}

TEST(IOTest, CreateVectorIO) {
    auto io = IOFactory::CreateVectorIO();
    EXPECT_NE(io.get(), nullptr);
}

TEST(IOTest, CreatePointCloudIO) {
    auto io = IOFactory::CreatePointCloudIO();
    EXPECT_NE(io.get(), nullptr);
}

TEST(IOTest, RasterLoadNonexistent) {
    RasterIO io;
    EXPECT_THROW(io.Load("/nonexistent/file.tif"), GisAiIOException);
}

TEST(IOTest, VectorLoadNonexistent) {
    VectorIO io;
    EXPECT_THROW(io.Load("/nonexistent/file.shp"), GisAiIOException);
}

TEST(IOTest, PointCloudLoadNonexistent) {
    PointCloudIO io;
    EXPECT_THROW(io.Load("/nonexistent/file.las"), GisAiIOException);
}
