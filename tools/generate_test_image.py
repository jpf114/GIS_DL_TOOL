"""
Generate a synthetic remote sensing test image for GIS_DL_TOOL.

Creates a GeoTIFF with realistic spatial patterns (buildings, roads, water,
vegetation) that can be used to test the segmentation pipeline end-to-end.

Usage:
  conda run -n GIS python tools/generate_test_image.py
"""

import sys
import os
import math
import random

try:
    import numpy as np
except ImportError:
    print("numpy not found.")
    sys.exit(1)

try:
    from osgeo import gdal, osr
except ImportError:
    print("GDAL not found. Install with: conda install gdal -c conda-forge")
    sys.exit(1)


def generate_test_image(output_path, width=1024, height=1024):
    gdal.AllRegister()

    random.seed(42)
    np.random.seed(42)

    bands = np.zeros((3, height, width), dtype=np.uint8)

    bands[0, :, :] = 120
    bands[1, :, :] = 140
    bands[2, :, :] = 100

    for _ in range(25):
        bx = random.randint(50, width - 150)
        by = random.randint(50, height - 150)
        bw = random.randint(30, 100)
        bh = random.randint(30, 80)
        r = random.randint(160, 220)
        g = random.randint(155, 200)
        b = random.randint(150, 190)
        bands[0, by:by+bh, bx:bx+bw] = r
        bands[1, by:by+bh, bx:bx+bw] = g
        bands[2, by:by+bh, bx:bx+bw] = b

    for i in range(0, height, random.randint(80, 200)):
        road_w = random.randint(4, 10)
        r_val = random.randint(80, 110)
        bands[0, i:i+road_w, :] = r_val
        bands[1, i:i+road_w, :] = r_val + 5
        bands[2, i:i+road_w, :] = r_val - 5

    for j in range(0, width, random.randint(80, 200)):
        road_w = random.randint(4, 10)
        r_val = random.randint(80, 110)
        bands[0, :, j:j+road_w] = r_val
        bands[1, :, j:j+road_w] = r_val + 5
        bands[2, :, j:j+road_w] = r_val - 5

    cx, cy = width // 4, height // 4
    rx, ry = width // 8, height // 8
    for y in range(max(0, cy-ry), min(height, cy+ry)):
        for x in range(max(0, cx-rx), min(width, cx+rx)):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
                bands[0, y, x] = 30
                bands[1, y, x] = 60
                bands[2, y, x] = 140

    cx2, cy2 = 3 * width // 4, 3 * height // 4
    rx2, ry2 = width // 10, height // 10
    for y in range(max(0, cy2-ry2), min(height, cy2+ry2)):
        for x in range(max(0, cx2-rx2), min(width, cx2+rx2)):
            if ((x - cx2) / rx2) ** 2 + ((y - cy2) / ry2) ** 2 <= 1.0:
                bands[0, y, x] = 25
                bands[1, y, x] = 55
                bands[2, y, x] = 135

    for _ in range(15):
        vx = random.randint(0, width - 60)
        vy = random.randint(0, height - 60)
        vw = random.randint(20, 60)
        vh = random.randint(20, 60)
        bands[0, vy:vy+vh, vx:vx+vw] = random.randint(20, 50)
        bands[1, vy:vy+vh, vx:vx+vw] = random.randint(80, 130)
        bands[2, vy:vy+vh, vx:vx+vw] = random.randint(20, 50)

    noise = np.random.randint(-8, 9, bands.shape).astype(np.int16)
    bands = np.clip(bands.astype(np.int16) + noise, 0, 255).astype(np.uint8)

    driver = gdal.GetDriverByName('GTiff')
    ds = driver.Create(output_path, width, height, 3, gdal.GDT_Byte,
                       options=['TILED=YES', 'COMPRESS=LZW', 'BIGTIFF=IF_NEEDED'])

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(32650)
    ds.SetProjection(srs.ExportToWkt())

    pixel_size = 0.5
    origin_x = 500000.0
    origin_y = 4000000.0
    ds.SetGeoTransform([origin_x, pixel_size, 0, origin_y, 0, -pixel_size])

    for i in range(3):
        ds.GetRasterBand(i + 1).WriteArray(bands[i])
        ds.GetRasterBand(i + 1).SetNoDataValue(0)

    ds.FlushCache()
    ds = None

    print(f"Test image generated: {output_path}")
    print(f"  Size: {width} x {height}, 3 bands, EPSG:32650")
    print(f"  Pixel size: {pixel_size}m")
    print(f"  Contains: buildings, roads, water bodies, vegetation")

    return output_path


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    output_dir = os.path.join(project_root, "test_data", "real")
    os.makedirs(output_dir, exist_ok=True)

    output_path = os.path.join(output_dir, "test_remote_sensing_1024x1024.tif")
    generate_test_image(output_path)
