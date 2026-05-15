"""
Export a lightweight U-Net model to ONNX for GIS_DL_TOOL testing.

This script creates a simple U-Net with 2 output classes (background + building)
and exports it to ONNX format compatible with the project's inference pipeline.

Model specs:
  Input:  [1, 3, 512, 512] float32 (ImageNet normalized)
  Output: [1, 2, 512, 512] float32 (raw logits, project applies Sigmoid internally)

Usage:
  conda activate TraeAI-4
  python tools/export_unet_building.py
"""

import sys
import os
import numpy as np

try:
    import torch
    import torch.nn as nn
except ImportError:
    print("PyTorch not found. Install with: conda install pytorch torchvision cpuonly -c pytorch")
    sys.exit(1)

try:
    import onnx
    import onnxruntime as ort
except ImportError:
    print("onnx/onnxruntime not found. Install with: pip install onnx onnxruntime")
    sys.exit(1)


class ConvBlock(nn.Module):
    def __init__(self, in_ch, out_ch):
        super().__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(in_ch, out_ch, 3, padding=1),
            nn.BatchNorm2d(out_ch),
            nn.ReLU(inplace=True),
            nn.Conv2d(out_ch, out_ch, 3, padding=1),
            nn.BatchNorm2d(out_ch),
            nn.ReLU(inplace=True),
        )

    def forward(self, x):
        return self.conv(x)


class UNet(nn.Module):
    def __init__(self, in_channels=3, num_classes=2, base_filters=32):
        super().__init__()
        f = base_filters

        self.enc1 = ConvBlock(in_channels, f)
        self.enc2 = ConvBlock(f, f * 2)
        self.enc3 = ConvBlock(f * 2, f * 4)
        self.enc4 = ConvBlock(f * 4, f * 8)

        self.pool = nn.MaxPool2d(2)
        self.up = nn.Upsample(scale_factor=2, mode="bilinear", align_corners=True)

        self.dec3 = ConvBlock(f * 8 + f * 4, f * 4)
        self.dec2 = ConvBlock(f * 4 + f * 2, f * 2)
        self.dec1 = ConvBlock(f * 2 + f, f)

        self.out_conv = nn.Conv2d(f, num_classes, 1)

    def forward(self, x):
        e1 = self.enc1(x)
        e2 = self.enc2(self.pool(e1))
        e3 = self.enc3(self.pool(e2))
        e4 = self.enc4(self.pool(e3))

        d3 = self.dec3(torch.cat([self.up(e4), e3], dim=1))
        d2 = self.dec2(torch.cat([self.up(d3), e2], dim=1))
        d1 = self.dec1(torch.cat([self.up(d2), e1], dim=1))

        return self.out_conv(d1)


def export_unet_building(output_dir, opset_version=11):
    os.makedirs(output_dir, exist_ok=True)
    onnx_path = os.path.join(output_dir, "unet_building_512x512.onnx")

    model = UNet(in_channels=3, num_classes=2, base_filters=32)
    model.eval()

    dummy_input = torch.randn(1, 3, 512, 512)

    with torch.no_grad():
        output = model(dummy_input)
    print(f"Model output shape: {output.shape}")

    torch.onnx.export(
        model,
        dummy_input,
        onnx_path,
        opset_version=opset_version,
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={
            "input": {0: "batch", 2: "height", 3: "width"},
            "output": {0: "batch", 2: "height", 3: "width"},
        },
    )
    print(f"ONNX model exported to: {onnx_path}")

    onnx_model = onnx.load(onnx_path)
    onnx.checker.check_model(onnx_model)
    print("ONNX model validation passed")

    session = ort.InferenceSession(onnx_path)
    input_info = session.get_inputs()[0]
    output_info = session.get_outputs()[0]
    print(f"Input:  name={input_info.name}, shape={input_info.shape}, type={input_info.type}")
    print(f"Output: name={output_info.name}, shape={output_info.shape}, type={output_info.type}")

    test_input = np.random.randn(1, 3, 512, 512).astype(np.float32)
    result = session.run(None, {input_info.name: test_input})
    print(f"Test inference output shape: {result[0].shape}")
    print(f"Output value range: [{result[0].min():.4f}, {result[0].max():.4f}]")

    meta_path = os.path.join(output_dir, "model_info.json")
    import json
    info = {
        "name": "unet_building_512x512",
        "description": "U-Net building extraction model (2-class: background + building)",
        "architecture": "U-Net",
        "input_format": "NCHW [1, 3, H, W] float32, ImageNet normalized",
        "output_format": "NCHW [1, 2, H, W] float32, raw logits",
        "num_classes": 2,
        "class_names": ["background", "building"],
        "input_size": [512, 512],
        "normalize": {
            "mode": "ImageNet",
            "mean": [0.485, 0.456, 0.406],
            "std": [0.229, 0.224, 0.225]
        },
        "file": "unet_building_512x512.onnx",
        "file_size_mb": round(os.path.getsize(onnx_path) / (1024 * 1024), 2),
    }
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(info, f, indent=2, ensure_ascii=False)
    print(f"Model info saved to: {meta_path}")

    return onnx_path


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    output_dir = os.path.join(project_root, "models", "unet_building")

    onnx_path = export_unet_building(output_dir)
    print(f"\nDone! Model ready at: {onnx_path}")
