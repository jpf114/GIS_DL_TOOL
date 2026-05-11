#ifndef GIS_AI_RASTER_IO_H
#define GIS_AI_RASTER_IO_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API RasterDataType {
    Byte,
    UInt16,
    Int16,
    UInt32,
    Int32,
    Float32,
    Float64,
    Unknown
};

enum class GIS_AI_API RasterOutputFormat {
    GTiff,
    COG,
    Auto
};

struct GIS_AI_API BandInfo {
    RasterDataType data_type = RasterDataType::Float32;
    std::optional<float> nodata_value;
};

struct GIS_AI_API RasterData {
    int width = 0;
    int height = 0;
    int band_count = 0;
    double geotransform[6] = {0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    std::string projection;
    std::vector<std::vector<float>> bands;
    std::vector<BandInfo> band_infos;
};

class GIS_AI_API RasterIO {
public:
    RasterIO() = default;
    ~RasterIO() = default;

    std::unique_ptr<RasterData> Load(const std::string& path);

    void Save(const RasterData& data, const std::string& path,
              RasterOutputFormat format = RasterOutputFormat::Auto);

private:
    static constexpr int GEOTRANSFORM_SIZE = 6;
    static RasterDataType GdalTypeToRasterType(int gdal_type);
    static int RasterTypeToGdalType(RasterDataType type);
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_IO_H
