#ifndef __CPPX_LOG_H__
#define __CPPX_LOG_H__

#include <utilities/cppx_json.h>

namespace cppx
{
namespace base
{

class EXPORT ILog
{
public:
    enum class LogLevel : uint8_t
    {
        kDebug = 0,
        kInfo,
        kWarn,
        kError,
        kFatal,
        kEvent,
    };

protected:
    virtual ~ILog() noexcept = default;

public:
    /**
     * @brief 创建一个ILog对象
     * @param pConfig 配置对象
     * @return 成功返回ILog对象指针，失败返回nullptr
     * @note 多线程安全
     */
    static ILog *Create(IJson *pConfig) noexcept;

    /**
     * @brief 销毁一个ILog对象
     * @param pLog ILog对象指针
     * @note 多线程安全
     */
    static void Destroy(ILog *pLog) noexcept;

    /**
     * @brief 初始化ILog对象
     * @param pConfig 配置对象
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t Init(IJson *pConfig) noexcept = 0;

    /**
     * @brief 启动ILog对象的线程
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t Start() noexcept = 0;

    /**
     * @brief 停止ILog对象的线程
     * @note 多线程不安全
     */
    virtual void Stop() noexcept = 0;

    /**
     * @brief 记录日志
     * @param iErrorNo 错误码
     * @param eLevel 日志级别
     * @param pFormat 格式化字符串
     * @param ... 可变参数
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t Log(int32_t iErrorNo, LogLevel eLevel, const char *pFormat, ...) noexcept = 0;

    /**
     * @brief 获取日志级别
     * @return 日志级别
     * @note 多线程安全
     */
    virtual LogLevel GetLogLevel() const noexcept = 0;

    /**
     * @brief 设置日志级别
     * @param eLevel 日志级别
     * @note 多线程安全
     */
    virtual void SetLogLevel(LogLevel eLevel) noexcept = 0;

    /**
     * @brief 获取统计信息
     * @param pJsonStats 统计信息对象
     * @return 成功返回0，失败返回错误码
     * @return 统计信息
     * @note 多线程安全
     */
    virtual int32_t GetStats(IJson *pJsonStats) const noexcept = 0;
};

}
}

#endif // __CPPX_LOG_H__