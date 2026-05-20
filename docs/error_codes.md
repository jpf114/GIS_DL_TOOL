# GIS AI Tool 错误码参考

## 错误码体系

错误码采用 4 位数字编码，按类别分段：

| 范围 | 类别 | 说明 |
|------|------|------|
| 0 | 成功 | 操作成功完成 |
| 1xxx | I/O 错误 | 文件读写、数据存取相关错误 |
| 2xxx | 模型/推理错误 | 模型加载、推理执行相关错误 |
| 3xxx | 算法错误 | GIS 算法执行相关错误 |
| 4xxx | 配置错误 | 配置解析、校验相关错误 |
| 5xxx | 内存错误 | 内存分配相关错误 |
| 6xxx | 参数错误 | 参数校验相关错误 |
| 7xxx | 任务执行错误 | 任务生命周期相关错误 |
| 9999 | 未知错误 | 未分类的错误 |

## 错误码详细说明

### 0 - 成功

| 错误码 | 名称 | 说明 |
|--------|------|------|
| 0 | Success | 操作成功完成 |

### 1xxx - I/O 错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 1001 | IO | 通用 I/O 错误 | 检查文件路径和权限 |
| 1002 | FileNotFound | 文件不存在 | 确认文件路径是否正确 |
| 1003 | FileReadError | 文件读取失败 | 检查文件是否被占用或损坏 |
| 1004 | FileWriteError | 文件写入失败 | 检查目标目录权限和磁盘空间 |

**典型触发位置：**

- `RasterIO::Load` — 无法打开栅格文件 (1001)
- `RasterIO::Load` — 无法读取波段数据 (1001)
- `RasterIO::Save` — GTiff 驱动不可用 (1001)
- `RasterIO::Save` — 无法创建输出文件 (1004)
- `RasterIO::Save` — 无法设置地理变换 (1004)
- `RasterIO::Save` — 无法设置投影信息 (1004)
- `RasterIO::Save` — 无法写入波段数据 (1004)

### 2xxx - 模型/推理错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 2001 | ModelLoad | 模型文件加载失败 | 确认模型文件路径和格式是否正确 |
| 2002 | Inference | 推理执行失败 | 检查输入数据格式和模型兼容性 |

**典型触发位置：**

- `LargeImageSeg::LargeImageSeg` — 模型文件不存在 (2001)
- `ModelManager::LoadModel` — 模型文件不存在 (2001)
- `ModelManager::LoadModel` — ONNX 模型加载失败 (2001)
- `InferenceEngine::Run` — 模型未加载 (2001)
- `InferenceEngine::Run` — 输入名称不匹配 (2002)
- `InferenceEngine::RunBatch` — 批量推理输入维度不匹配 (2002)
- `LargeImageSeg` — 分块推理失败 (2002)

### 3xxx - 算法错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 3001 | Algorithm | 算法执行过程中的逻辑错误 | 检查输入数据是否符合算法要求 |

**典型触发位置：**

- `Preprocess::RasterToTensor` — 输入栅格无波段 (3001)
- `Preprocess::RasterBandToTensor` — 波段索引越界 (3001)
- `RasterNormalize::Execute` — 输入栅格无波段 (3001)
- `RasterNormalize::ExecuteMinMax` — max_val 必须大于 min_val (3001)
- `RasterMosaic::Execute` — 未提供输入栅格 (3001)
- `RasterMosaic::Execute` — 栅格波段数不一致 (3001)
- `RasterMosaic::Execute` — 像素尺寸无效 (3001)
- `RasterResample::Execute` — 目标尺寸必须为正数 (3001)
- `RasterResample::Execute` — 输入栅格尺寸无效 (3001)
- `RasterThreshold::Execute` — 波段索引越界 (3001)
- `RasterThreshold::ExecuteRange` — min_val 必须小于等于 max_val (3001)
- `VectorSimplify::Execute` — 输入要素为空 (3001)
- `VectorSimplify::Execute` — 容差必须非负 (3001)
- `VectorSimplify::Execute` — 无法创建 GEOS 上下文 (3001)
- `VectorTopology::CheckOverlaps` — 输入要素为空 (3001)
- `BatchProcessor::Execute` — 输入目录不存在 (3001)
- `CoordTransform` — 无法创建坐标转换 (3001)

