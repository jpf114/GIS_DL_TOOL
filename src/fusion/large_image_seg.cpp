#include "fusion/large_image_seg.h"
#include "ai/mask_to_polygon.h"
#include "gis/vector_simplify.h"
#include "gis/vector_topology.h"
#include "gis/geos_utils.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <filesystem>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <numeric>

namespace gis_ai {

LargeImageSeg::LargeImageSeg(const std::string& model_path) {
    if (!std::filesystem::exists(model_path)) {
        throw GisAiModelException("Model file not found: " + model_path, "LargeImageSeg::LargeImageSeg");
    }

    model_name_ = std::filesystem::path(model_path).stem().string();
    model_manager_.LoadModel(model_path, model_name_);
    engine_ = std::make_unique<InferenceEngine>(model_manager_);
}

LargeImageSeg::~LargeImageSeg() = default;

void LargeImageSeg::SetProgressCallback(std::function<void(int, int, const std::string&)> callback) {
    progress_callback_ = std::move(callback);
}

std::vector<LargeImageSeg::TileInfo> LargeImageSeg::ComputeTiles(int img_w, int img_h, int tile_size, int stride) const {
    std::vector<TileInfo> tiles;

    int y = 0;
    while (y < img_h) {
        int x = 0;
        int tile_h = std::min(tile_size, img_h - y);
        while (x < img_w) {
            int tile_w = std::min(tile_size, img_w - x);

            TileInfo tile;
            tile.src_x = x;
            tile.src_y = y;
            tile.src_w = tile_w;
            tile.src_h = tile_h;
            tile.dst_x = 0;
            tile.dst_y = 0;
            tile.dst_w = tile_w;
            tile.dst_h = tile_h;

            int overlap_left = 0;
            if (x > 0 && stride < tile_size) {
                overlap_left = std::min(stride, tile_w);
            }
            tile.overlap_left = overlap_left;

            tile.overlap_top = (y > 0) ? std::min(stride, tile_h) : 0;
            tile.overlap_right = (x + tile_size < img_w) ? std::min(stride, tile_w) : 0;
            tile.overlap_bottom = (y + tile_size < img_h) ? std::min(stride, tile_h) : 0;

            tiles.push_back(tile);
            x += stride;
            if (x + tile_size > img_w && x < img_w) {
                x = img_w - tile_size;
                if (x <= (static_cast<int>(tiles.size() > 0 ? tiles.back().src_x : 0))) break;
            }
        }
        y += stride;
        if (y + tile_size > img_h && y < img_h) {
            y = img_h - tile_size;
            if (y <= (static_cast<int>(tiles.size() > 1 ? tiles[tiles.size() - 2].src_y : 0))) break;
        }
    }

    return tiles;
}

std::vector<float> LargeImageSeg::ComputeBlendWeights(int tile_size, int overlap, BlendMode mode, float sigma) const {
    if (overlap <= 0) {
        return std::vector<float>(tile_size, 1.0f);
    }

    std::vector<float> weights(tile_size, 1.0f);

    if (mode == BlendMode::Linear) {
        for (int i = 0; i < overlap; ++i) {
            float w = static_cast<float>(i + 1) / static_cast<float>(overlap + 1);
            weights[i] = w;
            weights[tile_size - 1 - i] = w;
        }
    } else if (mode == BlendMode::Gaussian) {
        float sigma_val = sigma * static_cast<float>(overlap);
        if (sigma_val < 1.0f) sigma_val = 1.0f;
        for (int i = 0; i < overlap; ++i) {
            float dist = static_cast<float>(overlap - i);
            float w = 1.0f - std::exp(-(dist * dist) / (2.0f * sigma_val * sigma_val));
            weights[i] = std::min(weights[i], w);
            weights[tile_size - 1 - i] = std::min(weights[tile_size - 1 - i], w);
        }
    }

    return weights;
}

bool LargeImageSeg::IsTileAllNodata(const RasterData& raster, int x, int y, int w, int h, float nodata_val) const {
    for (int b = 0; b < raster.band_count; ++b) {
        for (int py = y; py < y + h && py < raster.height; ++py) {
            for (int px = x; px < x + w && px < raster.width; ++px) {
                float val = raster.bands[b][py * raster.width + px];
                if (!std::isnan(nodata_val)) {
                    if (std::isnan(val) || std::abs(val - nodata_val) < 1e-6f) continue;
                    return false;
                } else {
                    if (!std::isnan(val)) return false;
                }
            }
        }
    }
    return true;
}

std::unique_ptr<RasterData> LargeImageSeg::ExtractTile(const RasterData& raster, int x, int y, int w, int h) const {
    auto tile = std::make_unique<RasterData>();
    tile->width = w;
    tile->height = h;
    tile->band_count = raster.band_count;
    tile->projection = raster.projection;
    tile->band_infos = raster.band_infos;

    tile->geotransform[0] = raster.geotransform[0] + x * raster.geotransform[1] + y * raster.geotransform[2];
    tile->geotransform[1] = raster.geotransform[1];
    tile->geotransform[2] = raster.geotransform[2];
    tile->geotransform[3] = raster.geotransform[3] + x * raster.geotransform[4] + y * raster.geotransform[5];
    tile->geotransform[4] = raster.geotransform[4];
    tile->geotransform[5] = raster.geotransform[5];

    tile->bands.resize(raster.band_count);
    for (int b = 0; b < raster.band_count; ++b) {
        tile->bands[b].resize(static_cast<size_t>(w) * h);
        for (int py = 0; py < h; ++py) {
            int src_y = y + py;
            if (src_y < 0 || src_y >= raster.height) {
                std::fill_n(tile->bands[b].data() + py * w, w, 0.0f);
                continue;
            }
            for (int px = 0; px < w; ++px) {
                int src_x = x + px;
                if (src_x < 0 || src_x >= raster.width) {
                    tile->bands[b][py * w + px] = 0.0f;
                } else {
                    tile->bands[b][py * w + px] = raster.bands[b][src_y * raster.width + src_x];
                }
            }
        }
    }

    return tile;
}

void LargeImageSeg::BlendTileIntoResult(const std::vector<float>& tile_output,
                                          int64_t out_h, int64_t out_w, int64_t num_classes,
                                          const TileInfo& tile,
                                          const std::vector<float>& weights_h,
                                          const std::vector<float>& weights_w,
                                          std::vector<float>& accumulated,
                                          std::vector<float>& weight_sum,
                                          int mosaic_w, int mosaic_h) const {
    for (int64_t c = 0; c < num_classes; ++c) {
        for (int64_t py = 0; py < out_h; ++py) {
            int dst_y = tile.src_y + static_cast<int>(py);
            if (dst_y < 0 || dst_y >= mosaic_h) continue;

            float wy = weights_h[static_cast<size_t>(py)];

            for (int64_t px = 0; px < out_w; ++px) {
                int dst_x = tile.src_x + static_cast<int>(px);
                if (dst_x < 0 || dst_x >= mosaic_w) continue;

                float wx = weights_w[static_cast<size_t>(px)];
                float w = wy * wx;

                size_t src_idx = static_cast<size_t>(c * out_h * out_w + py * out_w + px);
                size_t dst_idx = static_cast<size_t>(c * mosaic_h * mosaic_w + dst_y * mosaic_w + dst_x);

                accumulated[dst_idx] += tile_output[src_idx] * w;
                weight_sum[dst_idx] += w;
            }
        }
    }
}

std::unique_ptr<RasterData> LargeImageSeg::Segment(const RasterData& input, const LargeImageSegConfig& config) {
    auto total_start = std::chrono::high_resolution_clock::now();

    last_stats_ = SegmentationStats{};

    int tile_size = config.tile_size;
    int stride = config.stride;
    if (stride > tile_size) stride = tile_size;
    if (stride <= 0) stride = tile_size;

    bool needs_tiling = (input.width > tile_size || input.height > tile_size);

    if (!needs_tiling) {
        Preprocess preprocess;
        PreprocessConfig pp_config;
        pp_config.target_width = tile_size;
        pp_config.target_height = tile_size;
        pp_config.target_channels = config.target_channels;
        pp_config.normalize_mode = config.normalize_mode;
        pp_config.input_is_uint8 = config.input_is_uint8;
        pp_config.mean_r = config.mean_r;
        pp_config.mean_g = config.mean_g;
        pp_config.mean_b = config.mean_b;
        pp_config.std_r = config.std_r;
        pp_config.std_g = config.std_g;
        pp_config.std_b = config.std_b;

        auto tensor = preprocess.RasterToTensor(input, pp_config);
        auto shape = Preprocess::GetInputShape(pp_config);

        auto result = engine_->Run(model_name_, tensor, shape);
        last_stats_.total_inference_time_ms = result.inference_time_ms;
        last_stats_.total_tiles = 1;
        last_stats_.inferred_tiles = 1;

        auto& output = result.outputs[0];
        auto& out_shape = result.shapes[0];
        int64_t num_classes = out_shape[1];
        int64_t out_h = out_shape[2];
        int64_t out_w = out_shape[3];

        Postprocess postprocess;
        auto sigmoid_output = postprocess.Sigmoid(output);

        size_t out_pixels = static_cast<size_t>(out_h) * out_w;
        std::vector<uint8_t> mask(out_pixels, 0);
        for (size_t i = 0; i < out_pixels; ++i) {
            float best_prob = -1.0f;
            uint8_t best_class = 0;
            for (int64_t c = 0; c < num_classes; ++c) {
                float prob = sigmoid_output[static_cast<size_t>(c) * out_pixels + i];
                if (prob > best_prob) {
                    best_prob = prob;
                    best_class = static_cast<uint8_t>(c);
                }
            }
            if (best_prob < config.mask_threshold) {
                best_class = 0;
            }
            mask[i] = best_class;
            if (best_class < 256) last_stats_.class_counts[best_class]++;
        }

        bool has_input_nodata = !input.band_infos.empty() && input.band_infos[0].nodata_value.has_value();
        if (has_input_nodata && out_w == input.width && out_h == input.height) {
            float input_nodata = input.band_infos[0].nodata_value.value();
            for (size_t i = 0; i < out_pixels; ++i) {
                bool is_nodata = false;
                for (int b = 0; b < input.band_count; ++b) {
                    float val = input.bands[b][i];
                    if (!std::isnan(input_nodata) && std::abs(val - input_nodata) < 1e-6f) {
                        is_nodata = true; break;
                    }
                    if (std::isnan(input_nodata) && std::isnan(val)) {
                        is_nodata = true; break;
                    }
                }
                if (is_nodata) mask[i] = 255;
            }
        }

        auto raster = std::make_unique<RasterData>();
        *raster = postprocess.MaskToRaster(mask, static_cast<int>(out_w), static_cast<int>(out_h),
            input.geotransform, input.projection);

        if (has_input_nodata && out_w == input.width && out_h == input.height) {
            raster->band_infos[0].nodata_value = 255.0f;
        }

        auto total_end = std::chrono::high_resolution_clock::now();
        last_stats_.total_time_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();
        return raster;
    }

    auto tiles = ComputeTiles(input.width, input.height, tile_size, stride);
    last_stats_.total_tiles = static_cast<int>(tiles.size());

    LOG_INFO("Large image tiling: " + std::to_string(input.width) + "x" + std::to_string(input.height) +
        " -> " + std::to_string(tiles.size()) + " tiles (tile=" + std::to_string(tile_size) +
        ", stride=" + std::to_string(stride) + ")");

    auto model_info = model_manager_.GetModel(model_name_);
    int64_t num_classes = 1;
    if (model_info && !model_info->output_shapes.empty()) {
        num_classes = model_info->output_shapes[0][1];
    }

    size_t mosaic_pixels = static_cast<size_t>(input.width) * input.height;
    std::vector<float> accumulated(static_cast<size_t>(num_classes) * mosaic_pixels, 0.0f);
    std::vector<float> weight_sum(static_cast<size_t>(num_classes) * mosaic_pixels, 0.0f);

    std::vector<float> weights_h = ComputeBlendWeights(tile_size, tile_size - stride, config.blend_mode, config.gaussian_sigma);
    std::vector<float> weights_w = ComputeBlendWeights(tile_size, tile_size - stride, config.blend_mode, config.gaussian_sigma);

    Preprocess preprocess;
    Postprocess postprocess;

    PreprocessConfig pp_config;
    pp_config.target_width = tile_size;
    pp_config.target_height = tile_size;
    pp_config.target_channels = config.target_channels;
    pp_config.normalize_mode = config.normalize_mode;
    pp_config.input_is_uint8 = config.input_is_uint8;
    pp_config.mean_r = config.mean_r;
    pp_config.mean_g = config.mean_g;
    pp_config.mean_b = config.mean_b;
    pp_config.std_r = config.std_r;
    pp_config.std_g = config.std_g;
    pp_config.std_b = config.std_b;

    int tile_idx = 0;
    for (const auto& tile : tiles) {
        tile_idx++;

        if (config.skip_nodata && IsTileAllNodata(input, tile.src_x, tile.src_y, tile.src_w, tile.src_h, config.nodata_value)) {
            last_stats_.skipped_tiles++;
            if (progress_callback_) {
                progress_callback_(tile_idx, last_stats_.total_tiles, "skipped (nodata)");
            }
            continue;
        }

        auto tile_raster = ExtractTile(input, tile.src_x, tile.src_y, tile.src_w, tile.src_h);

        auto tensor = preprocess.RasterToTensor(*tile_raster, pp_config);
        auto shape = Preprocess::GetInputShape(pp_config);

        auto result = engine_->Run(model_name_, tensor, shape);
        last_stats_.total_inference_time_ms += result.inference_time_ms;
        last_stats_.inferred_tiles++;

        auto& output = result.outputs[0];
        auto& out_shape = result.shapes[0];
        int64_t out_nc = out_shape[1];
        int64_t out_h = out_shape[2];
        int64_t out_w = out_shape[3];

        auto sigmoid_output = postprocess.Sigmoid(output);

        if (config.blend_mode == BlendMode::None) {
            for (int64_t py = 0; py < out_h; ++py) {
                int dst_y = tile.src_y + static_cast<int>(py);
                if (dst_y < 0 || dst_y >= input.height) continue;
                for (int64_t px = 0; px < out_w; ++px) {
                    int dst_x = tile.src_x + static_cast<int>(px);
                    if (dst_x < 0 || dst_x >= input.width) continue;
                    size_t src_idx = static_cast<size_t>(py * out_w + px);
                    for (int64_t c = 0; c < out_nc; ++c) {
                        size_t sig_idx = static_cast<size_t>(c) * out_h * out_w + src_idx;
                        size_t dst_idx = static_cast<size_t>(c) * mosaic_pixels + dst_y * input.width + dst_x;
                        accumulated[dst_idx] = sigmoid_output[sig_idx];
                        weight_sum[dst_idx] = 1.0f;
                    }
                }
            }
        } else {
            BlendTileIntoResult(sigmoid_output, out_h, out_w, out_nc, tile,
                weights_h, weights_w, accumulated, weight_sum,
                input.width, input.height);
        }

        if (progress_callback_) {
            progress_callback_(tile_idx, last_stats_.total_tiles,
                "tile (" + std::to_string(tile.src_x) + "," + std::to_string(tile.src_y) + ")");
        }
    }

    std::vector<uint8_t> final_mask(mosaic_pixels, 0);
    for (size_t i = 0; i < mosaic_pixels; ++i) {
        float best_score = -1.0f;
        uint8_t best_class = 0;
        for (int64_t c = 0; c < num_classes; ++c) {
            size_t idx = static_cast<size_t>(c) * mosaic_pixels + i;
            float score = (weight_sum[idx] > 0.0f) ? accumulated[idx] / weight_sum[idx] : 0.0f;
            if (score > best_score) {
                best_score = score;
                best_class = static_cast<uint8_t>(c);
            }
        }
        if (best_score < config.mask_threshold) {
            best_class = 0;
        }
        final_mask[i] = best_class;
        if (best_class < 256) last_stats_.class_counts[best_class]++;
    }

    bool has_input_nodata = !input.band_infos.empty() && input.band_infos[0].nodata_value.has_value();
    if (has_input_nodata) {
        float input_nodata = input.band_infos[0].nodata_value.value();
        for (size_t i = 0; i < mosaic_pixels; ++i) {
            bool is_nodata = false;
            for (int b = 0; b < input.band_count; ++b) {
                float val = input.bands[b][i];
                if (!std::isnan(input_nodata) && std::abs(val - input_nodata) < 1e-6f) {
                    is_nodata = true; break;
                }
                if (std::isnan(input_nodata) && std::isnan(val)) {
                    is_nodata = true; break;
                }
            }
            if (is_nodata) final_mask[i] = 255;
        }
    }

    auto result_raster = std::make_unique<RasterData>();
    *result_raster = postprocess.MaskToRaster(final_mask, input.width, input.height,
        input.geotransform, input.projection);

    if (has_input_nodata) {
        result_raster->band_infos[0].nodata_value = 255.0f;
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    last_stats_.total_time_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();

    LOG_INFO("Large image segmentation completed: " + std::to_string(last_stats_.inferred_tiles) + "/" +
        std::to_string(last_stats_.total_tiles) + " tiles inferred, " +
        std::to_string(last_stats_.skipped_tiles) + " skipped, inference=" +
        std::to_string(last_stats_.total_inference_time_ms) + "ms, total=" +
        std::to_string(last_stats_.total_time_ms) + "ms");

    return result_raster;
}

std::unique_ptr<VectorData> LargeImageSeg::FilterSmallPolygons(std::unique_ptr<VectorData> polygons, double min_area) const {
    if (min_area <= 0.0 || polygons->features.empty()) return polygons;

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) return polygons;
    GEOSContextHandle_t ctx = geos_ctx.Get();

    auto filtered = std::make_unique<VectorData>();
    filtered->feature_type = polygons->feature_type;
    filtered->projection = polygons->projection;

    int removed = 0;
    for (const auto& feat : polygons->features) {
        if (feat.type != FeatureType::Polygon) {
            filtered->features.push_back(feat);
            continue;
        }

        GEOSGeometry* geom = geos_utils::FeatureToGeos(ctx, feat);
        if (!geom) continue;

        double area = 0.0;
        GEOSArea_r(ctx, geom, &area);
        GEOSGeom_destroy_r(ctx, geom);

        if (area >= min_area) {
            filtered->features.push_back(feat);
        } else {
            removed++;
        }
    }

    LOG_INFO("Small polygon filter: removed " + std::to_string(removed) +
        " polygons below area " + std::to_string(min_area));
    return filtered;
}

std::unique_ptr<VectorData> LargeImageSeg::SimplifyAndFix(std::unique_ptr<VectorData> polygons, double tolerance, bool fix_topology) const {
    if (!polygons || polygons->features.empty()) return polygons;

    if (tolerance > 0.0 && polygons->feature_type == FeatureType::Polygon) {
        VectorSimplify simplifier;
        polygons = simplifier.Execute(*polygons, tolerance);
    }

    if (fix_topology && polygons->feature_type == FeatureType::Polygon) {
        geos_utils::GeosContext geos_ctx;
        if (geos_ctx.Get()) {
            GEOSContextHandle_t ctx = geos_ctx.Get();

            auto fixed = std::make_unique<VectorData>();
            fixed->feature_type = polygons->feature_type;
            fixed->projection = polygons->projection;

            for (auto& feat : polygons->features) {
                GEOSGeometry* geom = geos_utils::FeatureToGeos(ctx, feat);
                if (!geom) {
                    fixed->features.push_back(std::move(feat));
                    continue;
                }

                if (GEOSisValid_r(ctx, geom) != 1) {
                    GEOSGeometry* repaired = GEOSBuffer_r(ctx, geom, 0.0, 8);
                    if (repaired && GEOSisValid_r(ctx, repaired) == 1) {
                        auto fixed_feat = geos_utils::GeosToFeature(ctx, repaired, feat.type);
                        fixed_feat->attributes = feat.attributes;
                        fixed->features.push_back(std::move(*fixed_feat));
                        GEOSGeom_destroy_r(ctx, repaired);
                    } else {
                        fixed->features.push_back(std::move(feat));
                        if (repaired) GEOSGeom_destroy_r(ctx, repaired);
                    }
                } else {
                    fixed->features.push_back(std::move(feat));
                }
                GEOSGeom_destroy_r(ctx, geom);
            }
            polygons = std::move(fixed);
        }
    }

    return polygons;
}

std::unique_ptr<VectorData> LargeImageSeg::AddAttributes(std::unique_ptr<VectorData> polygons, const RasterData& source_raster, uint8_t target_class) {
    double pixel_area = std::abs(source_raster.geotransform[1] * source_raster.geotransform[5]);

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) return polygons;
    GEOSContextHandle_t ctx = geos_ctx.Get();

