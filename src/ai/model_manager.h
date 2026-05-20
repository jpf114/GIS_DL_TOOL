#ifndef GIS_AI_MODEL_MANAGER_H
#define GIS_AI_MODEL_MANAGER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "core/export.h"

namespace Ort { class Session; }

namespace gis_ai {

struct ModelInfo {
    std::string model_path;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<std::vector<int64_t>> input_shapes;
    std::vector<std::vector<int64_t>> output_shapes;
};

class GIS_AI_API ModelManager {
public:
    ModelManager();
    ~ModelManager();

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    int LoadModel(const std::string& model_path, const std::string& model_name = "");

    void UnloadModel(const std::string& model_name);

    ModelInfo* GetModel(const std::string& model_name);

    bool HasModel(const std::string& model_name) const;

    std::vector<std::string> GetLoadedModels() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    friend Ort::Session* GetSession(ModelManager& manager, const std::string& model_name);
    friend std::vector<std::string> GetInputNames(ModelManager& manager, const std::string& model_name);
    friend std::vector<std::string> GetOutputNames(ModelManager& manager, const std::string& model_name);
};

} // namespace gis_ai

#endif // GIS_AI_MODEL_MANAGER_H
