#include "io/raster_io.h"
#include "io/gdal_utils.h"
#include "core/logger.h"
#include "core/exception.h"

#include <gdal_priv.h>

namespace gis_ai {

RasterDataType RasterIO::GdalTypeToRasterType(int gdal_type) {
    switch (gdal_type) {
        case GDT_Byte:    return RasterDataType::Byte;
        case GDT_UInt16:  return RasterDataType::UInt16;
        case GDT_Int16:   return RasterDataType::Int16;
        case GDT_UInt32:  return RasterDataType::UInt32;
        case GDT_Int32:   return RasterDataType::Int32;
        case GDT_Float32: return RasterDataType::Float32;
        case GDT_Float64: return RasterDataType::Float64;
        default:          return RasterDataType::Unknown;
    }
}

int RasterIO::RasterTypeToGdalType(RasterDataType type) {
    switch (type) {
        case RasterDataType::Byte:    return GDT_Byte;
        case RasterDataType::UInt16:  return GDT_UInt16;
        case RasterDataType::Int16:   return GDT_Int16;
        case RasterDataType::UInt32:  return GDT_UInt32;
        case RasterDataType::Int32:   return GDT_Int32;
        case RasterDataType::Float32: return GDT_Float32;
        case RasterDataType::Float64: return GDT_Float64;
        default:                      return GDT_Float32;
    }
}

std::unique_ptr<RasterData> RasterIO::Load(const std::string& path) {
    EnsureGDALInitialized();
    auto dataset = MakeGdalDataset(
        static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly)));
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
    data->band_infos.resize(data->band_count);
    for (int i = 0; i < data->band_count; ++i) {
        GDALRasterBand* band = dataset->GetRasterBand(i + 1);

        data->band_infos[i].data_type = GdalTypeToRasterType(band->GetRasterDataType());

        int nodata_ok = 0;
        double nodata_val = band->GetNoDataValue(&nodata_ok);
        if (nodata_ok) {
            data->band_infos[i].nodata_value = static_cast<float>(nodata_val);
        }

        data->bands[i].resize(static_cast<size_t>(data->width) * data->height);

        CPLErr err = band->RasterIO(GF_Read, 0, 0, data->width, data->height,
            data->bands[i].data(), data->width, data->height, GDT_Float32, 0, 0);
        if (err != CE_None) {
            throw GisAiIOException("Failed to read band " + std::to_string(i + 1) +
                " from: " + path, "RasterIO::Load");
        }
    }

    LOG_INFO("Loaded raster: " + path + " (" + std::to_string(data->width) + "x" +
        std::to_string(data->height) + ", " + std::to_string(data->band_count) + " bands)");
    return data;
}

void RasterIO::Save(const RasterData& data, const std::string& path, RasterOutputFormat format) {
    EnsureGDALInitialized();

    bool use_cog = false;
    if (format == RasterOutputFormat::COG) {
        use_cog = true;
    } else if (format == RasterOutputFormat::Auto) {
        std::string ext = GetExtensionLower(path);
        if (ext == ".cog" || ext == ".tif.cog") {
            use_cog = true;
        }
    }

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver) {
        throw GisAiIOException("GTiff driver not available", "RasterIO::Save");
    }

    GDALDataType output_type = GDT_Float32;
    if (!data.band_infos.empty() && data.band_infos[0].data_type != RasterDataType::Unknown) {
        output_type = static_cast<GDALDataType>(RasterTypeToGdalType(data.band_infos[0].data_type));
    }

    char** create_options = nullptr;
    if (use_cog) {
        create_options = CSLSetNameValue(create_options, "COPY_SRC_OVERVIEWS", "YES");
        create_options = CSLSetNameValue(create_options, "COMPRESS", "LZW");
        create_options = CSLSetNameValue(create_options, "TILED", "YES");
        create_options = CSLSetNameValue(create_options, "BLOCKXSIZE", "512");
        create_options = CSLSetNameValue(create_options, "BLOCKYSIZE", "512");
        create_options = CSLSetNameValue(create_options, "BIGTIFF", "IF_SAFER");
    } else {
        create_options = CSLSetNameValue(create_options, "TILED", "YES");
        create_options = CSLSetNameValue(create_options, "BLOCKXSIZE", "256");
        create_options = CSLSetNameValue(create_options, "BLOCKYSIZE", "256");
        create_options = CSLSetNameValue(create_options, "COMPRESS", "LZW");
        create_options = CSLSetNameValue(create_options, "BIGTIFF", "IF_SAFER");
    }

    auto dataset = MakeGdalDataset(
        driver->Create(path.c_str(), data.width, data.height,
            data.band_count, output_type, create_options));
    CSLDestroy(create_options);

    if (!dataset) {
        throw GisAiIOException("Failed to create raster file: " + path, "RasterIO::Save");
    }

    if (dataset->SetGeoTransform(const_cast<double*>(data.geotransform)) != CE_None) {
        throw GisAiIOException("Failed to set geotransform for: " + path, "RasterIO::Save");
    }

    if (!data.projection.empty()) {
        if (dataset->SetProjection(data.projection.c_str()) != CE_None) {
            throw GisAiIOException("Failed to set projection for: " + path, "RasterIO::Save");
        }
    }

    for (int i = 0; i < data.band_count; ++i) {
        GDALRasterBand* band = dataset->GetRasterBand(i + 1);

        if (i < static_cast<int>(data.band_infos.size()) && data.band_infos[i].nodata_value.has_value()) {
            band->SetNoDataValue(static_cast<double>(data.band_infos[i].nodata_value.value()));
        }

        CPLErr err = band->RasterIO(GF_Write, 0, 0, data.width, data.height,
            const_cast<float*>(data.bands[i].data()), data.width, data.height,
            GDT_Float32, 0, 0);
        if (err != CE_None) {
            throw GisAiIOException("Failed to write band " + std::to_string(i + 1) +
                " to: " + path, "RasterIO::Save");
        }
    }

    if (use_cog) {
        dataset->FlushCache();
        int overview_levels = 0;
        int max_dim = std::max(data.width, data.height);
        while (max_dim > 512) {
            max_dim /= 2;
            overview_levels++;
        }
        if (overview_levels > 0) {
            for (int i = 0; i < data.band_count; ++i) {
                GDALRasterBand* band = dataset->GetRasterBand(i + 1);
                band->GetOverviewCount();
            }
        }
    }

    std::string format_str = use_cog ? "COG" : "GTiff";
    LOG_INFO("Saved raster: " + path + " (" + std::to_string(data.width) + "x" +
        std::to_string(data.height) + ", " + std::to_string(data.band_count) +
        " bands, format=" + format_str + ")");
}

} // namespace gis_ai
