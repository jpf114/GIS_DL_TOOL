#include "io/pointcloud_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <gdal_priv.h>
#include <cpl_conv.h>

namespace gis_ai {

std::unique_ptr<PointCloudData> PointCloudIO::Load(const std::string& path) {
	GDALDataset* dataset = static_cast<GDALDataset*>(
		GDALOpen(path.c_str(), GA_ReadOnly));
	if (!dataset) {
		throw GisAiIOException("Failed to open point cloud file: " + path,
			"PointCloudIO::Load");
	}

	auto data = std::make_unique<PointCloudData>();

	OGRSpatialReference* srs = dataset->GetSpatialRef();
	if (srs) {
		char* wkt = nullptr;
		srs->exportToWkt(&wkt);
		if (wkt) {
			data->coordinate_system = wkt;
			CPLFree(wkt);
		}
	}

	GDALPointCloudLayer* pc_layer = dataset->GetLayer(0)
		? dynamic_cast<GDALPointCloudLayer*>(dataset->GetLayer(0))
		: nullptr;

	if (!pc_layer) {
		int feature_count = 0;
		for (int li = 0; li < dataset->GetLayerCount(); ++li) {
			OGRLayer* layer = dataset->GetLayer(li);
			if (!layer) continue;

			layer->ResetReading();
			while (OGRFeature* feat = layer->GetNextFeature()) {
				OGRGeometry* geom = feat->GetGeometryRef();
				if (geom && wkbFlatten(geom->getGeometryType()) == wkbPoint) {
					OGRPoint* pt = geom->toPoint();
					Point point;
					point.x = pt->getX();
					point.y = pt->getY();
					point.z = pt->getZ();

					int intensity_idx = feat->GetFieldIndex("intensity");
					if (intensity_idx >= 0 && feat->IsFieldSet(intensity_idx)) {
						point.intensity = static_cast<float>(feat->GetFieldAsDouble(intensity_idx));
					}

					int class_idx = feat->GetFieldIndex("classification");
					if (class_idx >= 0 && feat->IsFieldSet(class_idx)) {
						point.classification = static_cast<uint8_t>(feat->GetFieldAsInteger(class_idx));
					}

					data->points.push_back(point);
					++feature_count;
				}
				OGRFeature::DestroyFeature(feat);
			}
		}

		GDALClose(dataset);
		LOG_INFO("Loaded point cloud: " + path + " (" +
			std::to_string(feature_count) + " points via OGR layer fallback)");
		return data;
	}

	GIntBig point_count = pc_layer->GetFeatureCount(FALSE);
	data->points.reserve(static_cast<size_t>(point_count > 0 ? point_count : 0));

	pc_layer->ResetReading();
	while (OGRFeature* feat = pc_layer->GetNextFeature()) {
		OGRGeometry* geom = feat->GetGeometryRef();
		if (!geom || wkbFlatten(geom->getGeometryType()) != wkbPoint) {
			OGRFeature::DestroyFeature(feat);
			continue;
		}

		OGRPoint* pt = geom->toPoint();
		Point point;
		point.x = pt->getX();
		point.y = pt->getY();
		point.z = pt->getZ();

		int intensity_idx = feat->GetFieldIndex("intensity");
		if (intensity_idx >= 0 && feat->IsFieldSet(intensity_idx)) {
			point.intensity = static_cast<float>(feat->GetFieldAsDouble(intensity_idx));
		}

		int class_idx = feat->GetFieldIndex("classification");
		if (class_idx >= 0 && feat->IsFieldSet(class_idx)) {
			point.classification = static_cast<uint8_t>(feat->GetFieldAsInteger(class_idx));
		}

		data->points.push_back(point);
		OGRFeature::DestroyFeature(feat);
	}

	GDALClose(dataset);
	LOG_INFO("Loaded point cloud: " + path + " (" +
		std::to_string(data->points.size()) + " points)");
	return data;
}

void PointCloudIO::Save(const std::string& path, const PointCloudData& data) {
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("LAS");
	if (!driver) {
		driver = GetGDALDriverManager()->GetDriverByName("PDAL");
	}
	if (!driver) {
		throw GisAiIOException("LAS/PDAL driver not available for writing",
			"PointCloudIO::Save");
	}

	char** options = nullptr;
	GDALDataset* dataset = driver->Create(path.c_str(), 0, 0, 0, GDT_Unknown, options);
	if (!dataset) {
		CSLDestroy(options);
		throw GisAiIOException("Failed to create point cloud file: " + path,
			"PointCloudIO::Save");
	}

	OGRSpatialReference* srs = nullptr;
	if (!data.coordinate_system.empty()) {
		srs = new OGRSpatialReference(data.coordinate_system.c_str());
	}

	OGRwkbGeometryType geom_type = wkbPoint25D;
	OGRLayer* layer = dataset->CreateLayer("points", srs, geom_type, options);
	if (!layer) {
		GDALClose(dataset);
		if (srs) srs->Release();
		CSLDestroy(options);
		throw GisAiIOException("Failed to create layer in: " + path,
			"PointCloudIO::Save");
	}

	OGRFieldDefn intensity_field("intensity", OFTReal);
	intensity_field.SetWidth(10);
	intensity_field.SetPrecision(4);
	if (layer->CreateField(&intensity_field) != OGRERR_NONE) {
		LOG_WARN("Failed to create intensity field in: " + path);
	}

	OGRFieldDefn class_field("classification", OFTInteger);
	class_field.SetWidth(3);
	if (layer->CreateField(&class_field) != OGRERR_NONE) {
		LOG_WARN("Failed to create classification field in: " + path);
	}

	for (const auto& pt : data.points) {
		OGRFeature* feat = OGRFeature::CreateFeature(layer->GetLayerDefn());

		OGRPoint ogr_pt(pt.x, pt.y, pt.z);
		feat->SetGeometry(&ogr_pt);

		int intensity_idx = feat->GetFieldIndex("intensity");
		if (intensity_idx >= 0) {
			feat->SetField(intensity_idx, static_cast<double>(pt.intensity));
		}

		int class_idx = feat->GetFieldIndex("classification");
		if (class_idx >= 0) {
			feat->SetField(class_idx, static_cast<int>(pt.classification));
		}

		if (layer->CreateFeature(feat) != OGRERR_NONE) {
			OGRFeature::DestroyFeature(feat);
			GDALClose(dataset);
			if (srs) srs->Release();
			CSLDestroy(options);
			throw GisAiIOException("Failed to write point to: " + path,
				"PointCloudIO::Save");
		}
		OGRFeature::DestroyFeature(feat);
	}

	GDALClose(dataset);
	if (srs) srs->Release();
	CSLDestroy(options);
	LOG_INFO("Saved point cloud: " + path + " (" +
		std::to_string(data.points.size()) + " points)");
}

} // namespace gis_ai
