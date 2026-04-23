#ifndef GIS_AI_VECTOR_CLIP_H
#define GIS_AI_VECTOR_CLIP_H

#include <memory>
#include <string>
#include "io/vector_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API VectorClip {
public:
    VectorClip() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& target, const VectorData& clipper);
};

} // namespace gis_ai

#endif // GIS_AI_VECTOR_CLIP_H
