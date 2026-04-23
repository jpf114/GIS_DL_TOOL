#include "io/pointcloud_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <ogrsf_frmts.h>
#include <gdal_priv.h>

namespace gis_ai {

static void EnsureGDALInitialized() {
    static bool initialized = false;
    if (!initialized) {
        GDALAllRegister();
        initialized = true;
    }
}

std::unique_ptr<PointCloudData> PointCloudIO::Load(const std::string& path) {
    EnsureGDALInitialized();
	GDALDataset* dataset = reinterpret_cast<GDALDataset*>(
		OGROpen(path.c_str(), FALSE, nullptr));
	if (!dataset) {
		throw GisAiIOException("Failed to open point cloud file: " + path, "PointCloudIO::Load");
	}

	auto data = std::make_unique<PointCloudData>();

	OGRLayer* layer = dataset->GetLayer(0);
	if (!layer) {
		GDALClose(dataset);
		throw GisAiIOException("No layers found in: " + path, "PointCloudIO::Load");
	}

	const OGRSpatialReference* srs = layer->GetSpatialRef();
	if (srs) {
		char* wkt = nullptr;
		const_cast<OGRSpatialReference*>(srs)->exportToWkt(&wkt);
		if (wkt) {
			data->coordinate_system = wkt;
			CPLFree(wkt);
		}
	}

	layer->ResetReading();
	while (OGRFeature* ogr_feat = layer->GetNextFeature()) {
		OGRGeometry* geom = ogr_feat->GetGeometryRef();
		if (!geom) {
			OGRFeature::DestroyFeature(ogr_feat);
			continue;
		}

		OGRwkbGeometryType flat_type = wkbFlatten(geom->getGeometryType());
		if (flat_type == wkbPoint) {
			OGRPoint* pt = geom->toPoint();
			Point p;
			p.x = pt->getX();
			p.y = pt->getY();
			p.z = pt->getZ();

			int idx = ogr_feat->GetFieldIndex("intensity");
			if (idx >= 0 && ogr_feat->IsFieldSet(idx)) {
				p.intensity = static_cast<float>(ogr_feat->GetFieldAsDouble(idx));
			}
			idx = ogr_feat->GetFieldIndex("classification");
			if (idx >= 0 && ogr_feat->IsFieldSet(idx)) {
				p.classification = static_cast<uint8_t>(ogr_feat->GetFieldAsInteger(idx));
			}

			data->points.push_back(p);
		} else if (flat_type == wkbMultiPoint) {
			OGRMultiPoint* mp = geom->toMultiPoint();
			for (int i = 0; i < mp->getNumGeometries(); ++i) {
				const OGRPoint* pt = static_cast<const OGRPoint*>(mp->getGeometryRef(i));
				Point p;
				p.x = pt->getX();
				p.y = pt->getY();
				p.z = pt->getZ();
				data->points.push_back(p);
			}
		}

		OGRFeature::DestroyFeature(ogr_feat);
	}

	GDALClose(dataset);
	LOG_INFO("Loaded point cloud: " + path + " (" + std::to_string(data->points.size()) + " points)");
	return data;
}

void PointCloudIO::Save(const PointCloudData& data, const std::string& path) {
    EnsureGDALInitialized();
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GeoJSON");
	if (!driver) {
		driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
	}
	if (!driver) {
		throw GisAiIOException("No suitable driver for point cloud output", "PointCloudIO::Save");
	}

	OGRSpatialReference* srs = nullptr;
	if (!data.coordinate_system.empty()) {
		srs = new OGRSpatialReference(data.coordinate_system.c_str());
	}

	GDALDataset* dataset = driver->Create(path.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
	if (!dataset) {
		if (srs) srs->Release();
		throw GisAiIOException("Failed to create point cloud file: " + path, "PointCloudIO::Save");
	}

	OGRLayer* layer = dataset->CreateLayer("points", srs, wkbPoint, nullptr);
	if (!layer) {
		GDALClose(dataset);
		if (srs) srs->Release();
		throw GisAiIOException("Failed to create layer in: " + path, "PointCloudIO::Save");
	}

	OGRFieldDefn intensity_field("intensity", OFTReal);
	layer->CreateField(&intensity_field);

	OGRFieldDefn class_field("classification", OFTInteger);
	layer->CreateField(&class_field);

	for (const auto& pt : data.points) {
		OGRFeature* ogr_feat = OGRFeature::CreateFeature(layer->GetLayerDefn());
		OGRPoint ogr_pt(pt.x, pt.y, pt.z);
		ogr_feat->SetGeometry(&ogr_pt);

		int idx = ogr_feat->GetFieldIndex("intensity");
		if (idx >= 0) ogr_feat->SetField(idx, static_cast<double>(pt.intensity));

		idx = ogr_feat->GetFieldIndex("classification");
		if (idx >= 0) ogr_feat->SetField(idx, static_cast<int>(pt.classification));

		if (layer->CreateFeature(ogr_feat) != OGRERR_NONE) {
			OGRFeature::DestroyFeature(ogr_feat);
			GDALClose(dataset);
			if (srs) srs->Release();
			throw GisAiIOException("Failed to write point to: " + path, "PointCloudIO::Save");
		}
		OGRFeature::DestroyFeature(ogr_feat);
	}

	GDALClose(dataset);
	if (srs) srs->Release();
	LOG_INFO("Saved point cloud: " + path + " (" + std::to_string(data.points.size()) + " points)");
}

} // namespace gis_ai
