import onnx
from onnx import helper, TensorProto, numpy_helper
import numpy as np
import struct
import os

def create_minimal_seg_model(output_path, input_size=512, num_classes=2):
    X = helper.make_tensor_value_info('input', TensorProto.FLOAT, [1, 3, input_size, input_size])
    Y = helper.make_tensor_value_info('output', TensorProto.FLOAT, [1, num_classes, input_size, input_size])

    weight_data = np.random.randn(num_classes, 3, 1, 1).astype(np.float32) * 0.1
    bias_data = np.zeros(num_classes, dtype=np.float32)
    bias_data[0] = -1.0
    bias_data[1] = 1.0

    weight_init = numpy_helper.from_array(weight_data, name='conv_weight')
    bias_init = numpy_helper.from_array(bias_data, name='conv_bias')

    conv_node = helper.make_node(
        'Conv', ['input', 'conv_weight', 'conv_bias'], ['output'],
        kernel_shape=[1, 1], pads=[0, 0, 0, 0], strides=[1, 1]
    )

    graph = helper.make_graph(
        [conv_node], 'seg_model',
        [X], [Y],
        initializer=[weight_init, bias_init]
    )

    model = helper.make_model(graph, opset_imports=[helper.make_opsetid('', 11)])
    model.ir_version = 7

    onnx.checker.check_model(model)
    onnx.save(model, output_path)
    print(f"Model saved to {output_path} (input: [1,3,{input_size},{input_size}], output: [1,{num_classes},{input_size},{input_size}])")

def create_test_tiff(output_path, width=1024, height=768, bands=3):
    from osgeo import gdal, osr

    driver = gdal.GetDriverByName('GTiff')
    ds = driver.Create(output_path, width, height, bands, gdal.GDT_Byte)

    ds.SetGeoTransform([116.0, 0.001, 0.0, 40.0, 0.0, -0.001])

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)
    ds.SetProjection(srs.ExportToWkt())

    np.random.seed(12345)
    for b in range(bands):
        band = ds.GetRasterBand(b + 1)
        data = np.random.randint(50, 200, (height, width), dtype=np.uint8)

        cx, cy = width // 2, height // 2
        r = min(width, height) // 4
        y_idx, x_idx = np.ogrid[:height, :width]
        mask = (x_idx - cx)**2 + (y_idx - cy)**2 < r**2
        data[mask] = np.random.randint(180, 255, mask.sum(), dtype=np.uint8)

        band.WriteArray(data)
        band.SetNoDataValue(0)

    ds.FlushCache()
    ds = None
    print(f"Test TIFF saved to {output_path} ({width}x{height}x{bands})")

if __name__ == '__main__':
    test_dir = os.path.join(os.path.dirname(__file__), 'test_e2e_data')
    os.makedirs(test_dir, exist_ok=True)

    create_minimal_seg_model(os.path.join(test_dir, 'test_seg_model.onnx'), input_size=512, num_classes=2)
    create_test_tiff(os.path.join(test_dir, 'test_input.tif'), width=1024, height=768, bands=3)

    print("\nTest data generation complete!")
    print(f"  Model: {os.path.join(test_dir, 'test_seg_model.onnx')}")
    print(f"  Input: {os.path.join(test_dir, 'test_input.tif')}")
