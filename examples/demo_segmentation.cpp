#include "core/logger.h"
#include "core/exception.h"
#include "fusion/raster_seg.h"
#include <iostream>

int main(int argc, char* argv[]) {
    gis_ai::Logger::Instance().Initialize("demo_segmentation.log");

    if (argc < 3) {
        std::cout << "Usage: demo_segmentation <model_path> <input_tif> [output_tif] [output_shp]" << std::endl;
        return 1;
    }

    std::string model_path = argv[1];
    std::string input_path = argv[2];
    std::string output_tif = (argc > 3) ? argv[3] : "output_seg.tif";
    std::string output_shp = (argc > 4) ? argv[4] : "output_seg.shp";

    try {
        gis_ai::RasterSeg seg(model_path);

        int ret = seg.SegmentToFile(input_path, output_tif, output_shp);

        if (ret == 0) {
            std::cout << "Segmentation completed successfully!" << std::endl;
            std::cout << "  Output TIF: " << output_tif << std::endl;
            std::cout << "  Output SHP: " << output_shp << std::endl;
            std::cout << "  Inference time: " << seg.GetLastInferenceTimeMs() << " ms" << std::endl;
        } else {
            std::cerr << "Segmentation failed with code: " << ret << std::endl;
            return 1;
        }
    } catch (const gis_ai::GisAiModelException& e) {
        std::cerr << "Model Error: " << e.what() << std::endl;
        return 1;
    } catch (const gis_ai::GisAiAlgorithmException& e) {
        std::cerr << "Algorithm Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
