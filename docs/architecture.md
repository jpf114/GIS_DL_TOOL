# GIS AI 算法库架构设计文档

## 1. 概述

### 1.1 项目目标
构建一个高性能、跨平台、无 Python 依赖的 C++ GIS AI 算法库，提供：
- 遥感影像 AI 分割能力（建筑、道路、水体、植被）
- 激光雷达点云 AI 分类能力（地面、建筑、植被、电力线）
- 传统 GIS 基础算法（矢量/栅格/点云操作）
- 统一 C 接口，支持多语言调用

### 1.2 设计原则
1. **模块化**: 各模块职责清晰，依赖关系单向
2. **可扩展**: 新算法、新模型、新数据格式易于集成
3. **高性能**: 充分利用 C++ 性能优势，支持并行处理
4. **易用性**: 纯 C 接口，简化调用复杂度
5. **跨平台**: Windows/Linux/macOS 一致行为

---

## 2. 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│                    (C API Consumers)                         │
│          Python / C# / Java / Native C++ Applications        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        C API Layer                           │
│                     gis_ai.h (Public)                        │
│   Init/Shutdown | IO | GIS Ops | AI Inference | Fusion      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     C++ Internal Layer                       │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │   core   │    io    │   gis    │    ai    │  fusion  │  │
│  │          │          │          │          │          │  │
│  │ Logger   │ RasterIO │ Vector   │ ONNX     │ RasterSeg│  │
│  │ Exception│ VectorIO │ Buffer   │ Runtime  │ BatchProc│  │
│  │ Platform │ PointIO  │ Intersect│ Pre/Post │ Config   │  │
│  │ Memory   │ Factory  │ Clip...  │ Mask2Poly│          │  │
│  │ Config   │          │ CoordXfm │          │          │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   External Dependencies                      │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │   GDAL   │   GEOS   │   PROJ   │   ONNX   │  spdlog  │  │
│  │  (3.6+)  │ (3.11+)  │  (9.2+)  │(1.16+)   │ (1.12+)  │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. 模块详细设计

### 3.1 Core 模块 (`src/core/`)

**职责**: 提供全库通用的基础工具

**组件**:
| 组件 | 文件 | 功能 |
|------|------|------|
| Logger | `logger.h/.cpp` | spdlog 封装，分级日志 (DEBUG/INFO/ERROR) |
| Exception | `exception.h/.cpp` | GisAiException 异常类，含错误码和上下文 |
| Platform | `platform.h/.cpp` | 跨平台适配宏（路径分隔符、线程接口等） |
| Memory | `memory.h/.cpp` | 智能指针工具、内存池（可选） |
| Config | `config.h/.cpp` | JSON/YAML 配置管理 |

**依赖关系**: 无外部依赖（除 spdlog）

---

### 3.2 IO 模块 (`src/io/`)

**职责**: GIS 数据的读写抽象

**组件**:
| 组件 | 文件 | 功能 |
|------|------|------|
| RasterIO | `raster_io.h/.cpp` | TIFF 格式读写（单/多波段），保留坐标/投影信息 |
| VectorIO | `vector_io.h/.cpp` | Shapefile/GeoJSON 读写（点/线/面） |
| PointCloudIO | `pointcloud_io.h/.cpp` | LAS 格式读写（坐标/强度/分类） |
| IOFactory | `io_factory.h/.cpp` | 根据文件扩展名自动选择 IO 处理器 |

**数据结构**:
```cpp
// 栅格数据抽象
struct Raster {
    int width, height;
    int band_count;
    double geotransform[6];  // 地理变换参数
    std::string projection;   // 投影信息
    std::vector<std::vector<double>> bands;
};

// 矢量数据抽象
struct Vector {
    enum class Type { Point, LineString, Polygon };
    Type type;
    std::vector<std::vector<double>> coordinates;
    std::unordered_map<std::string, std::string> attributes;
};

// 点云数据抽象
struct PointCloud {
    struct Point {
        double x, y, z;
        float intensity;
        uint8_t classification;
    };
    std::vector<Point> points;
    std::string coordinate_system;
};
```

**依赖关系**: GDAL

---

### 3.3 GIS 模块 (`src/gis/`)

**职责**: 传统 GIS 算法实现

**组件**:

#### 矢量算法
| 组件 | 文件 | 功能 |
|------|------|------|
| VectorBuffer | `vector_buffer.h/.cpp` | 缓冲区分析（GEOS） |
| VectorIntersect | `vector_intersect.h/.cpp` | 相交分析（GEOS） |
| VectorClip | `vector_clip.h/.cpp` | 裁剪分析（GEOS） |
| VectorSimplify | `vector_simplify.h/.cpp` | 多边形简化（Douglas-Peucker） |
| VectorTopology | `vector_topology.h/.cpp` | 拓扑检查（重叠/缝隙检测） |

