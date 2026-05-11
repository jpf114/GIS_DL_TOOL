#ifndef GIS_AI_VECTOR_IO_H
#define GIS_AI_VECTOR_IO_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <optional>
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
    std::vector<std::vector<Coordinate>> inner_rings;
    std::map<std::string, AttributeValue> attributes;
};

struct GIS_AI_API VectorData {
    FeatureType feature_type = FeatureType::Point;
    std::string projection;
    std::vector<Feature> features;
};

struct GIS_AI_API LayerInfo {
    std::string name;
    FeatureType feature_type = FeatureType::Point;
    int feature_count = 0;
};

class GIS_AI_API VectorIO {
public:
    VectorIO() = default;
    ~VectorIO() = default;

    std::unique_ptr<VectorData> Load(const std::string& path,
                                      const std::string& layer_name = "");

    std::unique_ptr<VectorData> LoadLayer(const std::string& path, int layer_index);

    std::vector<LayerInfo> ListLayers(const std::string& path);

    void Save(const VectorData& data, const std::string& path);

private:
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_IO_H