    double total_area = 0.0;
    int idx = 0;
    for (auto& feat : polygons->features) {
        feat.attributes["class_id"] = static_cast<int>(target_class);
        feat.attributes["feature_id"] = idx;

        if (feat.type == FeatureType::Polygon) {
            GEOSGeometry* geom = geos_utils::FeatureToGeos(ctx, feat);
            if (geom) {
                double area = 0.0;
                GEOSArea_r(ctx, geom, &area);
                feat.attributes["area"] = area;
                feat.attributes["area_px"] = (pixel_area > 0) ? area / pixel_area : 0.0;
                total_area += area;
                GEOSGeom_destroy_r(ctx, geom);
            }
        }

        feat.attributes["source"] = std::string("gis_ai_seg");
        idx++;
    }

    last_stats_.polygon_count = static_cast<int>(polygons->features.size());
    last_stats_.total_polygon_area = total_area;

    return polygons;
}

std::unique_ptr<VectorData> LargeImageSeg::SegmentToPolygon(const RasterData& input, const LargeImageSegConfig& config) {
    auto mask_raster = Segment(input, config);

    MaskToPolygon m2p;
    auto polygons = m2p.ExecuteFromRaster(*mask_raster, config.target_class);

    polygons = FilterSmallPolygons(std::move(polygons), config.min_polygon_area);
    polygons = SimplifyAndFix(std::move(polygons), config.simplify_tolerance, config.fix_topology);
    polygons = AddAttributes(std::move(polygons), input, config.target_class);

    LOG_INFO("SegmentToPolygon completed: " + std::to_string(last_stats_.polygon_count) +
        " polygons, total area=" + std::to_string(last_stats_.total_polygon_area));
    return polygons;
}