#### 栅格算法
| 组件 | 文件 | 功能 |
|------|------|------|
| RasterResample | `raster_resample.h/.cpp` | 重采样（nearest/bilinear） |
| RasterNormalize | `raster_normalize.h/.cpp` | 归一化到 [0,1] |
| RasterClip | `raster_clip.h/.cpp` | 不规则裁剪 |
| RasterThreshold | `raster_threshold.h/.cpp` | 阈值分割 |
| RasterMosaic | `raster_mosaic.h/.cpp` | 多栅格拼接 |

#### 点云算法
| 组件 | 文件 | 功能 |
|------|------|------|
| PcFilter | `pc_filter.h/.cpp` | 直通滤波、高斯滤波 |
| PcDownsample | `pc_downsample.h/.cpp` | 体素降采样 |

#### 坐标转换
| 组件 | 文件 | 功能 |
|------|------|------|
| CoordTransform | `coord_transform.h/.cpp` | 基于 PROJ 的坐标投影转换 |

**依赖关系**: GEOS（矢量）、GDAL（栅格）、PROJ（坐标转换）

---

### 3.4 AI 模块 (`src/ai/`)

**职责**: AI 推理引擎封装

**组件**:
| 组件 | 文件 | 功能 |
|------|------|------|
| OrtContext | `ort_context.h/.cpp` | ONNX Runtime 会话管理 |
| ModelManager | `model_manager.h/.cpp` | 模型加载/卸载/切换 |
| InferenceEngine | `inference_engine.h/.cpp` | 统一推理接口 |
| Preprocess | `preprocess.h/.cpp` | 影像归一化、尺寸对齐、通道转换 |
| Postprocess | `postprocess.h/.cpp` | 概率图转二值图 |
| MaskToPolygon | `mask_to_polygon.h/.cpp` | 掩码转矢量轮廓 |

**推理流程**:
```
输入影像 → 预处理 → ONNX 推理 → 后处理 → 输出掩码
            ↓                        ↓
      归一化/尺寸调整           阈值/多边形提取
```

**依赖关系**: ONNX Runtime、GDAL（后处理）

---

### 3.5 Fusion 模块 (`src/fusion/`)

**职责**: AI+GIS 融合算法

**组件**:
| 组件 | 文件 | 功能 |
|------|------|------|
| RasterSeg | `raster_seg.h/.cpp` | 遥感影像端到端分割 |
| BatchProcessor | `batch_processor.h/.cpp` | 批量推理支持 |

**RasterSeg 流程**:
```
TIFF 输入 → RasterIO 读取 → 预处理 → AI 推理 → 后处理 → 
→ 二值 TIFF 输出 + SHP 矢量输出（保留坐标）
```

**依赖关系**: IO 模块、AI 模块、GIS 模块

---

## 4. C API 设计

### 4.1 核心接口

```c
// 初始化和关闭
GIS_AI_API int GisAi_Init(const char* config_path);
GIS_AI_API void GisAi_Shutdown();

// 错误处理
GIS_AI_API const char* GisAi_GetLastError();
GIS_AI_API int GisAi_GetLastErrorCode();
```

### 4.2 IO 接口

```c
// 栅格
GIS_AI_API GisAiRaster* GisAi_Raster_Load(const char* path);
GIS_AI_API int GisAi_Raster_Save(GisAiRaster* raster, const char* path);
GIS_AI_API void GisAi_Raster_Destroy(GisAiRaster* raster);

// 矢量
GIS_AI_API GisAiVector* GisAi_Vector_Load(const char* path);
GIS_AI_API int GisAi_Vector_Save(GisAiVector* vector, const char* path);
GIS_AI_API void GisAi_Vector_Destroy(GisAiVector* vector);

// 点云
GIS_AI_API GisAiPointCloud* GisAi_PointCloud_Load(const char* path);
GIS_AI_API int GisAi_PointCloud_Save(GisAiPointCloud* pc, const char* path);
GIS_AI_API void GisAi_PointCloud_Destroy(GisAiPointCloud* pc);
```

### 4.3 GIS 算法接口

```c
// 矢量算法
GIS_AI_API GisAiVector* GisAi_Vector_Buffer(GisAiVector* vector, double distance);
GIS_AI_API GisAiVector* GisAi_Vector_Intersect(GisAiVector* a, GisAiVector* b);
GIS_AI_API GisAiVector* GisAi_Vector_Clip(GisAiVector* target, GisAiVector* clipper);

// 栅格算法
GIS_AI_API GisAiRaster* GisAi_Raster_Resample(GisAiRaster* raster, int new_width, int new_height, const char* method);
GIS_AI_API GisAiRaster* GisAi_Raster_Normalize(GisAiRaster* raster);
GIS_AI_API GisAiRaster* GisAi_Raster_Threshold(GisAiRaster* raster, double threshold);
```

### 4.4 AI 接口

