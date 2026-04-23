#include "ai/model_manager.h"
#include "ai/ort_context.h"
#include "core/logger.h"
#include "core/exception.h"

#include <filesystem>
#include <onnxruntime_cxx_api.h>

namespace gis_ai {

struct ModelInfoInternal {
    std::string model_path;
    std::unique_ptr<Ort::Session> session;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<std::vector<int64_t>> input_shapes;
    std::vector<std::vector<int64_t>> output_shapes;
};

struct ModelManager::Impl {
    std::unordered_map<std::string, std::unique_ptr<ModelInfoInternal>> models;
    std::unordered_map<std::string, std::unique_ptr<ModelInfo>> public_infos;
};

ModelManager::ModelManager() : impl_(std::make_unique<Impl>()) {}

ModelManager::~ModelManager() = default;

int ModelManager::LoadModel(const std::string& model_path, const std::string& model_name) {
    if (!std::filesystem::exists(model_path)) {
        throw GisAiModelException("Model file not found: " + model_path, "ModelManager::LoadModel");
    }

    std::string name = model_name;
    if (name.empty()) {
        name = std::filesystem::path(model_path).stem().string();
    }

    if (impl_->models.find(name) != impl_->models.end()) {
        LOG_WARN("Model '" + name + "' already loaded, reloading");
        impl_->models.erase(name);
        impl_->public_infos.erase(name);
    }

    auto internal = std::make_unique<ModelInfoInternal>();
    internal->model_path = model_path;

    try {
        auto& ctx = OrtContext::Instance();
#ifdef _WIN32
        std::wstring wpath(model_path.begin(), model_path.end());
        internal->session = std::make_unique<Ort::Session>(ctx.GetEnv(), wpath.c_str(), ctx.GetSessionOptions());
#else
        internal->session = std::make_unique<Ort::Session>(ctx.GetEnv(), model_path.c_str(), ctx.GetSessionOptions());
#endif

        Ort::AllocatorWithDefaultOptions allocator;

        size_t num_inputs = internal->session->GetInputCount();
        for (size_t i = 0; i < num_inputs; ++i) {
            auto name_ptr = internal->session->GetInputNameAllocated(i, allocator);
            internal->input_names.push_back(name_ptr.get());

            auto type_info = internal->session->GetInputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            internal->input_shapes.push_back(tensor_info.GetShape());
        }

        size_t num_outputs = internal->session->GetOutputCount();
        for (size_t i = 0; i < num_outputs; ++i) {
            auto name_ptr = internal->session->GetOutputNameAllocated(i, allocator);
            internal->output_names.push_back(name_ptr.get());

            auto type_info = internal->session->GetOutputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            internal->output_shapes.push_back(tensor_info.GetShape());
        }
    } catch (const Ort::Exception& e) {
        throw GisAiModelException("Failed to load ONNX model: " + std::string(e.what()),
            "ModelManager::LoadModel");
    }

    auto public_info = std::make_unique<ModelInfo>();
    public_info->model_path = model_path;
    public_info->input_names = internal->input_names;
    public_info->output_names = internal->output_names;
    public_info->input_shapes = internal->input_shapes;
    public_info->output_shapes = internal->output_shapes;
    public_info->session_handle = internal->session.get();

    LOG_INFO("Model loaded: '" + name + "' from " + model_path +
        " (" + std::to_string(internal->input_names.size()) + " inputs, " +
        std::to_string(internal->output_names.size()) + " outputs)");

    impl_->models[name] = std::move(internal);
    impl_->public_infos[name] = std::move(public_info);
    return static_cast<int>(impl_->models.size());
}

void ModelManager::UnloadModel(const std::string& model_name) {
    auto it = impl_->models.find(model_name);
    if (it != impl_->models.end()) {
        impl_->models.erase(it);
        impl_->public_infos.erase(model_name);
        LOG_INFO("Model unloaded: '" + model_name + "'");
    } else {
        LOG_WARN("Model '" + model_name + "' not found for unloading");
    }
}

ModelInfo* ModelManager::GetModel(const std::string& model_name) {
    auto it = impl_->public_infos.find(model_name);
    if (it != impl_->public_infos.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ModelManager::HasModel(const std::string& model_name) const {
    return impl_->models.find(model_name) != impl_->models.end();
}

std::vector<std::string> ModelManager::GetLoadedModels() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : impl_->models) {
        names.push_back(name);
    }
    return names;
}

Ort::Session* GetSession(ModelManager& manager, const std::string& model_name) {
    auto* mgr_impl = manager.impl_.get();
    auto it = mgr_impl->models.find(model_name);
    if (it != mgr_impl->models.end()) {
        return it->second->session.get();
    }
    return nullptr;
}

std::vector<const char*> GetInputNames(ModelManager& manager, const std::string& model_name) {
    auto* mgr_impl = manager.impl_.get();
    auto it = mgr_impl->models.find(model_name);
    if (it != mgr_impl->models.end()) {
        std::vector<const char*> names;
        for (const auto& n : it->second->input_names) {
            names.push_back(n.c_str());
        }
        return names;
    }
    return {};
}

std::vector<const char*> GetOutputNames(ModelManager& manager, const std::string& model_name) {
    auto* mgr_impl = manager.impl_.get();
    auto it = mgr_impl->models.find(model_name);
    if (it != mgr_impl->models.end()) {
        std::vector<const char*> names;
        for (const auto& n : it->second->output_names) {
            names.push_back(n.c_str());
        }
        return names;
    }
    return {};
}

} // namespace gis_ai