std::unique_ptr<VectorData> LargeImageSeg::SegmentToPolygon(const RasterData& input,
                                                              const RasterData& mask_raster,
                                                              const LargeImageSegConfig& config) {
    MaskToPolygon m2p;
    auto polygons = m2p.ExecuteFromRaster(mask_raster, config.target_class);

    polygons = FilterSmallPolygons(std::move(polygons), config.min_polygon_area);
    polygons = SimplifyAndFix(std::move(polygons), config.simplify_tolerance, config.fix_topology);
    polygons = AddAttributes(std::move(polygons), input, config.target_class);

    LOG_INFO("SegmentToPolygon (from mask) completed: " + std::to_string(last_stats_.polygon_count) +
        " polygons, total area=" + std::to_string(last_stats_.total_polygon_area));
    return polygons;
}

int LargeImageSeg::SegmentToFile(const std::string& input_path,
                                   const std::string& output_tif,
                                   const std::string& output_shp,
                                   const LargeImageSegConfig& config) {
    RasterIO rio;
    auto raster_data = rio.Load(input_path);

    auto result_raster = Segment(*raster_data, config);
    rio.Save(*result_raster, output_tif);

    if (!output_shp.empty()) {
        auto vector_data = SegmentToPolygon(*raster_data, *result_raster, config);
        VectorIO vio;
        vio.Save(*vector_data, output_shp);
    }

    LOG_INFO("LargeImageSeg::SegmentToFile completed: " + input_path + " -> " + output_tif);
    return 0;
}

} // namespace gis_ai