```c
// 模型管理
GIS_AI_API GisAiModel* GisAi_Model_Load(const char* model_path);
GIS_AI_API void GisAi_Model_Unload(GisAiModel* model);

// 推理
GIS_AI_API GisAiRaster* GisAi_AI_Infer(GisAiModel* model, GisAiRaster* input);

// 遥感分割
GIS_AI_API GisAiRasterSeg* GisAi_RasterSeg_Create(const char* model_path);
GIS_AI_API int GisAi_RasterSeg_Run(GisAiRasterSeg* seg, const char* input_tif, const char* output_tif, const char* output_shp);
GIS_AI_API void GisAi_RasterSeg_Destroy(GisAiRasterSeg* seg);
```

---

## 5. 内存管理策略

### 5.1 智能指针
- 内部实现统一使用 `std::unique_ptr` 和 `std::shared_ptr`
- 避免原始指针和手动 `new/delete`

### 5.2 C API 所有权
- 所有 `GisAi*_Create`/`GisAi*_Load` 返回的对象由调用者拥有
- 必须调用对应的 `Destroy`/`Unload` 函数释放
- 文档中明确标注内存所有权转移

### 5.3 内存池（可选优化）
- 对大规模栅格/点云数据处理提供内存池选项
- 减少频繁分配开销

---

## 6. 异常处理策略

### 6.1 异常类型层次
```
GisAiException (基类)
├── GisAiIOException (IO 失败)
├── GisAiModelException (模型加载/推理失败)
├── GisAiAlgorithmException (算法执行失败)
└── GisAiConfigException (配置错误)
```

### 6.2 错误码定义
```c
enum GisAiErrorCode {
    GIS_AI_SUCCESS = 0,
    GIS_AI_ERR_IO = 1001,
    GIS_AI_ERR_MODEL_LOAD = 2001,
    GIS_AI_ERR_INFERENCE = 2002,
    GIS_AI_ERR_ALGORITHM = 3001,
    GIS_AI_ERR_CONFIG = 4001,
    GIS_AI_ERR_MEMORY = 5001,
    GIS_AI_ERR_INVALID_PARAM = 6001
};
```

### 6.3 C API 错误处理
- C 接口通过返回值和 `GisAi_GetLastError()` 传递错误
- 内部 C++ 异常在边界处捕获并转换为错误码

---

## 7. 线程安全设计

### 7.1 基本原则
- 无状态对象天然线程安全
- 有状态对象（如推理会话）需要外部同步
- 文档明确标注线程安全性

### 7.2 并行策略
- 批量推理：多个输入可并行处理
- 多线程支持：通过 OpenMP 或 std::thread 实现

---

## 8. 扩展机制

### 8.1 新数据格式
- 实现 `IRasterReader`/`IVectorReader` 接口
- 在 `IOFactory` 中注册

### 8.2 新 AI 模型
- 使用 `ModelManager` 加载任意 ONNX 模型
- 实现对应的预处理/后处理逻辑

### 8.3 新 GIS 算法
- 在对应模块添加新类
- 在 C API 层暴露接口

---

## 9. 性能优化方向

### 9.1 已实现
- 智能指针内存管理
- 流式处理大文件

### 9.2 计划中
- 内存池优化
- GPU 加速（CUDA/OpenVINO）
- 分布式批处理

---

## 10. 目录结构

```
gis_ai_lib/
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── README.md
├── LICENSE
├── include/
│   ├── gis_ai/
│   │   └── gis_ai.h           # Public C API
│   └── internal/              # Internal headers (optional)
├── src/
│   ├── core/
│   │   ├── logger.h/.cpp
│   │   ├── exception.h/.cpp
│   │   ├── platform.h/.cpp
│   │   ├── memory.h/.cpp
│   │   └── config.h/.cpp
│   ├── io/
│   │   ├── raster_io.h/.cpp
│   │   ├── vector_io.h/.cpp
│   │   ├── pointcloud_io.h/.cpp
│   │   └── io_factory.h/.cpp
│   ├── gis/
│   │   ├── vector_*.h/.cpp    # Vector algorithms
│   │   ├── raster_*.h/.cpp    # Raster algorithms
│   │   ├── pc_*.h/.cpp        # Point cloud algorithms
│   │   └── coord_transform.h/.cpp
│   ├── ai/
│   │   ├── ort_context.h/.cpp
│   │   ├── model_manager.h/.cpp
│   │   ├── inference_engine.h/.cpp
│   │   ├── preprocess.h/.cpp
│   │   ├── postprocess.h/.cpp
│   │   └── mask_to_polygon.h/.cpp
│   └── fusion/
│       ├── raster_seg.h/.cpp
│       └── batch_processor.h/.cpp
├── models/                    # ONNX models
├── tests/
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── test_core.cpp
│   ├── test_io.cpp
│   ├── test_gis.cpp
│   ├── test_ai.cpp
│   └── test_fusion.cpp
├── examples/
│   ├── CMakeLists.txt
│   ├── demo_segmentation.cpp
│   ├── demo_gis_ops.cpp
│   └── demo_config.cpp
├── scripts/                   # Utility scripts
├── docker/
│   └── Dockerfile.dev
└── docs/
    ├── PROJECT_PROGRESS.md
    ├── architecture.md
    └── api_reference.md
```

---

*最后更新：2026-04-23*