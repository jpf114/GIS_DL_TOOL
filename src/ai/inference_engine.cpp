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
            throw GisAiModelException(
                "Unsupported output tensor element type: " + std::to_string(static_cast<int>(element_type)),
                "InferenceEngine::Run");
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

    auto start = std::chrono::high_resolution_clock::now();

    auto output_tensors = session->Run(
        Ort::RunOptions{nullptr},
        input_names.data(), input_tensors.data(), input_tensors.size(),
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
            throw GisAiModelException(
                "Unsupported output tensor element type: " + std::to_string(static_cast<int>(element_type)),
                "InferenceEngine::RunMultiInput");
        }
    }

    LOG_INFO("Multi-input inference completed: model='" + model_name + "', time=" +
        std::to_string(elapsed_ms) + "ms");
    return result;
}

BatchInferenceResult InferenceEngine::RunBatch(const std::string& model_name,
                                                 const std::vector<std::vector<float>>& inputs_data,
                                                 const std::vector<int64_t>& single_input_shape) {
    BatchInferenceResult batch_result;

    if (inputs_data.empty()) {
        return batch_result;
    }

    if (inputs_data.size() == 1) {
        InferenceResult single = Run(model_name, inputs_data[0], single_input_shape);
        batch_result.results.push_back(std::move(single));
        batch_result.total_inference_time_ms = batch_result.results[0].inference_time_ms;
        return batch_result;
    }

    auto* session = GetSession(model_manager_, model_name);
    if (!session) {
        throw GisAiModelException("Model '" + model_name + "' not loaded",
            "InferenceEngine::RunBatch");
    }

    size_t batch_size = inputs_data.size();
    size_t single_size = inputs_data[0].size();

    std::vector<int64_t> batch_shape = single_input_shape;
    batch_shape[0] = static_cast<int64_t>(batch_size);

    std::vector<float> batch_data;
    batch_data.reserve(batch_size * single_size);
    for (const auto& input : inputs_data) {
        batch_data.insert(batch_data.end(), input.begin(), input.end());
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    auto input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, batch_data.data(), batch_data.size(),
        batch_shape.data(), batch_shape.size());

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
    batch_result.total_inference_time_ms = elapsed_ms;

    for (size_t t = 0; t < batch_size; ++t) {
        InferenceResult single_result;
        single_result.inference_time_ms = elapsed_ms / static_cast<double>(batch_size);

        for (size_t i = 0; i < output_tensors.size(); ++i) {
            auto& tensor = output_tensors[i];
            auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
            auto full_shape = tensor_info.GetShape();

            std::vector<int64_t> single_out_shape = full_shape;
            single_out_shape[0] = 1;

            size_t single_out_elements = 1;
            for (size_t d = 1; d < single_out_shape.size(); ++d) {
                single_out_elements *= static_cast<size_t>(single_out_shape[d]);
            }

            size_t offset = t * single_out_elements;
            auto element_type = tensor_info.GetElementType();

            if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                const float* data = tensor.GetTensorData<float>();
                single_result.outputs.push_back(
                    std::vector<float>(data + offset, data + offset + single_out_elements));
            } else if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
                const double* data = tensor.GetTensorData<double>();
                std::vector<float> converted(single_out_elements);
                for (size_t j = 0; j < single_out_elements; ++j) {
                    converted[j] = static_cast<float>(data[offset + j]);
                }
                single_result.outputs.push_back(std::move(converted));
            } else if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
                const int64_t* data = tensor.GetTensorData<int64_t>();
                std::vector<float> converted(single_out_elements);
                for (size_t j = 0; j < single_out_elements; ++j) {
                    converted[j] = static_cast<float>(data[offset + j]);
                }
                single_result.outputs.push_back(std::move(converted));
            } else {
                throw GisAiModelException(
                    "Unsupported output tensor element type: " + std::to_string(static_cast<int>(element_type)),
                    "InferenceEngine::RunBatch");
            }

            single_result.shapes.push_back(single_out_shape);
        }

        batch_result.results.push_back(std::move(single_result));
    }

    LOG_INFO("Batch inference completed: model='" + model_name + "', batch_size=" +
        std::to_string(batch_size) + ", time=" + std::to_string(elapsed_ms) + "ms");
    return batch_result;
}

} // namespace gis_ai
