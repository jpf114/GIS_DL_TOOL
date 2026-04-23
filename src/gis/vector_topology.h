#ifndef GIS_AI_VECTOR_TOPOLOGY_H
#define GIS_AI_VECTOR_TOPOLOGY_H

#include <string>
#include <vector>
#include "io/vector_io.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API TopologyIssue {
    int feature_index = -1;
    std::string issue_type;
    std::string description;
};

class GIS_AI_API VectorTopology {
public:
    VectorTopology() = default;

    std::vector<TopologyIssue> CheckOverlaps(const VectorData& data);

    std::vector<TopologyIssue> CheckGaps(const VectorData& data, double tolerance = 1e-6);

    std::vector<TopologyIssue> CheckValidity(const VectorData& data);

    std::vector<TopologyIssue> FullCheck(const VectorData& data, double tolerance = 1e-6);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_TOPOLOGY_H
