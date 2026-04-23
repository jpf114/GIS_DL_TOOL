#ifndef GIS_AI_POINTCLOUD_IO_H
#define GIS_AI_POINTCLOUD_IO_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace gis_ai {

struct Point {
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	float intensity = 0.0f;
	uint8_t classification = 0;
};

struct PointCloudData {
	std::vector<Point> points;
	std::string coordinate_system;
};

class PointCloudIO {
public:
	PointCloudIO() = default;
	~PointCloudIO() = default;

	std::unique_ptr<PointCloudData> Load(const std::string& path);
	void Save(const std::string& path, const PointCloudData& data);
};

} // namespace gis_ai

#endif // GIS_AI_POINTCLOUD_IO_H
