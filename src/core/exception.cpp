#include "core/exception.h"

namespace gis_ai {

GisAiException::GisAiException(ErrorCode code, const std::string& message,
                               const std::string& context)
    : std::runtime_error(
          context.empty()
              ? message
              : message + " [context: " + context + "]"),
      code_(code),
      context_(context) {}

int ErrorCodeToInt(ErrorCode code) {
    return static_cast<int>(code);
}

std::string ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:              return "成功";
        case ErrorCode::IO:                   return "IO 错误";
        case ErrorCode::FileNotFound:         return "文件未找到";
        case ErrorCode::FileReadError:        return "文件读取失败";
        case ErrorCode::FileWriteError:       return "文件写入失败";
        case ErrorCode::ModelLoad:            return "模型加载失败";
        case ErrorCode::Inference:            return "推理执行失败";
        case ErrorCode::Algorithm:            return "算法执行失败";
        case ErrorCode::Config:               return "配置错误";
        case ErrorCode::ConfigParseError:     return "配置解析失败";
        case ErrorCode::ConfigValidationError: return "配置校验失败";
        case ErrorCode::Memory:               return "内存不足";
        case ErrorCode::InvalidParam:         return "参数无效";
        case ErrorCode::MissingRequiredParam: return "缺少必填参数";
        case ErrorCode::TaskExecution:        return "任务执行失败";
        case ErrorCode::TaskCancelled:        return "任务已取消";
        case ErrorCode::Unknown:              return "未知错误";
        default:                              return "未定义错误";
    }
}

} // namespace gis_ai
