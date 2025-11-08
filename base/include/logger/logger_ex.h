#ifndef __CPPX_LOGGER_EX_H__
#define __CPPX_LOGGER_EX_H__

#include <type_traits>
#include <cinttypes>
#include <logger/logger.h>
#include <utilities/common.h>
#include <utilities/error_code.h>

namespace cppx
{
namespace base
{

namespace wrap_detail
{

/* ============================== 辅助模板 ============================== */
template<typename T>
struct FormatHelper;

template<> struct FormatHelper<uint8_t>  { static const char* GetFormat() { return "%u"; } };
template<> struct FormatHelper<uint16_t> { static const char* GetFormat() { return "%u"; } };
template<> struct FormatHelper<uint32_t> { static const char* GetFormat() { return "%u"; } };
template<> struct FormatHelper<uint64_t> { static const char* GetFormat() { return "%" PRIu64; } };
template<> struct FormatHelper<int8_t>   { static const char* GetFormat() { return "%d"; } };
template<> struct FormatHelper<int16_t>  { static const char* GetFormat() { return "%d"; } };
template<> struct FormatHelper<int32_t>  { static const char* GetFormat() { return "%d"; } };
template<> struct FormatHelper<int64_t>  { static const char* GetFormat() { return "%" PRId64; } };
template<> struct FormatHelper<float>    { static const char* GetFormat() { return "%f"; } };
template<> struct FormatHelper<double>   { static const char* GetFormat() { return "%f"; } };

// bool类型的特殊处理
template<typename T>
inline void FormatValue(char* buffer, uint32_t size, const T& value, 
                        typename std::enable_if<!std::is_same<T, bool>::value>::type* = 0)
{
    snprintf(buffer, size, FormatHelper<T>::GetFormat(), value);
}

template<typename T>
inline void FormatValue(char* buffer, uint32_t size, const T& value,
                        typename std::enable_if<std::is_same<T, bool>::value>::type* = 0)
{
    snprintf(buffer, size, "%s", value ? "true" : "false");
}

// 根据类型获取推荐的缓冲区大小
template<typename T>
struct SizeHelper;

template<> struct SizeHelper<uint8_t>  { enum { value = 8 }; };
template<> struct SizeHelper<uint16_t> { enum { value = 8 }; };
template<> struct SizeHelper<uint32_t> { enum { value = 16 }; };
template<> struct SizeHelper<uint64_t> { enum { value = 32 }; };
template<> struct SizeHelper<int8_t>   { enum { value = 8 }; };
template<> struct SizeHelper<int16_t>  { enum { value = 8 }; };
template<> struct SizeHelper<int32_t>  { enum { value = 16 }; };
template<> struct SizeHelper<int64_t>  { enum { value = 32 }; };
template<> struct SizeHelper<float>    { enum { value = 32 }; };
template<> struct SizeHelper<double>   { enum { value = 32 }; };
template<> struct SizeHelper<bool>     { enum { value = 8 }; };

}

template<uint32_t Size, typename T>
class Wrap
{
public:
    Wrap(const T &value)
    {
        wrap_detail::FormatValue(m_szBuffer, Size, value);
    }

    operator const char *() const { return m_szBuffer; }

private:
    char m_szBuffer[Size];
};

using WrapU8 = Wrap<8, uint8_t>;
using WrapU16 = Wrap<16, uint16_t>;
using WrapU32 = Wrap<32, uint32_t>;
using WrapU64 = Wrap<64, uint64_t>;
using WrapI8 = Wrap<8, int8_t>;
using WrapI16 = Wrap<16, int16_t>;
using WrapI32 = Wrap<32, int32_t>;
using WrapI64 = Wrap<64, int64_t>;
using WrapF = Wrap<32, float>;
using WrapD = Wrap<32, double>;
using WrapB = Wrap<8, bool>;

// 自动类型推断宏：根据参数类型自动选择对应的Wrap实现
// 使用方式：Wrap(a) 会自动推断类型并返回对应的Wrap实例
#define Wrap(val)                                                              \
  (::lite_drive::logger::Wrap<                                                 \
      ::lite_drive::logger::wrap_detail::SizeHelper<                           \
          typename std::decay<decltype(val)>::type>::value,                    \
      typename std::decay<decltype(val)>::type>(val))

/* ============================== 日志宏 ============================== */
#define LOG_BASE(pLogger, eLevel, iErrorNo, fmt, ...)                          \
  {                                                                            \
    if (likely(eLevel > cppx::base::ILogger::LogLevel::kInfo &&                \
               eLevel < cppx::base::ILogger::LogLevel::kEvent)) {              \
      ::cppx::base::SetLastError(iErrorNo);                                    \
    }                                                                          \
    if (likely(pLogger != nullptr && eLevel >= pLogger->GetLogLevel())) {      \
      const char *pParams[] = {"", ##__VA_ARGS__};                             \
      pLogger->Log(iErrorNo, eLevel, kModuleName, _POSITION_STRING, fmt,       \
                   &pParams[1], sizeof(pParams) / sizeof(pParams[0]) - 1);     \
    }                                                                          \
  }

#define LOG_TRACE(_logger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kTrace, iErrorNo, fmt, ##__VA_ARGS__) // 跟踪
#define LOG_DEBUG(pLogger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kDebug, iErrorNo, fmt, ##__VA_ARGS__) // 调试
#define LOG_INFO(pLogger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kInfo, iErrorNo, fmt, ##__VA_ARGS__)   // 信息
#define LOG_WARN(pLogger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kWarn, iErrorNo, fmt, ##__VA_ARGS__)   // 警告
#define LOG_ERROR(pLogger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kError, iErrorNo, fmt, ##__VA_ARGS__) // 错误
#define LOG_FATAL(pLogger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kFatal, iErrorNo, fmt, ##__VA_ARGS__) // 致命
#define LOG_EVENT(pLogger, iErrorNo, fmt, ...) LOG_BASE(pLogger, logger::LogLevel::kEvent, iErrorNo, fmt, ##__VA_ARGS__) // 事件

}
}
#endif // __CPPX_LOGGER_EX_H__
