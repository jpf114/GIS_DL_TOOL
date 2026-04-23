#ifndef GIS_AI_INFERENCE_ENGINE_H
#define GIS_AI_INFERENCE_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include "core/export.h"

namespace gis_ai {

class ModelManager;

struct GIS_AI_API InferenceResult {
    std::vector<std::vector<float>> outputs;
    std::vector<std::vector<int64_t>> shapes;
    double inference_time_ms = 0.0;
};

class GIS_AI_API InferenceEngine {
public:
    explicit InferenceEngine(ModelManager& model_manager);

    InferenceResult Run(const std::string& model_name,
                        const std::vector<float>& input_data,
                        const std::vector<int64_t>& input_shape);

    InferenceResult RunMultiInput(const std::string& model_name,
                                   const std::vector<std::vector<float>>& inputs_data,
                                   const std::vector<std::vector<int64_t>>& inputs_shape);

private:
    ModelManager& model_manager_;
};

} // namespace gis_ai

#endif // GIS_AI_INFERENCE_ENGINE_H
