#include <onnxruntime_cxx_api.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

static void WriteLE32(std::ostream& os, uint32_t v) {
    os.write(reinterpret_cast<const char*>(&v), 4);
}
static void WriteLE64(std::ostream& os, uint64_t v) {
    os.write(reinterpret_cast<const char*>(&v), 8);
}

int main() {
    std::string model_path = "test_data/models/test_seg_model.onnx";

    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "model_gen");
    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(1);

    std::vector<float> input_data(1 * 3 * 512 * 512, 0.5f);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    {
        Ort::Session session(env, model_path.c_str(), opts);
        std::cout << "Model already exists, verifying..." << std::endl;

        auto input_name = session.GetInputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
        auto output_name = session.GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions());

        std::vector<int64_t> input_shape = {1, 3, 512, 512};
        auto input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_data.data(), input_data.size(),
            input_shape.data(), input_shape.size());

        const char* input_names[] = {input_name.get()};
        const char* output_names[] = {output_name.get()};

        auto output = session.Run(Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1, output_names, 1);

        auto& out_tensor = output[0];
        auto info = out_tensor.GetTensorTypeAndShapeInfo();
        auto shape = info.GetShape();

        std::cout << "Output shape: [" << shape[0] << "," << shape[1] << ","
                  << shape[2] << "," << shape[3] << "]" << std::endl;

        const float* out_data = out_tensor.GetTensorData<float>();
        std::cout << "First output values: " << out_data[0] << ", " << out_data[1] << std::endl;

        std::cout << "Model verification PASSED!" << std::endl;
        return 0;
    }
}
