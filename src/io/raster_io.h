#ifndef GIS_AI_RASTER_IO_H
#define GIS_AI_RASTER_IO_H

#include <string>
#include <vector>
#include <memory>
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API RasterData {
	int width = 0;
	int height = 0;
	int band_count = 0;
	double geotransform[6] = {0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
	std::string projection;
	std::vector<std::vector<float>> bands;
};

class GIS_AI_API RasterIO {
public:
	RasterIO() = default;
	~RasterIO() = default;

	std::unique_ptr<RasterData> Load(const std::string& path);
	void Save(const RasterData& data, const std::string& path);

private:
	static constexpr int GEOTRANSFORM_SIZE = 6;
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_IO_H
