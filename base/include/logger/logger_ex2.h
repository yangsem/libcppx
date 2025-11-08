#ifndef __CPPX_LOGGER_EX2_H__
#define __CPPX_LOGGER_EX2_H__

#include <logger/logger.h>
#include <utilities/common.h>
#include <utilities/error_code.h>

#define LOG_BASE2(pLogger, eLevel, iErrorNo, fmt, ...)                         \
  {                                                                            \
    if (likely(eLevel > cppx::base::ILogger::LogLevel::kInfo &&                \
               eLevel < cppx::base::ILogger::LogLevel::kEvent)) {              \
      ::cppx::base::SetLastError(iErrorNo);                                    \
    }                                                                          \
    if (likely(pLogger != nullptr && eLevel >= pLogger->GetLogLevel())) {      \
      pLogger->LogFormat(iErrorNo, eLevel, "[%s] " fmt "(%s,%s)", kModuleName, \
                         ##__VA_ARGS__, __POSITION__);                         \
    }                                                                          \
  }

#define LOG_TRACE2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kTrace, iErrorNo, fmt, ##__VA_ARGS__) // 跟踪
#define LOG_DEBUG2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kDebug, iErrorNo, fmt, ##__VA_ARGS__) // 调试
#define LOG_INFO2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kInfo, iErrorNo, fmt, ##__VA_ARGS__) // 信息
#define LOG_WARN2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kWarn, iErrorNo, fmt, ##__VA_ARGS__) // 警告
#define LOG_ERROR2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kError, iErrorNo, fmt, ##__VA_ARGS__) // 错误
#define LOG_FATAL2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kFatal, iErrorNo, fmt, ##__VA_ARGS__) // 致命错误
#define LOG_EVENT2(pLogger, iErrorNo, fmt, ...) LOG_BASE2(pLogger, cppx::base::ILogger::LogLevel::kEvent, iErrorNo, fmt, ##__VA_ARGS__) // 事件


#endif // __CPPX_LOGGER_EX2_H__