### 4xxx - 配置错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 4001 | Config | 配置文件缺失或格式错误 | 检查配置文件路径和 JSON 格式 |
| 4002 | ConfigParseError | 配置解析失败 | 检查 JSON 语法和字段类型 |
| 4003 | ConfigValidationError | 配置校验失败 | 检查必填字段和参数约束 |

**典型触发位置：**

- `TaskConfig::LoadFromFile` — 配置文件不存在 (4001)
- `TaskConfig::LoadFromFile` — 无法打开配置文件 (4001)
- `TaskConfig::LoadFromString` — JSON 解析失败 (4001)
- `TaskConfig::LoadFromString` — 未知的 task_type (4001)
- `TaskConfig::SaveToFile` — 无法打开文件写入 (4001)
- `TaskRunner::Execute` — segment 缺少 model_path (4001)
- `TaskRunner::Execute` — segment 缺少 input_path (4001)
- `TaskRunner::Execute` — batch_segment 缺少 model_path (4001)
- `TaskRunner::Execute` — batch_segment 缺少 input_dir (4001)
- `TaskRunner::Execute` — batch_inference 缺少 model_path (4001)
- `TaskRunner::Execute` — batch_inference 缺少 input_dir (4001)
- `TaskRunner::Execute` — inference 缺少 model_path (4001)
- `TaskRunner::Execute` — inference 缺少 input_path (4001)
- `TaskRunner::Execute` — preprocess 缺少 input_path (4001)
- `TaskRunner::Execute` — vector_simplify 缺少 input_path (4001)
- `TaskRunner::Execute` — vector_buffer 缺少 input_path (4001)
- `TaskRunner::Execute` — raster_mosaic 缺少 input_dir (4001)
- `TaskRunner::Execute` — raster_mosaic 目录中无 TIFF 文件 (4001)
- `TaskRunner::Execute` — raster_resample 缺少 input_path (4001)
- `TaskRunner::Execute` — resample_resolution 必须为正数 (4001)

### 5xxx - 内存错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 5001 | Memory | 内存分配失败 | 减小处理数据范围或使用分块处理 |

### 6xxx - 参数错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 6001 | InvalidParam | 参数值无效 | 检查参数取值范围 |
| 6002 | MissingRequiredParam | 缺少必填参数 | 补充必填参数值 |

### 7xxx - 任务执行错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 7001 | TaskExecution | 任务执行失败 | 查看详细错误信息 |
| 7002 | TaskCancelled | 任务被用户取消 | 正常行为，无需处理 |

### 9999 - 未知错误

| 错误码 | 名称 | 触发场景 | 建议处理 |
|--------|------|----------|----------|
| 9999 | Unknown | 未分类的异常 | 提交 Issue 并附上复现步骤 |

## 异常类层次

```
std::runtime_error
  └── GisAiException          (基类，携带 ErrorCode + context)
        ├── GisAiIOException       (ErrorCode::IO)
        ├── GisAiModelException    (ErrorCode::ModelLoad)
        ├── GisAiAlgorithmException (ErrorCode::Algorithm)
        └── GisAiConfigException   (ErrorCode::Config)
```

## CLI 退出码映射

CLI 程序的退出码直接映射到错误码的整数值：

```bash
# 成功
gis-ai-gui run config.json && echo "成功 (退出码 0)"

# 配置错误
gis-ai-gui run bad_config.json
echo $?  # 输出 4001

# 模型加载失败
gis-ai-gui run missing_model_config.json
echo $?  # 输出 2001
```
