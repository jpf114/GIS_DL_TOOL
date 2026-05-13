#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <cpl_conv.h>
#include <cmath>
#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::create_directories("test_data/raster");
    std::filesystem::create_directories("test_data/vector");

    GDALAllRegister();

    auto* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver) { std::cerr << "No GTiff driver\n"; return 1; }

    char** create_opts = nullptr;
    auto* ds = driver->Create("test_data/raster/test_100x100.tif", 100, 100, 3, GDT_Float32, create_opts);
    if (!ds) { std::cerr << "Failed to create tif\n"; return 1; }

    double gt[6] = {116.0, 0.001, 0.0, 40.0, 0.0, -0.001};
    ds->SetGeoTransform(gt);
    OGRSpatialReference srs;
    srs.importFromEPSG(4326);
    char* wkt = nullptr;
    srs.exportToWkt(&wkt);
    ds->SetProjection(wkt);
    CPLFree(wkt);

    for (int b = 1; b <= 3; ++b) {
        auto* band = ds->GetRasterBand(b);
        float* data = new float[100 * 100];
        for (int i = 0; i < 100 * 100; ++i) {
            data[i] = static_cast<float>((i % 256) * b * 0.5f);
        }
        band->RasterIO(GF_Write, 0, 0, 100, 100, data, 100, 100, GDT_Float32, 0, 0);
        delete[] data;
    }
    GDALClose(ds);
    std::cout << "Created test GeoTIFF: test_data/raster/test_100x100.tif" << std::endl;

    auto* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (!shp_driver) { std::cerr << "No Shapefile driver\n"; return 1; }

    auto* shp_ds = shp_driver->Create("test_data/vector/test_polygons.shp", 0, 0, 0, GDT_Unknown, nullptr);
    if (!shp_ds) { std::cerr << "Failed to create shp\n"; return 1; }

    OGRLayer* layer = shp_ds->CreateLayer("polygons", &srs, wkbPolygon, nullptr);
    OGRFieldDefn name_field("name", OFTString);
    layer->CreateField(&name_field);

    double centers[][2] = {{116.05, 39.95}, {116.07, 39.97}};
    const char* names[] = {"poly_0", "poly_1"};

    for (int idx = 0; idx < 2; ++idx) {
        OGRFeature feat(layer->GetLayerDefn());
        feat.SetField("name", names[idx]);
        double cx = centers[idx][0], cy = centers[idx][1];
        double half = 0.02;
        OGRLinearRing ring;
        ring.addPoint(cx - half, cy - half);
        ring.addPoint(cx + half, cy - half);
        ring.addPoint(cx + half, cy + half);
        ring.addPoint(cx - half, cy + half);
        ring.addPoint(cx - half, cy - half);
        OGRPolygon poly;
        poly.addRing(&ring);
        feat.SetGeometry(&poly);
        layer->CreateFeature(&feat);
    }
    GDALClose(shp_ds);
    std::cout << "Created test Shapefile: test_data/vector/test_polygons.shp" << std::endl;

    auto* json_driver = GetGDALDriverManager()->GetDriverByName("GeoJSON");
    if (!json_driver) { std::cerr << "No GeoJSON driver\n"; return 1; }

    auto* json_ds = json_driver->Create("test_data/vector/test_points.geojson", 0, 0, 0, GDT_Unknown, nullptr);
    if (!json_ds) { std::cerr << "Failed to create geojson\n"; return 1; }

    OGRLayer* pt_layer = json_ds->CreateLayer("points", &srs, wkbPoint, nullptr);
    OGRFieldDefn val_field("value", OFTReal);
    pt_layer->CreateField(&val_field);

    for (int i = 0; i < 10; ++i) {
        OGRFeature feat(pt_layer->GetLayerDefn());
        feat.SetField("value", i * 10.0);
        OGRPoint pt(116.0 + i * 0.01, 39.9 + i * 0.005);
        feat.SetGeometry(&pt);
        pt_layer->CreateFeature(&feat);
    }
    GDALClose(json_ds);
    std::cout << "Created test GeoJSON: test_data/vector/test_points.geojson" << std::endl;

    std::cout << "\nAll test data created successfully!" << std::endl;
    return 0;
}
