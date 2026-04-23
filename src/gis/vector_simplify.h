#ifndef GIS_AI_VECTOR_SIMPLIFY_H
#define GIS_AI_VECTOR_SIMPLIFY_H

#include <memory>
#include <string>
#include "io/vector_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API VectorSimplify {
public:
    VectorSimplify() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& input, double tolerance);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_SIMPLIFY_H
