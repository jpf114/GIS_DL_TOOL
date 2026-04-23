#ifndef GIS_AI_VECTOR_IO_H
#define GIS_AI_VECTOR_IO_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include "core/export.h"

#include <ogr_core.h>

namespace gis_ai {

enum class GIS_AI_API FeatureType {
	Point,
	LineString,
	Polygon
};

struct GIS_AI_API Coordinate {
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
};

using AttributeValue = std::variant<std::string, int, double>;

struct GIS_AI_API Feature {
	FeatureType type = FeatureType::Point;
	std::vector<Coordinate> coordinates;
	std::map<std::string, AttributeValue> attributes;
};

struct GIS_AI_API VectorData {
	FeatureType feature_type = FeatureType::Point;
	std::string projection;
	std::vector<Feature> features;
};

class GIS_AI_API VectorIO {
public:
	VectorIO() = default;
	~VectorIO() = default;

	std::unique_ptr<VectorData> Load(const std::string& path);
	void Save(const VectorData& data, const std::string& path);

private:
	static FeatureType DetectFeatureType(OGRwkbGeometryType geom_type);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_IO_H
