# GIS AI TOOL C++ API 参考

本文档详细描述 GIS AI TOOL 的 C++ 接口，涵盖 `core`、`io`、`gis`、`ai`、`fusion` 五大模块。所有类与函数均位于 `gis_ai` 命名空间下，导出宏为 `GIS_AI_API`。

---

## 目录

- [1. 命名空间与约定](#1-命名空间与约定)
- [2. 数据结构](#2-数据结构)
  - [2.1 栅格数据](#21-栅格数据)
  - [2.2 矢量数据](#22-矢量数据)
  - [2.3 点云数据](#23-点云数据)
  - [2.4 辅助结构](#24-辅助结构)
- [3. 枚举类型](#3-枚举类型)
- [4. 异常体系与错误码](#4-异常体系与错误码)
- [5. core 模块](#5-core-模块)
  - [5.1 Logger](#51-logger)
  - [5.2 Config](#52-config)
  - [5.3 MemoryPool](#53-memorypool)
  - [5.4 Platform](#54-platform)
- [6. io 模块](#6-io-模块)
  - [6.1 RasterIO](#61-rasterio)
  - [6.2 VectorIO](#62-vectorio)
  - [6.3 PointCloudIO](#63-pointcloudio)
  - [6.4 IOFactory](#64-iofactory)
- [7. gis 模块](#7-gis-模块)
  - [7.1 CoordTransform](#71-coordtransform)
  - [7.2 RasterClip](#72-rasterclip)
  - [7.3 RasterMosaic](#73-rastermosaic)
  - [7.4 RasterNormalize](#74-rasternormalize)
  - [7.5 RasterResample](#75-rasterresample)
  - [7.6 RasterThreshold](#76-rasterthreshold)
  - [7.7 VectorBuffer](#77-vectorbuffer)
  - [7.8 VectorClip](#78-vectorclip)
  - [7.9 VectorIntersect](#79-vectorintersect)
  - [7.10 VectorSimplify](#710-vectorsimplify)
  - [7.11 VectorTopology](#711-vectortopology)
  - [7.12 PcFilter](#712-pcfilter)
  - [7.13 PcDownsample](#713-pcdownsample)
- [8. ai 模块](#8-ai-模块)
  - [8.1 OrtContext](#81-ortcontext)
  - [8.2 ModelManager](#82-modelmanager)
  - [8.3 InferenceEngine](#83-inferenceengine)
  - [8.4 Preprocess](#84-preprocess)
  - [8.5 Postprocess](#85-postprocess)
  - [8.6 MaskToPolygon](#86-masktopolygon)
- [9. fusion 模块](#9-fusion-模块)
  - [9.1 RasterSeg](#91-rasterseg)
  - [9.2 LargeImageSeg](#92-largeimageseg)
  - [9.3 BatchProcessor](#93-batchprocessor)
  - [9.4 TaskConfig / TaskRunner](#94-taskconfig--taskrunner)
- [10. 使用示例](#10-使用示例)
  - [10.1 配置与日志](#101-配置与日志)
  - [10.2 GIS 栅格操作](#102-gis-栅格操作)
  - [10.3 语义分割](#103-语义分割)
  - [10.4 大图分割](#104-大图分割)
  - [10.5 批量处理](#105-批量处理)
  - [10.6 任务配置驱动](#106-任务配置驱动)

---

## 1. 命名空间与约定

| 约定 | 说明 |
|------|------|
| 命名空间 | `gis_ai` |
| 导出宏 | `GIS_AI_API`（控制 DLL 导出/导入） |
| 智能指针 | 所有 `Load`/`Execute` 返回 `std::unique_ptr<T>`，调用方拥有所有权 |
| 异常 | 可抛出 `GisAiException` 及其子类，建议使用 try-catch 包裹 |
| 头文件路径 | `#include "模块/类名.h"`，例如 `#include "io/raster_io.h"` |

---

## 2. 数据结构

### 2.1 栅格数据

#### RasterData

```cpp
struct RasterData {
    int width = 0;                              // 栅格宽度（像素）
    int height = 0;                             // 栅格高度（像素）
    int band_count = 0;                         // 波段数
    double geotransform[6] = {0,1,0,0,0,1};    // GDAL 仿射变换六参数
    std::string projection;                     // CRS 的 WKT 表示
    std::vector<std::vector<float>> bands;      // bands[band_idx][pixel_idx]
    std::vector<BandInfo> band_infos;           // 各波段元信息
};
```

- `geotransform` 遵循 GDAL 约定：`[左上角X, 水平分辨率, 旋转, 左上角Y, 旋转, 垂直分辨率（负值）]`
- `bands` 中每个 `vector<float>` 长度为 `width * height`，按行优先存储
- `band_infos` 长度应与 `band_count` 一致

#### BandInfo

```cpp
struct BandInfo {
    RasterDataType data_type = RasterDataType::Float32;
    std::optional<float> nodata_value;          // 无数据值（可选）
};
```

### 2.2 矢量数据

#### VectorData

```cpp
struct VectorData {
    FeatureType feature_type = FeatureType::Point;
    std::string projection;                     // CRS 的 WKT 表示
    std::vector<Feature> features;
};
```

#### Feature

```cpp
struct Feature {
    FeatureType type = FeatureType::Point;
    std::vector<Coordinate> coordinates;        // 外环坐标
    std::vector<std::vector<Coordinate>> inner_rings; // 内环（仅 Polygon）
    std::map<std::string, AttributeValue> attributes;
};
```

- 对于 `Point`，`coordinates` 包含 1 个元素
- 对于 `LineString`，`coordinates` 包含至少 2 个元素
- 对于 `Polygon`，`coordinates` 为外环，`inner_rings` 为孔洞

#### Coordinate

```cpp
struct Coordinate {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};
```

#### LayerInfo

```cpp
struct LayerInfo {
    std::string name;
    FeatureType feature_type = FeatureType::Point;
    int feature_count = 0;
};
```

#### AttributeValue

```cpp
using AttributeValue = std::variant<std::string, int, double>;
```

### 2.3 点云数据

#### PointCloudData

```cpp
struct PointCloudData {
    std::vector<Point> points;
    std::string coordinate_system;
};
```

#### Point

```cpp
struct Point {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float intensity = 0.0f;
    uint8_t classification = 0;
};
```

### 2.4 辅助结构

#### BoundingBox

```cpp
struct BoundingBox {
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
};
```

坐标与 `RasterData::geotransform` 一致，使用地图坐标（非像素坐标）。

#### CoordPair

```cpp
struct CoordPair {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};
```

#### ContourPoint

```cpp
struct ContourPoint {
    double x;
    double y;
};
```

#### TopologyIssue

```cpp
struct TopologyIssue {
    int feature_index = -1;
    std::string issue_type;
    std::string description;
};
```

#### PassFilterParams

```cpp
struct PassFilterParams {
    std::string axis = "z";     // 滤波轴："x"、"y"、"z"
    double min_val = 0.0;
    double max_val = 100.0;
};
```

#### GaussianFilterParams

```cpp
struct GaussianFilterParams {
    double sigma = 1.0;
    int kernel_size = 3;
};
```

---

## 3. 枚举类型

| 枚举 | 值 | 说明 |
|------|----|------|
| **RasterDataType** | `Byte`, `UInt16`, `Int16`, `UInt32`, `Int32`, `Float32`, `Float64`, `Unknown` | 栅格像素数据类型 |
| **RasterOutputFormat** | `GTiff`, `COG`, `Auto` | 栅格输出格式。`Auto` 根据文件扩展名自动选择 |
| **FeatureType** | `Point`, `LineString`, `Polygon` | 矢量要素类型 |
| **IOFormat** | `Raster`, `Vector`, `PointCloud`, `Unknown` | IO 格式检测类型 |
| **ResampleMethod** | `Nearest`, `Bilinear` | 重采样方法。`Nearest` 适合分类图，`Bilinear` 适合连续值 |
| **MosaicStrategy** | `Overwrite`, `First`, `Mean`, `Max`, `Min` | 镶嵌重叠区策略 |
| **NormalizeMode** | `None`, `ImageNet`, `MinMax01`, `ZScore`, `Custom` | 归一化模式 |
| **BlendMode** | `None`, `Linear`, `Gaussian` | 大图分割滑窗混合模式 |
| **TaskType** | `Segment`, `SegmentToPolygon`, `BatchSegment`, `BatchInference`, `Inference`, `Preprocess`, `VectorSimplify`, `VectorBuffer`, `RasterMosaic`, `RasterResample` | 任务类型 |

---

## 4. 异常体系与错误码

### 错误码枚举

| 错误码 | 数值 | 含义 |
|--------|------|------|
| `Success` | 0 | 成功 |
| `IO` | 1001 | 通用 IO 错误 |
| `FileNotFound` | 1002 | 文件未找到 |
| `FileReadError` | 1003 | 文件读取错误 |
| `FileWriteError` | 1004 | 文件写入错误 |
| `ModelLoad` | 2001 | 模型加载失败 |
| `Inference` | 2002 | 推理执行失败 |
| `Algorithm` | 3001 | 算法执行错误 |
| `Config` | 4001 | 配置错误 |
| `ConfigParseError` | 4002 | 配置解析错误 |
| `ConfigValidationError` | 4003 | 配置校验失败 |
| `Memory` | 5001 | 内存错误 |
| `InvalidParam` | 6001 | 参数无效 |
| `MissingRequiredParam` | 6002 | 缺少必要参数 |
| `TaskExecution` | 7001 | 任务执行错误 |
| `TaskCancelled` | 7002 | 任务已取消 |
| `Unknown` | 9999 | 未知错误 |

### 异常类层次

```
std::runtime_error
  └── GisAiException          // 基类，携带 ErrorCode + context
        ├── GisAiIOException      // IO 类错误
        ├── GisAiModelException   // 模型加载/推理错误
        ├── GisAiAlgorithmException // 算法执行错误
        └── GisAiConfigException  // 配置错误
```

### GisAiException

```cpp
class GisAiException : public std::runtime_error {
public:
    GisAiException(ErrorCode code, const std::string& message, const std::string& context = "");
    ErrorCode GetCode() const;
    std::string GetContext() const;
};
```

| 方法 | 说明 |
|------|------|
| `GetCode()` | 返回错误码枚举 |
| `GetContext()` | 返回错误上下文（如函数名、文件路径等） |

### 辅助函数

```cpp
std::string ErrorCodeToString(ErrorCode code);  // 错误码 → 可读字符串
int ErrorCodeToInt(ErrorCode code);              // 错误码 → 整数值
```

---

## 5. core 模块

核心基础设施：日志、配置、内存管理、平台抽象。

### 5.1 Logger

单例日志器，基于 spdlog，支持控制台彩色输出和文件输出。

```cpp
class Logger {
public:
    static Logger& Instance();

    void Initialize(const std::string& log_file = "gis_ai.log",
                    spdlog::level::level_enum level = spdlog::level::info);
    std::shared_ptr<spdlog::logger> GetLogger() const;
    void EnsureInitialized();

    void Debug(const std::string& msg);
    void Info(const std::string& msg);
    void Warn(const std::string& msg);
    void Error(const std::string& msg);
};
```

| 方法 | 说明 |
|------|------|
| `Instance()` | 获取单例引用 |
| `Initialize()` | 初始化日志系统。`log_file` 为输出文件路径，`level` 为最低日志级别 |
| `GetLogger()` | 获取底层 spdlog logger，用于自定义日志 |
| `EnsureInitialized()` | 若未初始化则以默认参数初始化 |
| `Debug/Info/Warn/Error` | 各级别日志输出 |

**便捷宏**：

```cpp
LOG_DEBUG(msg)
LOG_INFO(msg)
LOG_WARN(msg)
LOG_ERROR(msg)
```

> **线程安全**：spdlog 默认线程安全，Logger 内部使用 `std::mutex` 保护初始化过程。

### 5.2 Config

单例配置管理器，支持 JSON 文件读写和运行时键值存取。

```cpp
class Config {
public:
    using Value = std::variant<std::string, int, double, bool>;

    static Config& Instance();

    bool LoadFromFile(const std::string& path);
    bool LoadFromString(const std::string& json_str);
    void SaveToFile(const std::string& path) const;

    std::optional<Value> Get(const std::string& key) const;
    std::string GetString(const std::string& key, const std::string& default_val = "") const;
    int GetInt(const std::string& key, int default_val = 0) const;
    double GetDouble(const std::string& key, double default_val = 0.0) const;
    bool GetBool(const std::string& key, bool default_val = false) const;

    void Set(const std::string& key, const Value& value);
    bool Has(const std::string& key) const;
    void Remove(const std::string& key);
    void Clear();
};
```

| 方法 | 说明 |
|------|------|
| `LoadFromFile()` | 从 JSON 文件加载配置，返回是否成功 |
| `LoadFromString()` | 从 JSON 字符串加载配置 |
| `SaveToFile()` | 将当前配置序列化为 JSON 写入文件 |
| `Get()` | 获取原始 `Value`，键不存在返回 `std::nullopt` |
| `GetString/GetInt/GetDouble/GetBool()` | 类型安全获取，键不存在或类型不匹配返回默认值 |
| `Set()` | 设置键值对，`Value` 支持 `string/int/double/bool` |
| `Has()` | 检查键是否存在 |
| `Remove()` | 删除指定键 |
| `Clear()` | 清空所有配置项 |

> **线程安全**：Config 非线程安全，建议在主线程初始化后只读使用。

### 5.3 MemoryPool

简易内存池，用于频繁分配/释放同类型内存块。

```cpp
class MemoryPool {
public:
    explicit MemoryPool(size_t block_size = 1024 * 1024);

    void* Allocate(size_t size);
    void Deallocate(void* ptr, size_t size);
    void Reset();

    size_t GetTotalAllocated() const;
    size_t GetPeakUsage() const;
};
```

| 方法 | 说明 |
|------|------|
| `Allocate()` | 分配指定大小的内存块 |
| `Deallocate()` | 释放内存（实际不归还操作系统，仅标记可复用） |
| `Reset()` | 重置内存池，释放所有已分配块 |
| `GetTotalAllocated()` | 获取累计分配字节数 |
| `GetPeakUsage()` | 获取峰值内存使用量 |

> **不可拷贝**，可移动。

### 5.4 Platform

平台相关工具类，全部为静态方法。

```cpp
class Platform {
public:
    static std::string GetPathSeparator();
    static std::string JoinPath(const std::string& a, const std::string& b);
    static std::string GetExecutableDir();
    static std::string GetHomeDir();
    static bool FileExists(const std::string& path);
    static bool DirExists(const std::string& path);
    static bool CreateDir(const std::string& path);
    static unsigned int GetHardwareConcurrency();
};
```

| 方法 | 说明 |
|------|------|
| `GetPathSeparator()` | 返回当前平台路径分隔符（Windows: `\`，POSIX: `/`） |
| `JoinPath()` | 拼接两个路径片段 |
| `GetExecutableDir()` | 获取当前可执行文件所在目录 |
| `GetHomeDir()` | 获取用户主目录 |
| `FileExists()` | 检查文件是否存在 |
| `DirExists()` | 检查目录是否存在 |
| `CreateDir()` | 创建目录（递归创建父目录） |
| `GetHardwareConcurrency()` | 获取硬件并发线程数 |

**平台宏**：

```cpp
// Windows 下
#define GIS_AI_PLATFORM_WINDOWS 1
#define GIS_AI_PATH_SEPARATOR '\\'

// Linux/macOS 下
#define GIS_AI_PLATFORM_POSIX 1
#define GIS_AI_PATH_SEPARATOR '/'
```

---

## 6. io 模块

数据 IO 层，负责栅格、矢量、点云数据的读写。

### 6.1 RasterIO

栅格数据读写，基于 GDAL。

```cpp
class RasterIO {
public:
    RasterIO() = default;
    ~RasterIO() = default;

    std::unique_ptr<RasterData> Load(const std::string& path);
    void Save(const RasterData& data, const std::string& path,
              RasterOutputFormat format = RasterOutputFormat::Auto);
};
```

| 方法 | 说明 |
|------|------|
| `Load(path)` | 加载栅格文件，支持 GeoTIFF 等格式。返回 `unique_ptr<RasterData>`，失败时抛出 `GisAiIOException` |
| `Save(data, path, format)` | 保存栅格数据到文件。`format` 默认 `Auto`，根据扩展名推断 |

**支持的格式**：GeoTIFF (`.tif`/`.tiff`)、COG (Cloud Optimized GeoTIFF) 等 GDAL 支持的栅格格式。

### 6.2 VectorIO

矢量数据读写，基于 OGR。

```cpp
class VectorIO {
public:
    VectorIO() = default;
    ~VectorIO() = default;

    std::unique_ptr<VectorData> Load(const std::string& path, const std::string& layer_name = "");
    std::unique_ptr<VectorData> LoadLayer(const std::string& path, int layer_index);
    std::vector<LayerInfo> ListLayers(const std::string& path);
    void Save(const VectorData& data, const std::string& path);
};
```

| 方法 | 说明 |
|------|------|
| `Load(path, layer_name)` | 加载矢量文件。`layer_name` 为空时加载第一个图层 |
| `LoadLayer(path, layer_index)` | 按索引加载指定图层 |
| `ListLayers(path)` | 列出文件中所有图层信息 |
| `Save(data, path)` | 保存矢量数据，根据扩展名自动选择驱动（`.shp` → Shapefile） |

**支持的格式**：Shapefile (`.shp`)、GeoJSON (`.geojson`/`.json`)、GPKG (`.gpkg`) 等 OGR 支持的矢量格式。

### 6.3 PointCloudIO

点云数据读写。

```cpp
class PointCloudIO {
public:
    PointCloudIO() = default;
    ~PointCloudIO() = default;

    std::unique_ptr<PointCloudData> Load(const std::string& path);
    void Save(const PointCloudData& data, const std::string& path);
};
```

| 方法 | 说明 |
|------|------|
| `Load(path)` | 加载点云文件（LAS/LAZ 格式） |
| `Save(data, path)` | 保存点云数据到文件 |

### 6.4 IOFactory

IO 工厂类，用于格式检测和 IO 对象创建。

```cpp
class IOFactory {
public:
    static IOFormat DetectFormat(const std::string& path);
    static std::unique_ptr<RasterIO> CreateRasterIO();
    static std::unique_ptr<VectorIO> CreateVectorIO();
    static std::unique_ptr<PointCloudIO> CreatePointCloudIO();
};
```

| 方法 | 说明 |
|------|------|
| `DetectFormat(path)` | 根据文件扩展名和内容检测数据格式 |
| `CreateRasterIO()` | 创建 RasterIO 实例 |
| `CreateVectorIO()` | 创建 VectorIO 实例 |
| `CreatePointCloudIO()` | 创建 PointCloudIO 实例 |

---

## 7. gis 模块

GIS 算法层，提供栅格处理、矢量处理、坐标转换、点云处理功能。

### 7.1 CoordTransform

坐标转换，基于 PROJ 库。

```cpp
class CoordTransform {
public:
    CoordTransform() = default;

    CoordPair Transform(double x, double y, double z,
                        const std::string& src_crs, const std::string& dst_crs);

    std::vector<CoordPair> TransformBatch(const std::vector<CoordPair>& coords,
                                           const std::string& src_crs,
                                           const std::string& dst_crs);

    std::unique_ptr<VectorData> TransformVector(const VectorData& input,
                                                 const std::string& dst_crs);

    std::unique_ptr<PointCloudData> TransformPointCloud(const PointCloudData& input,
                                                         const std::string& dst_crs);
};
```

| 方法 | 说明 |
|------|------|
| `Transform()` | 单点坐标转换。`src_crs`/`dst_crs` 支持 EPSG 代码（如 `"EPSG:4326"`）或 WKT |
| `TransformBatch()` | 批量坐标转换，性能优于逐点调用 |
| `TransformVector()` | 对矢量数据进行坐标转换，返回新 VectorData |
| `TransformPointCloud()` | 对点云数据进行坐标转换，返回新 PointCloudData |

### 7.2 RasterClip

栅格裁剪。

```cpp
class RasterClip {
public:
    RasterClip() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input, const BoundingBox& bbox);
    std::unique_ptr<RasterData> ExecuteByPixel(const RasterData& input,
                                                int x_off, int y_off, int width, int height);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(input, bbox)` | 按地图坐标范围裁剪，`bbox` 使用地图坐标 |
| `ExecuteByPixel(input, x_off, y_off, width, height)` | 按像素偏移裁剪，`x_off`/`y_off` 为左上角像素坐标 |

### 7.3 RasterMosaic

栅格镶嵌（拼接）。

```cpp
class RasterMosaic {
public:
    RasterMosaic() = default;

    std::unique_ptr<RasterData> Execute(
        const std::vector<std::reference_wrapper<const RasterData>>& rasters,
        const MosaicConfig& config = MosaicConfig());
};
```

| 方法 | 说明 |
|------|------|
| `Execute(rasters, config)` | 将多个栅格镶嵌为一个。要求所有输入具有相同波段数和 CRS |

**MosaicConfig**：

```cpp
struct MosaicConfig {
    MosaicStrategy strategy = MosaicStrategy::Overwrite;
    float nodata_value = NaN;   // 无数据值
};
```

| MosaicStrategy | 说明 |
|----------------|------|
| `Overwrite` | 后写入的栅格覆盖先前的值 |
| `First` | 保留最先遇到的非 NoData 值 |
| `Mean` | 取所有非 NoData 值的均值 |
| `Max` | 取最大值 |
| `Min` | 取最小值 |

### 7.4 RasterNormalize

栅格归一化。

```cpp
class RasterNormalize {
public:
    RasterNormalize() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input);
    std::unique_ptr<RasterData> ExecuteMinMax(const RasterData& input,
                                               float min_val, float max_val);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(input)` | 归一化到 [0, 1]，使用数据自身的 min/max |
| `ExecuteMinMax(input, min_val, max_val)` | 按指定范围归一化：`(val - min_val) / (max_val - min_val)` |

### 7.5 RasterResample

栅格重采样。

```cpp
class RasterResample {
public:
    RasterResample() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input,
                                         int new_width, int new_height,
                                         ResampleMethod method = ResampleMethod::Nearest);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(input, new_width, new_height, method)` | 重采样到指定尺寸 |

| ResampleMethod | 适用场景 |
|----------------|----------|
| `Nearest` | 分类图、标签图（不引入新值） |
| `Bilinear` | 连续值影像（DEM、遥感反射率等） |

### 7.6 RasterThreshold

栅格阈值分割。

```cpp
class RasterThreshold {
public:
    RasterThreshold() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input, double threshold,
                                         int band_index = 0);
    std::unique_ptr<RasterData> ExecuteRange(const RasterData& input,
                                              double min_val, double max_val,
                                              int band_index = 0);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(input, threshold, band_index)` | 二值化：大于阈值为 1，否则为 0 |
| `ExecuteRange(input, min_val, max_val, band_index)` | 范围分割：值在 [min_val, max_val] 内为 1，否则为 0 |

### 7.7 VectorBuffer

矢量缓冲区分析。

```cpp
class VectorBuffer {
public:
    VectorBuffer() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& input, double distance,
                                         int quad_segments = 8);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(input, distance, quad_segments)` | 对所有要素生成缓冲区。`distance` 为缓冲距离（地图单位），`quad_segments` 为四分之一圆弧的线段数 |

### 7.8 VectorClip

矢量裁剪。

```cpp
class VectorClip {
public:
    VectorClip() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& target, const VectorData& clipper);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(target, clipper)` | 用 `clipper` 裁剪 `target`，返回裁剪后的要素集合 |

### 7.9 VectorIntersect

矢量相交分析。

```cpp
class VectorIntersect {
public:
    VectorIntersect() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& a, const VectorData& b);
    bool Intersects(const VectorData& a, const VectorData& b);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(a, b)` | 计算两个矢量数据集的交集 |
| `Intersects(a, b)` | 快速判断是否存在相交，不计算交集几何 |

### 7.10 VectorSimplify

矢量简化（Douglas-Peucker 算法）。

```cpp
class VectorSimplify {
public:
    VectorSimplify() = default;

    std::unique_ptr<VectorData> Execute(const VectorData& input, double tolerance);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(input, tolerance)` | 简化矢量要素。`tolerance` 为容差（地图单位），值越大简化程度越高 |

### 7.11 VectorTopology

矢量拓扑检查。

```cpp
class VectorTopology {
public:
    VectorTopology() = default;

    std::vector<TopologyIssue> CheckOverlaps(const VectorData& data);
    std::vector<TopologyIssue> CheckGaps(const VectorData& data, double tolerance = 1e-6);
    std::vector<TopologyIssue> CheckValidity(const VectorData& data);
    std::vector<TopologyIssue> FullCheck(const VectorData& data, double tolerance = 1e-6);
};
```

| 方法 | 说明 |
|------|------|
| `CheckOverlaps()` | 检查要素间重叠 |
| `CheckGaps()` | 检查要素间缝隙，`tolerance` 为缝隙容差 |
| `CheckValidity()` | 检查几何有效性（自相交、环方向等） |
| `FullCheck()` | 执行以上所有检查 |

### 7.12 PcFilter

点云滤波。

```cpp
class PcFilter {
public:
    PcFilter() = default;

    std::unique_ptr<PointCloudData> PassThrough(const PointCloudData& input,
                                                 const PassFilterParams& params);
    std::unique_ptr<PointCloudData> StatisticalOutlierRemoval(
        const PointCloudData& input, int mean_k = 50, double stddev_mul_thresh = 1.0);
};
```

| 方法 | 说明 |
|------|------|
| `PassThrough()` | 直通滤波：保留指定轴上值在 [min_val, max_val] 范围内的点 |
| `StatisticalOutlierRemoval()` | 统计离群值滤波：对每个点计算其 `mean_k` 个近邻的平均距离，移除超过 `stddev_mul_thresh` 倍标准差的点 |

### 7.13 PcDownsample

点云降采样。

```cpp
class PcDownsample {
public:
    PcDownsample() = default;

    std::unique_ptr<PointCloudData> VoxelGrid(const PointCloudData& input, double voxel_size);
    std::unique_ptr<PointCloudData> RandomDownsample(const PointCloudData& input, double ratio);
};
```

| 方法 | 说明 |
|------|------|
| `VoxelGrid(input, voxel_size)` | 体素网格降采样：每个体素内保留一个点（质心） |
| `RandomDownsample(input, ratio)` | 随机降采样：`ratio` 为保留比例，范围 (0, 1] |

---

## 8. ai 模块

AI 推理层，封装 ONNX Runtime 模型管理与推理。

### 8.1 OrtContext

ONNX Runtime 全局上下文，单例模式。管理 `Ort::Env` 和 `SessionOptions`。

```cpp
class OrtContext {
public:
    static OrtContext& Instance();

    Ort::Env& GetEnv();
    Ort::SessionOptions& GetSessionOptions();
    OrtLoggingLevel GetLogLevel() const;
    void SetLogLevel(OrtLoggingLevel level);
};
```

| 方法 | 说明 |
|------|------|
| `Instance()` | 获取单例 |
| `GetEnv()` | 获取 ONNX Runtime 环境 |
| `GetSessionOptions()` | 获取会话选项（可配置线程数、优化级别等） |
| `SetLogLevel()` | 设置 ORT 日志级别 |

> **注意**：通常无需直接使用此类，`ModelManager` 内部会自动使用。

### 8.2 ModelManager

模型管理器，负责加载、缓存和查询 ONNX 模型。

```cpp
struct ModelInfo {
    std::string model_path;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<std::vector<int64_t>> input_shapes;
    std::vector<std::vector<int64_t>> output_shapes;
    void* session_handle = nullptr;
};

class ModelManager {
public:
    ModelManager();
    ~ModelManager();

    int LoadModel(const std::string& model_path, const std::string& model_name = "");
    void UnloadModel(const std::string& model_name);
    ModelInfo* GetModel(const std::string& model_name);
    bool HasModel(const std::string& model_name) const;
    std::vector<std::string> GetLoadedModels() const;
};
```

| 方法 | 说明 |
|------|------|
| `LoadModel(path, name)` | 加载 ONNX 模型。`name` 为空时使用文件名。返回 0 表示成功 |
| `UnloadModel(name)` | 卸载指定模型，释放 Session 资源 |
| `GetModel(name)` | 获取模型信息指针，不存在返回 `nullptr` |
| `HasModel(name)` | 检查模型是否已加载 |
| `GetLoadedModels()` | 获取所有已加载模型的名称列表 |

> **不可拷贝**。每个 `ModelManager` 实例独立管理自己的模型集合。
> **线程安全**：同一 `ModelManager` 实例不应在多线程中并发调用。

### 8.3 InferenceEngine

推理引擎，执行 ONNX 模型推理。

```cpp
struct InferenceResult {
    std::vector<std::vector<float>> outputs;       // 各输出张量数据
    std::vector<std::vector<int64_t>> shapes;      // 各输出张量形状
    double inference_time_ms = 0.0;                // 推理耗时（毫秒）
};

class InferenceEngine {
public:
    explicit InferenceEngine(ModelManager& model_manager);

    InferenceResult Run(const std::string& model_name,
                        const std::vector<float>& input_data,
                        const std::vector<int64_t>& input_shape);

    InferenceResult RunMultiInput(const std::string& model_name,
                                   const std::vector<std::vector<float>>& inputs_data,
                                   const std::vector<std::vector<int64_t>>& inputs_shape);
};
```

| 方法 | 说明 |
|------|------|
| `Run(model_name, input_data, input_shape)` | 单输入推理。`input_shape` 如 `{1, 3, 512, 512}` |
| `RunMultiInput(model_name, inputs_data, inputs_shape)` | 多输入推理 |

> **注意**：`InferenceEngine` 不拥有 `ModelManager`，调用方需确保 `ModelManager` 在引擎生命周期内有效。

### 8.4 Preprocess

数据预处理，将栅格数据转换为模型输入张量。

```cpp
struct PreprocessConfig {
    int target_width = 512;
    int target_height = 512;
    int target_channels = 3;
    NormalizeMode normalize_mode = NormalizeMode::ImageNet;
    bool input_is_uint8 = true;
    float mean_r = 0.485f, mean_g = 0.456f, mean_b = 0.406f;
    float std_r = 0.229f, std_g = 0.224f, std_b = 0.225f;
};

class Preprocess {
public:
    Preprocess() = default;

    std::vector<float> RasterToTensor(const RasterData& raster,
                                       const PreprocessConfig& config = PreprocessConfig());

    std::vector<float> RasterBandToTensor(const RasterData& raster,
                                           int band_index, int target_size);

    static std::vector<int64_t> GetInputShape(const PreprocessConfig& config);
};
```

| 方法 | 说明 |
|------|------|
| `RasterToTensor(raster, config)` | 将栅格转为 NCHW 张量（resize + normalize）。返回 `{1, C, H, W}` 大小的 float 向量 |
| `RasterBandToTensor(raster, band_index, target_size)` | 提取单波段并 resize 到正方形 |
| `GetInputShape(config)` | 根据 config 计算输入形状，如 `{1, 3, 512, 512}` |

**NormalizeMode 说明**：

| 模式 | 公式 |
|------|------|
| `None` | 仅做类型转换，不归一化 |
| `ImageNet` | `(pixel / 255.0 - mean) / std` |
| `MinMax01` | `(pixel - min) / (max - min)` |
| `ZScore` | `(pixel - mean) / stddev` |
| `Custom` | 使用 `PreprocessConfig` 中的 mean/std 参数 |

### 8.5 Postprocess

推理结果后处理。

```cpp
class Postprocess {
public:
    Postprocess() = default;

    std::vector<uint8_t> SigmoidArgmax(const std::vector<float>& output_data,
                                        int64_t height, int64_t width, int64_t num_classes);

    std::vector<float> Sigmoid(const std::vector<float>& logits);

    std::vector<uint8_t> ThresholdMask(const std::vector<float>& mask,
                                        float threshold = 0.5f);

    RasterData MaskToRaster(const std::vector<uint8_t>& mask, int width, int height,
                             const double* geotransform, const std::string& projection);
};
```

| 方法 | 说明 |
|------|------|
| `SigmoidArgmax()` | 对多类输出先 Sigmoid 再 Argmax，返回每个像素的类别标签 |
| `Sigmoid()` | 对 logits 执行 Sigmoid 激活 |
| `ThresholdMask()` | 对概率掩膜二值化，大于 `threshold` 为 1 |
| `MaskToRaster()` | 将二值掩膜转为 `RasterData`，继承地理信息 |

### 8.6 MaskToPolygon

掩膜转矢量多边形。

```cpp
class MaskToPolygon {
public:
    MaskToPolygon() = default;

    std::unique_ptr<VectorData> Execute(const std::vector<uint8_t>& mask,
                                         int width, int height,
                                         const double* geotransform,
                                         uint8_t target_class = 1);

    std::unique_ptr<VectorData> ExecuteFromRaster(const RasterData& raster,
                                                    uint8_t target_class = 1);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(mask, width, height, geotransform, target_class)` | 从二值掩膜提取指定类别的多边形 |
| `ExecuteFromRaster(raster, target_class)` | 从栅格数据直接提取多边形（使用栅格的 geotransform） |

> `geotransform` 为 6 元素数组，将像素坐标映射为地图坐标。

---

## 9. fusion 模块

融合层，编排 AI 推理与 GIS 处理，提供端到端的高层接口。

### 9.1 RasterSeg

单图语义分割，适用于影像尺寸不超过模型输入分辨率的情况。

```cpp
struct RasterSegConfig {
    int input_size = 512;
    float mean_r = 0.485f, mean_g = 0.456f, mean_b = 0.406f;
    float std_r = 0.229f, std_g = 0.224f, std_b = 0.225f;
    float mask_threshold = 0.5f;
    uint8_t target_class = 1;
};

class RasterSeg {
public:
    explicit RasterSeg(const std::string& model_path);
    ~RasterSeg();

    std::unique_ptr<RasterData> Segment(const RasterData& input);
    std::unique_ptr<VectorData> SegmentToPolygon(const RasterData& input);
    int SegmentToFile(const std::string& input_path,
                       const std::string& output_tif,
                       const std::string& output_shp = "");
    double GetLastInferenceTimeMs() const;
};
```

| 方法 | 说明 |
|------|------|
| `Segment(input)` | 对栅格执行语义分割，返回分割掩膜栅格 |
| `SegmentToPolygon(input)` | 分割并转为矢量多边形 |
| `SegmentToFile(input_path, output_tif, output_shp)` | 端到端：加载→分割→保存。`output_shp` 为空时不生成矢量。返回 0 表示成功 |
| `GetLastInferenceTimeMs()` | 获取最近一次推理耗时 |

> **不可拷贝**。内部持有 `ModelManager` 和 `InferenceEngine`。

### 9.2 LargeImageSeg

大图语义分割，使用滑窗推理 + 混合策略处理大幅遥感影像。

```cpp
struct LargeImageSegConfig {
    int tile_size = 512;                         // 分块大小
    int stride = 384;                            // 滑窗步长（< tile_size 产生重叠）
    int target_channels = 3;
    NormalizeMode normalize_mode = NormalizeMode::ImageNet;
    bool input_is_uint8 = true;
    float mean_r = 0.485f, mean_g = 0.456f, mean_b = 0.406f;
    float std_r = 0.229f, std_g = 0.224f, std_b = 0.225f;
    float mask_threshold = 0.5f;
    uint8_t target_class = 1;
    BlendMode blend_mode = BlendMode::Gaussian;  // 混合模式
    float gaussian_sigma = 0.5f;                 // 高斯混合参数
    bool skip_nodata = true;                     // 跳过全 NoData 分块
    float nodata_value = 0.0f;                   // NoData 值
    double min_polygon_area = 100.0;             // 最小多边形面积过滤
    double simplify_tolerance = 1.0;             // 矢量简化容差
    bool fix_topology = true;                    // 是否修复拓扑错误
};

struct SegmentationStats {
    int total_tiles = 0;
    int skipped_tiles = 0;
    int inferred_tiles = 0;
    double total_inference_time_ms = 0.0;
    double total_time_ms = 0.0;
    int polygon_count = 0;
    double total_polygon_area = 0.0;
    int class_counts[256] = {};
};

class LargeImageSeg {
public:
    explicit LargeImageSeg(const std::string& model_path);
    ~LargeImageSeg();

    std::unique_ptr<RasterData> Segment(const RasterData& input,
                                         const LargeImageSegConfig& config = LargeImageSegConfig());

    std::unique_ptr<VectorData> SegmentToPolygon(const RasterData& input,
                                                   const LargeImageSegConfig& config = LargeImageSegConfig());

    std::unique_ptr<VectorData> SegmentToPolygon(const RasterData& input,
                                                   const RasterData& mask_raster,
                                                   const LargeImageSegConfig& config = LargeImageSegConfig());

    int SegmentToFile(const std::string& input_path,
                       const std::string& output_tif,
                       const std::string& output_shp = "",
                       const LargeImageSegConfig& config = LargeImageSegConfig());

    SegmentationStats GetLastStats() const;
    void SetProgressCallback(std::function<void(int, int, const std::string&)> callback);
};
```

| 方法 | 说明 |
|------|------|
| `Segment(input, config)` | 大图滑窗分割，返回完整分割掩膜 |
| `SegmentToPolygon(input, config)` | 分割并转为矢量多边形（含面积过滤和简化） |
| `SegmentToPolygon(input, mask_raster, config)` | 使用已有掩膜栅格转矢量 |
| `SegmentToFile(...)` | 端到端：加载→分割→保存。返回 0 表示成功 |
| `GetLastStats()` | 获取最近一次分割的统计信息 |
| `SetProgressCallback(cb)` | 设置进度回调：`cb(current_tile, total_tiles, message)` |

**BlendMode 说明**：

| 模式 | 说明 |
|------|------|
| `None` | 直接覆盖，重叠区取最后推理结果 |
| `Linear` | 线性渐变混合，重叠区按距离线性插值 |
| `Gaussian` | 高斯渐变混合，重叠区按高斯权重融合（推荐，效果最佳） |

> **stride 与 tile_size 的关系**：`stride < tile_size` 时产生重叠区域，重叠区域通过 BlendMode 混合，消除拼接缝隙。推荐 `stride = tile_size * 3/4`。

### 9.3 BatchProcessor

批量处理器，支持多线程并行处理多幅影像。

```cpp
struct BatchResult {
    std::string input_path;
    std::string output_tif_path;
    std::string output_shp_path;
    bool success = false;
    double inference_time_ms = 0.0;
    std::string error_message;
};

class BatchProcessor {
public:
    explicit BatchProcessor(const std::string& model_path, int num_threads = 1);
    ~BatchProcessor();

    std::vector<BatchResult> ProcessDirectory(const std::string& input_dir,
                                               const std::string& output_dir,
                                               bool generate_shp = true);

    std::vector<BatchResult> ProcessFiles(const std::vector<std::string>& input_files,
                                           const std::string& output_dir,
                                           bool generate_shp = true);

    void SetProgressCallback(std::function<void(int, int, const std::string&)> callback);
    void SetSegConfig(const LargeImageSegConfig& config);
    LargeImageSegConfig& SegConfig();
};
```

| 方法 | 说明 |
|------|------|
| `ProcessDirectory(input_dir, output_dir, generate_shp)` | 处理目录下所有支持的影像文件 |
| `ProcessFiles(input_files, output_dir, generate_shp)` | 处理指定文件列表 |
| `SetProgressCallback(cb)` | 设置全局进度回调 |
| `SetSegConfig(config)` | 设置分割配置 |
| `SegConfig()` | 获取配置引用，可直接修改 |

> **线程安全**：`num_threads > 1` 时，每个线程创建独立的 `LargeImageSeg` 实例，共享模型文件路径。进度回调通过 mutex 保护。

### 9.4 TaskConfig / TaskRunner

任务配置与执行器，支持通过 JSON 配置文件驱动任务。

#### TaskType 枚举

| 值 | 说明 |
|----|------|
| `Segment` | 单图分割 |
| `SegmentToPolygon` | 单图分割转矢量 |
| `BatchSegment` | 批量分割 |
| `BatchInference` | 批量推理 |
| `Inference` | 单次推理 |
| `Preprocess` | 数据预处理 |
| `VectorSimplify` | 矢量简化 |
| `VectorBuffer` | 矢量缓冲区 |
| `RasterMosaic` | 栅格镶嵌 |
| `RasterResample` | 栅格重采样 |

#### TaskConfig

```cpp
struct TaskConfig {
    TaskType task_type = TaskType::Segment;
    std::string model_path;
    std::string input_path;
    std::string output_tif_path;
    std::string output_shp_path;
    std::string input_dir;
    std::string output_dir;
    std::string output_path;
    LargeImageSegConfig seg_config;
    double resample_resolution = 0.0;
    std::string resample_method = "nearest";
    double simplify_tolerance = 1.0;
    double buffer_distance = 0.0;
    int num_threads = 1;
    bool verbose = false;
    std::string log_file = "gis_ai.log";

    static TaskConfig LoadFromFile(const std::string& path);
    static TaskConfig LoadFromString(const std::string& json_str);
    void SaveToFile(const std::string& path) const;
    std::string ToString() const;
};
```

| 方法 | 说明 |
|------|------|
| `LoadFromFile(path)` | 从 JSON 文件加载任务配置 |
| `LoadFromString(json_str)` | 从 JSON 字符串加载 |
| `SaveToFile(path)` | 保存配置到 JSON 文件 |
| `ToString()` | 转为可读字符串 |

#### TaskReport

```cpp
struct TaskReport {
    bool success = false;
    std::string error_message;
    double total_time_ms = 0.0;
    SegmentationStats seg_stats;
    std::vector<std::string> output_files;
    std::string task_type_name;
    std::string input_path;
    std::string start_time;
    std::string end_time;
    int error_code = 0;

    std::string ToString() const;
    std::string ToJson() const;
    void SaveReport(const std::string& path) const;
};
```

#### TaskRunner

```cpp
class TaskRunner {
public:
    static TaskReport Execute(const TaskConfig& config);
};
```

| 方法 | 说明 |
|------|------|
| `Execute(config)` | 根据 TaskConfig 执行任务，返回 TaskReport |

---

## 10. 使用示例

### 10.1 配置与日志

```cpp
#include "core/config.h"
#include "core/logger.h"

int main() {
    // 初始化日志
    gis_ai::Logger::Instance().Initialize("app.log");

    // 使用配置
    auto& config = gis_ai::Config::Instance();
    config.Set("input_dir", std::string("/data/input"));
    config.Set("tile_size", 512);
    config.Set("use_gpu", false);

    LOG_INFO("tile_size = " + std::to_string(config.GetInt("tile_size")));

    // 持久化配置
    config.SaveToFile("config.json");

    return 0;
}
```

### 10.2 GIS 栅格操作

```cpp
#include "core/logger.h"
#include "core/exception.h"
#include "io/raster_io.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_clip.h"

int main() {
    gis_ai::Logger::Instance().Initialize("demo.log");

    try {
        gis_ai::RasterIO io;
        auto raster = io.Load("input.tif");

        LOG_INFO("尺寸: " + std::to_string(raster->width) + "x"
                 + std::to_string(raster->height));

        // 重采样
        gis_ai::RasterResample resample;
        auto resampled = resample.Execute(*raster, 256, 256,
                                           gis_ai::ResampleMethod::Bilinear);

        // 归一化
        gis_ai::RasterNormalize normalize;
        auto normalized = normalize.Execute(*raster);

        // 像素裁剪
        gis_ai::RasterClip clip;
        auto clipped = clip.ExecuteByPixel(*raster, 0, 0, 100, 100);

        // 保存
        io.Save(*resampled, "resampled.tif");
        io.Save(*normalized, "normalized.tif");
        io.Save(*clipped, "clipped.tif");

    } catch (const gis_ai::GisAiIOException& e) {
        LOG_ERROR("IO 错误: " + std::string(e.what()));
    } catch (const gis_ai::GisAiAlgorithmException& e) {
        LOG_ERROR("算法错误: " + std::string(e.what()));
    }

    return 0;
}
```

### 10.3 语义分割

```cpp
#include "core/logger.h"
#include "core/exception.h"
#include "fusion/raster_seg.h"

int main() {
    gis_ai::Logger::Instance().Initialize("seg.log");

    try {
        gis_ai::RasterSeg seg("model.onnx");

        // 方式一：端到端文件处理
        int ret = seg.SegmentToFile("input.tif", "output.tif", "output.shp");
        if (ret == 0) {
            LOG_INFO("推理耗时: " + std::to_string(seg.GetLastInferenceTimeMs()) + " ms");
        }

        // 方式二：内存中处理
        gis_ai::RasterIO io;
        auto raster = io.Load("input.tif");

        auto mask = seg.Segment(*raster);
        io.Save(*mask, "mask.tif");

        auto polygons = seg.SegmentToPolygon(*raster);
        gis_ai::VectorIO vio;
        vio.Save(*polygons, "polygons.shp");

    } catch (const gis_ai::GisAiModelException& e) {
        LOG_ERROR("模型错误: " + std::string(e.what()));
    }

    return 0;
}
```

### 10.4 大图分割

```cpp
#include "core/logger.h"
#include "fusion/large_image_seg.h"

int main() {
    gis_ai::Logger::Instance().Initialize("large_seg.log");

    gis_ai::LargeImageSeg seg("model.onnx");

    // 配置
    gis_ai::LargeImageSegConfig config;
    config.tile_size = 512;
    config.stride = 384;
    config.blend_mode = gis_ai::BlendMode::Gaussian;
    config.gaussian_sigma = 0.5f;
    config.skip_nodata = true;
    config.min_polygon_area = 100.0;
    config.simplify_tolerance = 1.0;

    // 设置进度回调
    seg.SetProgressCallback([](int current, int total, const std::string& msg) {
        LOG_INFO("进度: " + std::to_string(current) + "/" + std::to_string(total)
                 + " " + msg);
    });

    try {
        int ret = seg.SegmentToFile("large_image.tif",
                                     "output_mask.tif",
                                     "output_polygons.shp",
                                     config);

        auto stats = seg.GetLastStats();
        LOG_INFO("总分块: " + std::to_string(stats.total_tiles)
                 + ", 推理分块: " + std::to_string(stats.inferred_tiles)
                 + ", 跳过分块: " + std::to_string(stats.skipped_tiles)
                 + ", 推理耗时: " + std::to_string(stats.total_inference_time_ms) + " ms"
                 + ", 多边形数: " + std::to_string(stats.polygon_count));

    } catch (const gis_ai::GisAiException& e) {
        LOG_ERROR("分割失败: " + std::string(e.what()));
    }

    return 0;
}
```

### 10.5 批量处理

```cpp
#include "core/logger.h"
#include "fusion/batch_processor.h"

int main() {
    gis_ai::Logger::Instance().Initialize("batch.log");

    // 4 线程并行
    gis_ai::BatchProcessor processor("model.onnx", 4);

    // 配置分割参数
    auto& config = processor.SegConfig();
    config.tile_size = 512;
    config.stride = 384;
    config.blend_mode = gis_ai::BlendMode::Gaussian;

    // 进度回调
    processor.SetProgressCallback([](int current, int total, const std::string& msg) {
        LOG_INFO("批量进度: " + std::to_string(current) + "/" + std::to_string(total));
    });

    // 处理整个目录
    auto results = processor.ProcessDirectory("/data/input", "/data/output", true);

    int success_count = 0;
    for (const auto& r : results) {
        if (r.success) {
            success_count++;
        } else {
            LOG_ERROR("失败: " + r.input_path + " - " + r.error_message);
        }
    }
    LOG_INFO("完成: " + std::to_string(success_count) + "/"
             + std::to_string(results.size()));

    return 0;
}
```

### 10.6 任务配置驱动

```cpp
#include "fusion/task_config.h"
#include "core/logger.h"

int main() {
    gis_ai::Logger::Instance().Initialize("task.log");

    // 从 JSON 配置创建任务
    gis_ai::TaskConfig config;
    config.task_type = gis_ai::TaskType::SegmentToPolygon;
    config.model_path = "model.onnx";
    config.input_path = "input.tif";
    config.output_tif_path = "output.tif";
    config.output_shp_path = "output.shp";
    config.seg_config.tile_size = 512;
    config.seg_config.stride = 384;
    config.seg_config.blend_mode = gis_ai::BlendMode::Gaussian;

    // 保存配置供后续使用
    config.SaveToFile("task_config.json");

    // 执行任务
    gis_ai::TaskReport report = gis_ai::TaskRunner::Execute(config);

    if (report.success) {
        LOG_INFO("任务完成，耗时: " + std::to_string(report.total_time_ms) + " ms");
    } else {
        LOG_ERROR("任务失败: " + report.error_message);
    }

    // 也可以从文件加载配置
    auto loaded_config = gis_ai::TaskConfig::LoadFromFile("task_config.json");
    report = gis_ai::TaskRunner::Execute(loaded_config);

    // 导出报告
    report.SaveReport("report.json");

    return 0;
}
```

---

## 附录：头文件索引

| 模块 | 头文件 | 主要内容 |
|------|--------|----------|
| core | `core/logger.h` | Logger 单例、日志宏 |
| core | `core/exception.h` | ErrorCode、GisAiException 层次 |
| core | `core/config.h` | Config 单例、JSON 配置 |
| core | `core/memory.h` | MemoryPool、MakeUnique/MakeShared |
| core | `core/platform.h` | Platform 静态工具、平台宏 |
| io | `io/raster_io.h` | RasterData、RasterIO |
| io | `io/vector_io.h` | VectorData、Feature、VectorIO |
| io | `io/pointcloud_io.h` | PointCloudData、PointCloudIO |
| io | `io/io_factory.h` | IOFactory 工厂 |
| ai | `ai/ort_context.h` | OrtContext 单例 |
| ai | `ai/model_manager.h` | ModelInfo、ModelManager |
| ai | `ai/inference_engine.h` | InferenceResult、InferenceEngine |
| ai | `ai/preprocess.h` | PreprocessConfig、Preprocess |
| ai | `ai/postprocess.h` | Postprocess |
| ai | `ai/mask_to_polygon.h` | MaskToPolygon |
| gis | `gis/coord_transform.h` | CoordPair、CoordTransform |
| gis | `gis/raster_clip.h` | BoundingBox、RasterClip |
| gis | `gis/raster_mosaic.h` | MosaicStrategy、MosaicConfig、RasterMosaic |
| gis | `gis/raster_normalize.h` | RasterNormalize |
| gis | `gis/raster_resample.h` | ResampleMethod、RasterResample |
| gis | `gis/raster_threshold.h` | RasterThreshold |
| gis | `gis/vector_buffer.h` | VectorBuffer |
| gis | `gis/vector_clip.h` | VectorClip |
| gis | `gis/vector_intersect.h` | VectorIntersect |
| gis | `gis/vector_simplify.h` | VectorSimplify |
| gis | `gis/vector_topology.h` | TopologyIssue、VectorTopology |
| gis | `gis/pc_filter.h` | PassFilterParams、PcFilter |
| gis | `gis/pc_downsample.h` | PcDownsample |
| fusion | `fusion/raster_seg.h` | RasterSegConfig、RasterSeg |
| fusion | `fusion/large_image_seg.h` | BlendMode、LargeImageSegConfig、SegmentationStats、LargeImageSeg |
| fusion | `fusion/batch_processor.h` | BatchResult、BatchProcessor |
| fusion | `fusion/task_config.h` | TaskType、TaskConfig、TaskReport、TaskRunner |
