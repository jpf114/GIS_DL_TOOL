#ifndef GIS_AI_IO_FACTORY_H
#define GIS_AI_IO_FACTORY_H

#include <string>
#include <memory>

namespace gis_ai {

class RasterIO;
class VectorIO;
class PointCloudIO;

enum class IOFormat {
	Raster,
	Vector,
	PointCloud,
	Unknown
};

class IOFactory {
public:
	static IOFormat DetectFormat(const std::string& path);

	static std::unique_ptr<RasterIO> CreateRasterIO();
	static std::unique_ptr<VectorIO> CreateVectorIO();
	static std::unique_ptr<PointCloudIO> CreatePointCloudIO();
};

} // namespace gis_ai

#endif // GIS_AI_IO_FACTORY_H
