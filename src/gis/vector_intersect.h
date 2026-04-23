#ifndef GIS_AI_VECTOR_INTERSECT_H
#define GIS_AI_VECTOR_INTERSECT_H

#include <memory>
#include <string>
#include "io/vector_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API VectorIntersect {
public:
    VectorIntersect() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& a, const VectorData& b);

    bool Intersects(const VectorData& a, const VectorData& b);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_INTERSECT_H
