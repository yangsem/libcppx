#ifndef __CPPX_LOG_H__
#define __CPPX_LOG_H__

#include <cstdint>
#include <utilities/export.h>
#include <utilities/json.h>

namespace cppx
{
namespace base
{

class EXPORT ILogger
{
public:
    enum class LogLevel : uint8_t
    {
        kTrace = 0, // 跟踪
        kDebug,     // 调试
        kInfo,      // 信息
        kWarn,      // 警告
        kError,     // 错误
        kFatal,     // 致命错误
        kEvent,     // 事件
    };

protected:
    LogLevel m_eLogLevel; // 日志级别

    virtual ~ILogger() noexcept = default;

public:
    /**
     * @brief 创建一个ILogger对象
     * @param pConfig 配置对象
     * @return 成功返回ILogger对象指针，失败返回nullptr
     * @note 多线程安全
     */
    static ILogger *Create(IJson *pConfig) noexcept;

    /**
     * @brief 销毁一个ILogger对象
     * @param pLog ILogger对象指针
     * @note 多线程安全
     */
    static void Destroy(ILogger *pLog) noexcept;

    /**
     * @brief 获取日志级别
     * @return 日志级别
     * @note 多线程安全
     */
    LogLevel GetLogLevel() const noexcept { return m_eLogLevel; }

    /**
     * @brief 设置日志级别
     * @param eLevel 日志级别
     * @note 多线程安全
     */
    void SetLogLevel(LogLevel eLevel) noexcept { m_eLogLevel = eLevel; }

    /**
     * @brief 初始化ILogger对象
     * @param pConfig 配置对象
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t Init(IJson *pConfig) noexcept = 0;

    /**
     * @brief 清理资源并退出ILogger对象
     * @note 多线程不安全
     */
    virtual void Exit() noexcept = 0;

    /**
     * @brief 启动ILogger对象的线程
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t Start() noexcept = 0;

    /**
     * @brief 停止ILogger对象的线程
     * @note 多线程不安全
     */
    virtual void Stop() noexcept = 0;

    /**
     * @brief 记录日志
     * @param iErrorNo 错误码
     * @param eLevel 日志级别
     * @param pModule 模块名称
     * @param pFileLine 文件行号
     * @param pFunction 函数名称
     * @param pFormat 格式化字符串, 格式："this is a test {} {} {}"
     * @param ppParams 参数数组, 格式：{"test", "test2", "test3"}
     * @param uParamCount 参数数量
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t Log(int32_t iErrorNo, LogLevel eLevel, const char *pModule,
                        const char *pFileLine, const char *pFunction,
                        const char *pFormat, const char **ppParams, uint32_t uParamCount) noexcept = 0;

    /**
     * @brief 记录日志
     * @param iErrorNo 错误码
     * @param eLevel 日志级别
     * @param pFormat 格式化字符串, 格式参考printf函数
     * @param ... 可变参数
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t LogFormat(int32_t iErrorNo, LogLevel eLevel, const char *pFormat, ...) noexcept = 0;

    /**
     * @brief 获取统计信息
     * @param pJsonStats 统计信息对象
     * @return 成功返回0，失败返回错误码
     * @return 统计信息
     * @note 多线程安全
     */
    virtual int32_t GetStats(IJson *pJsonStats) const noexcept = 0;
};

namespace config
{
constexpr const char *kLoggerName = "logger_name"; // 日志名称, 类型: string
constexpr const char *kLogLevel = "log_level";  // 日志级别, 类型: string
constexpr const char *kLogAsync = "log_async";  // 是否异步, 类型: bool
constexpr const char *kBindCpuNo = "bind_cpu_no";  // 绑定CPU编号, 类型: uint32_t
constexpr const char *kLogPath = "log_path";    // 日志路径, 类型: string
constexpr const char *kLogPrefix = "log_prefix";    // 日志文件前缀名, 类型: string
constexpr const char *kLogSuffix = "log_suffix";    // 日志文件后缀名, 类型: string
constexpr const char *kLogFileMaxSizeMB = "log_file_max_size_mb"; // 日志文件最大大小(MB), 类型: uint64_t
constexpr const char *kLogTotalSizeMB = "log_total_size_mb"; // 日志文件总大小(MB), 类型: uint64_t
constexpr const char *kLogFormatBufferSize = "log_format_buffer_size"; // 日志格式化缓冲区大小, 类型: uint32_t
constexpr const char *kLogChannelMaxCount = "log_channel_max_count"; // 日志通道最大元素数量, 类型: uint32_t
}

namespace default_value
{
constexpr const char *kLoggerName = ""; // 日志名称, 默认: 无
constexpr uint32_t kLogLevel = (uint32_t)ILogger::LogLevel::kInfo; // 日志级别, 默认: info
constexpr const bool kLogAsync = false; // 是否异步, 默认: false
constexpr const uint32_t kBindCpuNo = UINT32_MAX; // 绑定CPU编号, 默认: 不绑定
constexpr const char *kLogPath = "./log"; // 日志路径, 默认: ./log
constexpr const char *kLogPrefix = ""; // 日志文件前缀名, 默认: 空
constexpr const char *kLogSuffix = ".log"; // 日志文件后缀名, 默认: .log
constexpr const uint64_t kLogFileMaxSizeMB = 16; // 日志文件最大大小(MB), 默认: 16MB
constexpr const uint64_t kLogTotalSizeMB = 4 * 1024; // 日志文件总大小(MB), 默认: 4GB
constexpr const uint32_t kLogFormatBufferSize = 4096; // 日志格式化缓冲区大小, 默认: 4096
constexpr const uint32_t kLogChannelMaxCount = 8192; // 日志通道最大元素数量, 默认: 8192
}

}
}

#endif // __CPPX_LOG_H__