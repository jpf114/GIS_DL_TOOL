#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdlib>

#include "io/raster_io.h"
#include "io/vector_io.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_clip.h"
#include "gis/raster_threshold.h"
#include "gis/vector_buffer.h"
#include "gis/vector_simplify.h"
#include "gis/pc_filter.h"
#include "gis/pc_downsample.h"

using namespace gis_ai;

struct BenchResult {
    std::string name;
    double time_ms;
    size_t iterations;
    double avg_ms() const { return time_ms / iterations; }
};

template <typename Func>
BenchResult RunBench(const std::string& name, size_t iterations, Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    return {name, elapsed, iterations};
}

int main() {
    std::vector<BenchResult> results;

    std::cout << "=== GIS Algorithm Performance Benchmark ===" << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Creating test data..." << std::endl;
    std::cout.flush();

    RasterData raster;
    raster.width = 500;
    raster.height = 500;
    raster.band_count = 3;
    raster.geotransform[0] = 0.0; raster.geotransform[1] = 1.0; raster.geotransform[2] = 0.0;
    raster.geotransform[3] = 0.0; raster.geotransform[4] = 0.0; raster.geotransform[5] = -1.0;
    for (int b = 0; b < 3; ++b) {
        raster.bands.emplace_back(500 * 500, static_cast<float>(b + 1) * 10.0f);
    }
    std::cout << "Raster created." << std::endl;
    std::cout.flush();

    VectorData vector_data;
    vector_data.feature_type = FeatureType::Polygon;
    for (int i = 0; i < 50; ++i) {
        Feature f;
        f.type = FeatureType::Polygon;
        double x0 = static_cast<double>(i % 10);
        double y0 = static_cast<double>(i / 10);
        f.coordinates = {{x0, y0, 0}, {x0+1, y0, 0}, {x0+1, y0+1, 0}, {x0, y0+1, 0}, {x0, y0, 0}};
        vector_data.features.push_back(std::move(f));
    }
    std::cout << "Vector created." << std::endl;
    std::cout.flush();

    PointCloudData pc_data;
    pc_data.coordinate_system = "EPSG:4326";
    for (int i = 0; i < 50000; ++i) {
        Point p;
        p.x = static_cast<double>(rand()) / RAND_MAX * 100.0;
        p.y = static_cast<double>(rand()) / RAND_MAX * 100.0;
        p.z = static_cast<double>(rand()) / RAND_MAX * 50.0;
        pc_data.points.push_back(p);
    }
    std::cout << "PointCloud created." << std::endl;
    std::cout.flush();

    std::cout << "Running benchmarks..." << std::endl;
    std::cout.flush();

    try {
        results.push_back(RunBench("RasterResample_Bilinear(500x500->256x256)", 10, [&]() {
            RasterResample resample;
            resample.Execute(raster, 256, 256, ResampleMethod::Bilinear);
        }));
        std::cout << "  RasterResample_Bilinear done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("RasterResample_Nearest(500x500->256x256)", 10, [&]() {
            RasterResample resample;
            resample.Execute(raster, 256, 256, ResampleMethod::Nearest);
        }));
        std::cout << "  RasterResample_Nearest done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("RasterNormalize(500x500x3)", 10, [&]() {
            RasterNormalize norm;
            norm.Execute(raster);
        }));
        std::cout << "  RasterNormalize done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("RasterClip(500x500->250x250)", 10, [&]() {
            RasterClip clip;
            BoundingBox bbox{125.0, 375.0, -375.0, -125.0};
            clip.Execute(raster, bbox);
        }));
        std::cout << "  RasterClip done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("RasterThreshold(500x500)", 10, [&]() {
            RasterThreshold thresh;
            thresh.Execute(raster, 50.0);
        }));
        std::cout << "  RasterThreshold done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("VectorBuffer(50 polygons)", 10, [&]() {
            VectorBuffer buffer;
            buffer.Execute(vector_data, 1.0);
        }));
        std::cout << "  VectorBuffer done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("VectorSimplify(50 polygons)", 10, [&]() {
            VectorSimplify simplify;
            simplify.Execute(vector_data, 0.5);
        }));
        std::cout << "  VectorSimplify done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("PcFilter_PassThrough(50K points)", 10, [&]() {
            PcFilter filter;
            PassFilterParams params;
            params.axis = "z";
            params.min_val = 10.0;
            params.max_val = 40.0;
            filter.PassThrough(pc_data, params);
        }));
        std::cout << "  PcFilter_PassThrough done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("PcDownsample_VoxelGrid(50K points)", 10, [&]() {
            PcDownsample ds;
            ds.VoxelGrid(pc_data, 1.0);
        }));
        std::cout << "  PcDownsample_VoxelGrid done." << std::endl;
        std::cout.flush();

        results.push_back(RunBench("PcDownsample_Random(50K points)", 10, [&]() {
            PcDownsample ds;
            ds.RandomDownsample(pc_data, 0.1);
        }));
        std::cout << "  PcDownsample_Random done." << std::endl;
        std::cout.flush();

    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 2;
    }

    std::cout << std::endl;
    std::cout << std::left << std::setw(50) << "Benchmark"
              << std::right << std::setw(10) << "Iters"
              << std::setw(12) << "Total(ms)"
              << std::setw(12) << "Avg(ms)" << std::endl;
    std::cout << std::string(84, '-') << std::endl;

    for (const auto& r : results) {
        std::cout << std::left << std::setw(50) << r.name
                  << std::right << std::setw(10) << r.iterations
                  << std::setw(12) << r.time_ms
                  << std::setw(12) << r.avg_ms() << std::endl;
    }

    std::cout << std::endl << "=== Benchmark Complete ===" << std::endl;
    return 0;
}
