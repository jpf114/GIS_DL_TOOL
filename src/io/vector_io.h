#ifndef GIS_AI_VECTOR_IO_H
#define GIS_AI_VECTOR_IO_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>

namespace gis_ai {

enum class FeatureType {
	Point,
	LineString,
	Polygon
};

struct Coordinate {
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
};

using AttributeValue = std::variant<std::string, int, double>;

struct Feature {
	FeatureType type = FeatureType::Point;
	std::vector<Coordinate> coordinates;
	std::map<std::string, AttributeValue> attributes;
};

struct VectorData {
	FeatureType feature_type = FeatureType::Point;
	std::string projection;
	std::vector<Feature> features;
};

class VectorIO {
public:
	VectorIO() = default;
	~VectorIO() = default;

	std::unique_ptr<VectorData> Load(const std::string& path);
	void Save(const std::string& path, const VectorData& data);

private:
	static FeatureType DetectFeatureType(enum OGRwkbGeometryType geom_type);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_IO_H
