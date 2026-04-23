#include "io/io_factory.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "io/pointcloud_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <algorithm>
#include <filesystem>

namespace gis_ai {

static std::string GetExtensionLower(const std::string& path) {
	std::string ext = std::filesystem::path(path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext;
}

IOFormat IOFactory::DetectFormat(const std::string& path) {
	std::string ext = GetExtensionLower(path);

	if (ext == ".tif" || ext == ".tiff") {
		return IOFormat::Raster;
	}
	if (ext == ".shp" || ext == ".geojson" || ext == ".json") {
		return IOFormat::Vector;
	}
	if (ext == ".las" || ext == ".laz") {
		return IOFormat::PointCloud;
	}

	return IOFormat::Unknown;
}

std::unique_ptr<RasterIO> IOFactory::CreateRasterIO() {
	LOG_INFO("Creating RasterIO handler");
	return std::make_unique<RasterIO>();
}

std::unique_ptr<VectorIO> IOFactory::CreateVectorIO() {
	LOG_INFO("Creating VectorIO handler");
	return std::make_unique<VectorIO>();
}

std::unique_ptr<PointCloudIO> IOFactory::CreatePointCloudIO() {
	LOG_INFO("Creating PointCloudIO handler");
	return std::make_unique<PointCloudIO>();
}

} // namespace gis_ai
