#ifndef GIS_AI_VECTOR_BUFFER_H
#define GIS_AI_VECTOR_BUFFER_H

#include <memory>
#include <string>
#include "io/vector_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API VectorBuffer {
public:
    VectorBuffer() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& input, double distance, int quad_segments = 8);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_BUFFER_H
