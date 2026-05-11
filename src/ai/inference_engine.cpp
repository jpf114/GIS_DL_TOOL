#include "ai/inference_engine.h"
#include "ai/model_manager.h"
#include "ai/ort_context.h"
#include "core/logger.h"
#include "core/exception.h"

#include <chrono>
#include <onnxruntime_cxx_api.h>

namespace gis_ai {

extern Ort::Session* GetSession(ModelManager& manager, const std::string& model_name);
extern std::vector<std::string> GetInputNames(ModelManager& manager, const std::string& model_name);
extern std::vector<std::string> GetOutputNames(ModelManager& manager, const std::string& model_name);

static std::vector<const char*> ToConstCharPtrs(const std::vector<std::string>& names) {
    std::vector<const char*> ptrs;
    ptrs.reserve(names.size());
    for (const auto& n : names) {
        ptrs.push_back(n.c_str());
    }
    return ptrs;
}

static std::vector<float> ConvertToFloat(const int64_t* data, size_t count) {
    std::vector<float> result(count);
    for (size_t i = 0; i < count; ++i) {
        result[i] = static_cast<float>(data[i]);
    }
    return result;
}

static std::vector<float> ConvertToFloat(const double* data, size_t count) {
    std::vector<float> result(count);
    for (size_t i = 0; i < count; ++i) {
        result[i] = static_cast<float>(data[i]);
    }
    return result;
}

InferenceEngine::InferenceEngine(ModelManager& model_manager)
    : model_manager_(model_manager) {
}

InferenceResult InferenceEngine::Run(const std::string& model_name,
                                      const std::vector<float>& input_data,
                                      const std::vector<int64_t>& input_shape) {
    auto* session = GetSession(model_manager_, model_name);
    if (!session) {
        throw GisAiModelException("Model '" + model_name + "' not loaded",
            "InferenceEngine::Run");
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    auto input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, const_cast<float*>(input_data.data()), input_data.size(),
        input_shape.data(), input_shape.size());

    auto input_name_strs = GetInputNames(model_manager_, model_name);
    auto output_name_strs = GetOutputNames(model_manager_, model_name);
    auto input_names = ToConstCharPtrs(input_name_strs);
    auto output_names = ToConstCharPtrs(output_name_strs);

    auto start = std::chrono::high_resolution_clock::now();

    auto output_tensors = session->Run(
        Ort::RunOptions{nullptr},
        input_names.data(), &input_tensor, 1,
        output_names.data(), static_cast<size_t>(output_names.size()));

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    InferenceResult result;
    result.inference_time_ms = elapsed_ms;

    for (size_t i = 0; i < output_tensors.size(); ++i) {
        auto& tensor = output_tensors[i];
        auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
        auto shape = tensor_info.GetShape();
        result.shapes.push_back(shape);

        size_t total = tensor_info.GetElementCount();
        auto element_type = tensor_info.GetElementType();
        if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            const float* data = tensor.GetTensorData<float>();
            result.outputs.push_back(std::vector<float>(data, data + total));
        } else if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
            const double* data = tensor.GetTensorData<double>();
            result.outputs.push_back(ConvertToFloat(data, total));
        } else if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
            const int64_t* data = tensor.GetTensorData<int64_t>();
            result.outputs.push_back(ConvertToFloat(data, total));
        } else {
            const float* data = tensor.GetTensorData<float>();
            result.outputs.push_back(std::vector<float>(data, data + total));
        }
    }

    LOG_INFO("Inference completed: model='" + model_name + "', time=" +
        std::to_string(elapsed_ms) + "ms");
    return result;
}

InferenceResult InferenceEngine::RunMultiInput(const std::string& model_name,
                                                const std::vector<std::vector<float>>& inputs_data,
                                                const std::vector<std::vector<int64_t>>& inputs_shape) {
    auto* session = GetSession(model_manager_, model_name);
    if (!session) {
        throw GisAiModelException("Model '" + model_name + "' not loaded",
            "InferenceEngine::RunMultiInput");
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<Ort::Value> input_tensors;
    for (size_t i = 0; i < inputs_data.size(); ++i) {
        auto tensor = Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(inputs_data[i].data()), inputs_data[i].size(),
            inputs_shape[i].data(), inputs_shape[i].size());
        input_tensors.push_back(std::move(tensor));
    }

    auto input_name_strs = GetInputNames(model_manager_, model_name);
    auto output_name_strs = GetOutputNames(model_manager_, model_name);
    auto input_names = ToConstCharPtrs(input_name_strs);
    auto output_names = ToConstCharPtrs(output_name_strs);

    std::vector<Ort::Value> input_refs;
    for (auto& t : input_tensors) {
        input_refs.push_back(std::move(t));
    }

    auto start = std::chrono::high_resolution_clock::now();

    auto output_tensors = session->Run(
        Ort::RunOptions{nullptr},
        input_names.data(), input_refs.data(), input_refs.size(),
        output_names.data(), static_cast<size_t>(output_names.size()));

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    InferenceResult result;
    result.inference_time_ms = elapsed_ms;

    for (size_t i = 0; i < output_tensors.size(); ++i) {
        auto& tensor = output_tensors[i];
        auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
        auto shape = tensor_info.GetShape();
        result.shapes.push_back(shape);

        size_t total = tensor_info.GetElementCount();
        auto element_type = tensor_info.GetElementType();
        if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            const float* data = tensor.GetTensorData<float>();
            result.outputs.push_back(std::vector<float>(data, data + total));
        } else if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
            const double* data = tensor.GetTensorData<double>();
            result.outputs.push_back(ConvertToFloat(data, total));
        } else if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
            const int64_t* data = tensor.GetTensorData<int64_t>();
            result.outputs.push_back(ConvertToFloat(data, total));
        } else {
            const float* data = tensor.GetTensorData<float>();
            result.outputs.push_back(std::vector<float>(data, data + total));
        }
    }

    LOG_INFO("Multi-input inference completed: model='" + model_name + "', time=" +
        std::to_string(elapsed_ms) + "ms");
    return result;
}

} // namespace gis_ai
