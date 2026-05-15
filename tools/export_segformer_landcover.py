"""
Export a SegFormer-B0 style model to ONNX for GIS_DL_TOOL.

Since HuggingFace may be inaccessible from China, this script builds a
SegFormer-B0 architecture from scratch with random weights and exports it
to ONNX format compatible with the project's inference pipeline.

If HuggingFace is accessible, set USE_PRETRAINED=1 to download real weights.

Model specs:
  Input:  [1, 3, 512, 512] float32 (ImageNet normalized)
  Output: [1, 7, 512, 512] float32 (raw logits, 7 ADE20K land-cover classes)

Usage:
  conda run -n GIS python tools/export_segformer_landcover.py
  USE_PRETRAINED=1 conda run -n GIS python tools/export_segformer_landcover.py
"""

import sys
import os
import json
import numpy as np

try:
    import torch
    import torch.nn as nn
    import torch.nn.functional as F
except ImportError:
    print("PyTorch not found.")
    sys.exit(1)

try:
    import onnx
    import onnxruntime as ort
except ImportError:
    print("onnx/onnxruntime not found.")
    sys.exit(1)


class PatchEmbedding(nn.Module):
    def __init__(self, in_channels, embed_dim, patch_size=7, stride=4):
        super().__init__()
        self.proj = nn.Conv2d(in_channels, embed_dim, kernel_size=patch_size,
                              stride=stride, padding=patch_size // 2)
        self.norm = nn.LayerNorm(embed_dim)

    def forward(self, x):
        x = self.proj(x)
        _, _, H, W = x.shape
        x = x.flatten(2).transpose(1, 2)
        x = self.norm(x)
        x = x.transpose(1, 2).reshape(-1, self.norm.normalized_shape[0], H, W)
        return x


class EfficientSelfAttention(nn.Module):
    def __init__(self, dim, num_heads=8, sr_ratio=1):
        super().__init__()
        self.num_heads = num_heads
        self.head_dim = dim // num_heads
        self.q = nn.Linear(dim, dim)
        self.k = nn.Linear(dim, dim)
        self.v = nn.Linear(dim, dim)
        self.sr_ratio = sr_ratio
        if sr_ratio > 1:
            self.sr = nn.Conv2d(dim, dim, kernel_size=sr_ratio, stride=sr_ratio)
            self.norm = nn.LayerNorm(dim)

    def forward(self, x, H, W):
        B, C, H_, W_ = x.shape
        x_flat = x.flatten(2).transpose(1, 2)
        N = x_flat.shape[1]

        q = self.q(x_flat).reshape(B, N, self.num_heads, self.head_dim).permute(0, 2, 1, 3)

        if self.sr_ratio > 1:
            x_sr = self.sr(x).flatten(2).transpose(1, 2)
            x_sr = self.norm(x_sr)
            k = self.k(x_sr).reshape(B, -1, self.num_heads, self.head_dim).permute(0, 2, 1, 3)
            v = self.v(x_sr).reshape(B, -1, self.num_heads, self.head_dim).permute(0, 2, 1, 3)
        else:
            k = self.k(x_flat).reshape(B, N, self.num_heads, self.head_dim).permute(0, 2, 1, 3)
            v = self.v(x_flat).reshape(B, N, self.num_heads, self.head_dim).permute(0, 2, 1, 3)

        attn = (q @ k.transpose(-2, -1)) * (self.head_dim ** -0.5)
        attn = attn.softmax(dim=-1)
        out = (attn @ v).transpose(1, 2).reshape(B, N, -1)
        return out.transpose(1, 2).reshape(B, C, H_, W_)


class MixFFN(nn.Module):
    def __init__(self, dim, mlp_ratio=4):
        super().__init__()
        hidden = int(dim * mlp_ratio)
        self.fc1 = nn.Conv2d(dim, hidden, 1)
        self.dwconv = nn.Conv2d(hidden, hidden, 3, padding=1, groups=hidden)
        self.fc2 = nn.Conv2d(hidden, dim, 1)
        self.act = nn.GELU()

    def forward(self, x):
        x = self.act(self.fc1(x))
        x = self.act(self.dwconv(x))
        x = self.fc2(x)
        return x


class TransformerBlock(nn.Module):
    def __init__(self, dim, num_heads=8, sr_ratio=1, mlp_ratio=4):
        super().__init__()
        self.norm1 = nn.LayerNorm(dim)
        self.attn = EfficientSelfAttention(dim, num_heads, sr_ratio)
        self.norm2 = nn.LayerNorm(dim)
        self.mlp = MixFFN(dim, mlp_ratio)

    def forward(self, x, H, W):
        B, C, H_, W_ = x.shape
        residual = x
        x_flat = x.flatten(2).transpose(1, 2)
        x_flat = self.norm1(x_flat)
        x = x_flat.transpose(1, 2).reshape(B, C, H_, W_)
        x = residual + self.attn(x, H, W)

        residual = x
        x_flat = x.flatten(2).transpose(1, 2)
        x_flat = self.norm2(x_flat)
        x = x_flat.transpose(1, 2).reshape(B, C, H_, W_)
        x = residual + self.mlp(x)
        return x


class SegFormerEncoder(nn.Module):
    def __init__(self, embed_dims=(32, 64, 160, 256), num_heads=(1, 2, 5, 8),
                 depths=(2, 2, 2, 2), sr_ratios=(8, 4, 2, 1)):
        super().__init__()
        self.patch_embeds = nn.ModuleList()
        self.blocks = nn.ModuleList()
        self.norms = nn.ModuleList()

        in_ch = 3
        for i in range(4):
            self.patch_embeds.append(PatchEmbedding(in_ch, embed_dims[i],
                                                     patch_size=7 if i == 0 else 3,
                                                     stride=4 if i == 0 else 2))
            stage_blocks = nn.ModuleList()
            for _ in range(depths[i]):
                stage_blocks.append(TransformerBlock(embed_dims[i], num_heads[i], sr_ratios[i]))
            self.blocks.append(stage_blocks)
            self.norms.append(nn.LayerNorm(embed_dims[i]))
            in_ch = embed_dims[i]

    def forward(self, x):
        features = []
        for i in range(4):
            x = self.patch_embeds[i](x)
            B, C, H, W = x.shape
            for blk in self.blocks[i]:
                x = blk(x, H, W)
            x_flat = x.flatten(2).transpose(1, 2)
            x_flat = self.norms[i](x_flat)
            x = x_flat.transpose(1, 2).reshape(B, C, H, W)
            features.append(x)
        return features


class SegFormerDecoder(nn.Module):
    def __init__(self, embed_dims=(32, 64, 160, 256), num_classes=7):
        super().__init__()
        self.linear_c4 = nn.Conv2d(embed_dims[3], 256, 1)
        self.linear_c3 = nn.Conv2d(embed_dims[2], 256, 1)
        self.linear_c2 = nn.Conv2d(embed_dims[1], 256, 1)
        self.linear_c1 = nn.Conv2d(embed_dims[0], 256, 1)
        self.fuse = nn.Sequential(
            nn.Conv2d(1024, 256, 1),
            nn.BatchNorm2d(256),
            nn.ReLU(inplace=True),
        )
        self.classifier = nn.Conv2d(256, num_classes, 1)

    def forward(self, features, target_size=512):
        c1, c2, c3, c4 = features

        c4 = self.linear_c4(c4)
        c4 = F.interpolate(c4, size=(target_size, target_size), mode='bilinear', align_corners=False)

        c3 = self.linear_c3(c3)
        c3 = F.interpolate(c3, size=(target_size, target_size), mode='bilinear', align_corners=False)

        c2 = self.linear_c2(c2)
        c2 = F.interpolate(c2, size=(target_size, target_size), mode='bilinear', align_corners=False)

        c1 = self.linear_c1(c1)
        c1 = F.interpolate(c1, size=(target_size, target_size), mode='bilinear', align_corners=False)

        x = torch.cat([c1, c2, c3, c4], dim=1)
        x = self.fuse(x)
        x = self.classifier(x)
        return x


class SegFormerB0(nn.Module):
    def __init__(self, num_classes=7):
        super().__init__()
        self.encoder = SegFormerEncoder(
            embed_dims=(32, 64, 160, 256),
            num_heads=(1, 2, 5, 8),
            depths=(2, 2, 2, 2),
            sr_ratios=(8, 4, 2, 1),
        )
        self.decoder = SegFormerDecoder(
            embed_dims=(32, 64, 160, 256),
            num_classes=num_classes,
        )

    def forward(self, x):
        features = self.encoder(x)
        return self.decoder(features, target_size=x.shape[2])


def export_segformer_landcover(output_dir):
    os.makedirs(output_dir, exist_ok=True)

    use_pretrained = os.environ.get("USE_PRETRAINED", "0") == "1"

    if use_pretrained:
        try:
            from transformers import SegformerForSemanticSegmentation
            model_name = "nvidia/segformer-b0-finetuned-ade-512-512"
            print(f"Loading pre-trained model: {model_name} ...")
            hf_model = SegformerForSemanticSegmentation.from_pretrained(model_name)
            num_classes = hf_model.config.num_labels
            print(f"Number of classes: {num_classes}")

            class Wrapper(nn.Module):
                def __init__(self, model, target_size=512):
                    super().__init__()
                    self.model = model
                    self.target_size = target_size
                def forward(self, x):
                    out = self.model(pixel_values=x).logits
                    if out.shape[2] != self.target_size or out.shape[3] != self.target_size:
                        out = F.interpolate(out, size=(self.target_size, self.target_size),
                                            mode='bilinear', align_corners=False)
                    return out
            model = Wrapper(hf_model)
            class_names = [hf_model.config.id2label.get(str(i), f"class_{i}")
                          for i in range(num_classes)]
        except Exception as e:
            print(f"Failed to load pre-trained model: {e}")
            print("Falling back to random-weight model.")
            use_pretrained = False

    if not use_pretrained:
        num_classes = 7
        class_names = ["background", "building", "road", "water", "barren",
                       "forest", "agriculture"]
        model = SegFormerB0(num_classes=num_classes)
        print(f"Built SegFormer-B0 with {num_classes} classes (random weights)")

    model.eval()
    onnx_path = os.path.join(output_dir, "segformer_b0_landcover_512x512.onnx")

    dummy_input = torch.randn(1, 3, 512, 512)
    with torch.no_grad():
        output = model(dummy_input)
    print(f"Model output shape: {output.shape}")

    print("Exporting to ONNX...")
    torch.onnx.export(
        model,
        dummy_input,
        onnx_path,
        opset_version=14,
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
    info = {
        "name": "segformer_b0_landcover_512x512",
        "description": f"SegFormer-B0 land-cover segmentation ({num_classes} classes: {', '.join(class_names)})",
        "architecture": "SegFormer-B0",
        "source": "nvidia/segformer-b0-finetuned-ade-512-512" if use_pretrained else "random-weight (for pipeline testing)",
        "input_format": "NCHW [1, 3, H, W] float32, ImageNet normalized",
        "output_format": f"NCHW [1, {num_classes}, H, W] float32, raw logits",
        "num_classes": num_classes,
        "class_names": class_names,
        "input_size": [512, 512],
        "normalize": {
            "mode": "ImageNet",
            "mean": [0.485, 0.456, 0.406],
            "std": [0.229, 0.224, 0.225]
        },
        "file": "segformer_b0_landcover_512x512.onnx",
        "file_size_mb": round(os.path.getsize(onnx_path) / (1024 * 1024), 2),
    }
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(info, f, indent=2, ensure_ascii=False)
    print(f"Model info saved to: {meta_path}")

    return onnx_path


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    output_dir = os.path.join(project_root, "models", "segformer_landcover")

    onnx_path = export_segformer_landcover(output_dir)
    print(f"\nDone! Model ready at: {onnx_path}")
