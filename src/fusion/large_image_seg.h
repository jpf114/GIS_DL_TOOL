#ifndef GIS_AI_LARGE_IMAGE_SEG_H
#define GIS_AI_LARGE_IMAGE_SEG_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "ai/model_manager.h"
#include "ai/inference_engine.h"
#include "ai/preprocess.h"
#include "ai/postprocess.h"
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API BlendMode {
    None,
    Linear,
    Gaussian
};

struct GIS_AI_API LargeImageSegConfig {
    int tile_size = 512;
    int stride = 384;
    int target_channels = 3;
    NormalizeMode normalize_mode = NormalizeMode::ImageNet;
    bool input_is_uint8 = true;
    float mean_r = 0.485f;
    float mean_g = 0.456f;
    float mean_b = 0.406f;
    float std_r = 0.229f;
    float std_g = 0.224f;
    float std_b = 0.225f;
    float mask_threshold = 0.5f;
    uint8_t target_class = 1;
    BlendMode blend_mode = BlendMode::Gaussian;
    float gaussian_sigma = 0.5f;
    bool skip_nodata = true;
    float nodata_value = 0.0f;
    double min_polygon_area = 100.0;
    double simplify_tolerance = 1.0;
    bool fix_topology = true;
    int inference_batch_size = 1;
};

struct GIS_AI_API SegmentationStats {
    int total_tiles = 0;
    int skipped_tiles = 0;
    int inferred_tiles = 0;
    double total_inference_time_ms = 0.0;
    double total_time_ms = 0.0;
    int polygon_count = 0;
    double total_polygon_area = 0.0;
    int class_counts[256] = {};
};

class GIS_AI_API LargeImageSeg {
public:
    explicit LargeImageSeg(const std::string& model_path);
    ~LargeImageSeg();

    LargeImageSeg(const LargeImageSeg&) = delete;
    LargeImageSeg& operator=(const LargeImageSeg&) = delete;

    std::unique_ptr<RasterData> Segment(const RasterData& input,
                                         const LargeImageSegConfig& config = LargeImageSegConfig());

    std::unique_ptr<VectorData> SegmentToPolygon(const RasterData& input,
                                                   const LargeImageSegConfig& config = LargeImageSegConfig());

    std::unique_ptr<VectorData> SegmentToPolygon(const RasterData& input,
                                                   const RasterData& mask_raster,
                                                   const LargeImageSegConfig& config = LargeImageSegConfig());

    int SegmentToFile(const std::string& input_path,
                       const std::string& output_tif,
                       const std::string& output_shp = "",
                       const LargeImageSegConfig& config = LargeImageSegConfig());

    SegmentationStats GetLastStats() const { return last_stats_; }

    void SetProgressCallback(std::function<void(int, int, const std::string&)> callback);

private:
    struct TileInfo {
        int src_x, src_y;
        int src_w, src_h;
        int dst_x, dst_y;
        int dst_w, dst_h;
        int overlap_left, overlap_top, overlap_right, overlap_bottom;
    };

    std::vector<TileInfo> ComputeTiles(int img_w, int img_h, int tile_size, int stride) const;

    std::vector<float> ComputeBlendWeights(int tile_size, int overlap, BlendMode mode, float sigma) const;

    bool IsTileAllNodata(const RasterData& raster, int x, int y, int w, int h, float nodata_val) const;

    std::unique_ptr<RasterData> ExtractTile(const RasterData& raster, int x, int y, int w, int h) const;

    void BlendTileIntoResult(const std::vector<float>& tile_output,
                              int64_t out_h, int64_t out_w, int64_t num_classes,
                              const TileInfo& tile,
                              const std::vector<float>& weights_h,
                              const std::vector<float>& weights_w,
                              std::vector<float>& accumulated,
                              std::vector<float>& weight_sum,
                              int mosaic_w, int mosaic_h) const;

    std::unique_ptr<VectorData> FilterSmallPolygons(std::unique_ptr<VectorData> polygons,
                                                      double min_area) const;

    std::unique_ptr<VectorData> SimplifyAndFix(std::unique_ptr<VectorData> polygons,
                                                 double tolerance, bool fix_topology) const;

    std::unique_ptr<VectorData> AddAttributes(std::unique_ptr<VectorData> polygons,
                                                const RasterData& source_raster,
                                                uint8_t target_class = 1);

    ModelManager model_manager_;
    std::unique_ptr<InferenceEngine> engine_;
    std::string model_name_;
    SegmentationStats last_stats_;
    std::function<void(int, int, const std::string&)> progress_callback_;
};

} // namespace gis_ai

#endif // GIS_AI_LARGE_IMAGE_SEG_H
