#include "core/logger.h"
#include "core/exception.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "io/io_factory.h"
#include "gis/vector_buffer.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_clip.h"
#include "gis/coord_transform.h"
#include <iostream>

int main(int argc, char* argv[]) {
    gis_ai::Logger::Instance().Initialize("demo_gis_ops.log");

    if (argc < 2) {
        std::cout << "Usage: demo_gis_ops <input_tif>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];

    try {
        gis_ai::RasterIO raster_io;
        auto raster = raster_io.Load(input_path);

        std::cout << "Loaded raster: " << raster->width << "x" << raster->height
                  << " bands=" << raster->band_count << std::endl;

        gis_ai::RasterResample resample;
        auto resampled = resample.Execute(*raster, 256, 256, gis_ai::ResampleMethod::Bilinear);
        std::cout << "Resampled to: " << resampled->width << "x" << resampled->height << std::endl;

        gis_ai::RasterNormalize normalize;
        auto normalized = normalize.Execute(*raster);
        std::cout << "Normalized first pixel: " << normalized->bands[0][0] << std::endl;

        gis_ai::RasterClip clip;
        auto clipped = clip.ExecuteByPixel(*raster, 0, 0, 100, 100);
        std::cout << "Clipped to: " << clipped->width << "x" << clipped->height << std::endl;

        raster_io.Save(*resampled, "output_resampled.tif");
        raster_io.Save(*normalized, "output_normalized.tif");
        raster_io.Save(*clipped, "output_clipped.tif");

        std::cout << "Results saved." << std::endl;
    } catch (const gis_ai::GisAiIOException& e) {
        std::cerr << "IO Error: " << e.what() << std::endl;
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
