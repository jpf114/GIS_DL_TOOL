#include "io/raster_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <gdal_priv.h>
#include <cpl_conv.h>

namespace gis_ai {

std::unique_ptr<RasterData> RasterIO::Load(const std::string& path) {
	GDALDataset* dataset = static_cast<GDALDataset*>(
		GDALOpen(path.c_str(), GA_ReadOnly));
	if (!dataset) {
		throw GisAiIOException("Failed to open raster file: " + path, "RasterIO::Load");
	}

	auto data = std::make_unique<RasterData>();
	data->width = dataset->GetRasterXSize();
	data->height = dataset->GetRasterYSize();
	data->band_count = dataset->GetRasterCount();

	if (dataset->GetGeoTransform(data->geotransform) != CE_None) {
		LOG_WARN("No geotransform found in: " + path);
	}

	const char* proj = dataset->GetProjectionRef();
	if (proj && proj[0] != '\0') {
		data->projection = proj;
	}

	data->bands.resize(data->band_count);
	for (int i = 0; i < data->band_count; ++i) {
		GDALRasterBand* band = dataset->GetRasterBand(i + 1);
		data->bands[i].resize(static_cast<size_t>(data->width) * data->height);

		CPLErr err = band->RasterIO(GF_Read, 0, 0, data->width, data->height,
			data->bands[i].data(), data->width, data->height, GDT_Float32, 0, 0);
		if (err != CE_None) {
			GDALClose(dataset);
			throw GisAiIOException("Failed to read band " + std::to_string(i + 1) +
				" from: " + path, "RasterIO::Load");
		}
	}

	GDALClose(dataset);
	LOG_INFO("Loaded raster: " + path + " (" + std::to_string(data->width) + "x" +
		std::to_string(data->height) + ", " + std::to_string(data->band_count) + " bands)");
	return data;
}

void RasterIO::Save(const std::string& path, const RasterData& data) {
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (!driver) {
		throw GisAiIOException("GTiff driver not available", "RasterIO::Save");
	}

	GDALDataset* dataset = driver->Create(path.c_str(), data.width, data.height,
		data.band_count, GDT_Float32, nullptr);
	if (!dataset) {
		throw GisAiIOException("Failed to create raster file: " + path, "RasterIO::Save");
	}

	if (dataset->SetGeoTransform(const_cast<double*>(data.geotransform)) != CE_None) {
		GDALClose(dataset);
		throw GisAiIOException("Failed to set geotransform for: " + path, "RasterIO::Save");
	}

	if (!data.projection.empty()) {
		if (dataset->SetProjection(data.projection.c_str()) != CE_None) {
			GDALClose(dataset);
			throw GisAiIOException("Failed to set projection for: " + path, "RasterIO::Save");
		}
	}

	for (int i = 0; i < data.band_count; ++i) {
		GDALRasterBand* band = dataset->GetRasterBand(i + 1);
		CPLErr err = band->RasterIO(GF_Write, 0, 0, data.width, data.height,
			const_cast<float*>(data.bands[i].data()), data.width, data.height,
			GDT_Float32, 0, 0);
		if (err != CE_None) {
			GDALClose(dataset);
			throw GisAiIOException("Failed to write band " + std::to_string(i + 1) +
				" to: " + path, "RasterIO::Save");
		}
	}

	GDALClose(dataset);
	LOG_INFO("Saved raster: " + path + " (" + std::to_string(data.width) + "x" +
		std::to_string(data.height) + ", " + std::to_string(data.band_count) + " bands)");
}

} // namespace gis_ai
