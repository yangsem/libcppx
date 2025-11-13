#ifndef __CPPX_THREAD_H__
#define __CPPX_THREAD_H__

#include <cstdint>
#include <utilities/export.h>

namespace cppx
{
namespace base
{

class EXPORT IThread
{
public:
    enum class ThreadState : uint8_t
    {
        kCreated = 0, // 创建状态
        kRunning,     // 运行状态
        kPaused,      // 暂停状态
        kStopped,     // 停止状态
    };

    /**
     * @brief 线程函数
     * @param pThreadArg 线程参数
     * @return 返回true继续运行，返回false停止运行
     */
    using ThreadFunc = bool (*) (void *);

protected:
    virtual ~IThread() = default;

public:
    /**
     * @brief 创建一个线程
     * @param pThreadName 线程名字
     * @param pThreadFunc 线程函数
     * @param pThreadArg 线程参数
     * @return 成功返回线程指针，失败返回 nullptr
     * 
     * @note 多线程安全
     */
    static IThread *Create(const char *pThreadName, ThreadFunc pThreadFunc, void *pThreadArg);

    /**
     * @brief 销毁一个线程
     * @param pThread 线程指针
     * 
     * @note 多线程安全
     */
    static void Destroy(IThread *pThread);

    /**
     * @brief 绑定线程
     * @param pThreadName 线程名字
     * @param pThreadFunc 线程函数
     * @param pThreadParam 线程参数
     * @return 成功返回0，失败返回错误码
     * 
     * @note 多线程不安全
     */
    virtual int32_t Bind(const char *pThreadName, ThreadFunc pThreadFunc, void *pThreadArg) = 0;

    /**
     * @brief 绑定CPU
     * @param iCpuNo CPU编号
     * @return 成功返回0，失败返回错误码
     * 
     * @note 多线程不安全
     */
    virtual int32_t BindCpu(int32_t iCpuNo) = 0;

    /**
     * @brief 绑定节点
     * @param iNodeNo 节点编号
     * @return 成功返回0，失败返回错误码
     * 
     * @note 多线程不安全
     */
    virtual int32_t BindNode(int32_t iNodeNo) = 0;

    /**
     * @brief 启动线程
     * @return 成功返回0，失败返回错误码
     * 
     * @note 多线程不安全
     */
    virtual int32_t Start() = 0;

    /**
     * @brief 停止线程
     * 
     * @note 多线程不安全
     */
    virtual void Stop() = 0;

    /**
     * @brief 暂停线程并同步等待线程暂停完成
     * 
     * @note 多线程不安全
     */
    virtual int32_t Pause() = 0;

    /**
     * @brief 恢复线程，不等待线程恢复
     * 
     * @note 多线程不安全
     */
    virtual int32_t Resume() = 0;

    /**
     * @brief 获取线程状态
     * @return 线程状态
     * 
     * @note 多线程安全，但可能不准确
     */
    virtual ThreadState GetThreadState() const = 0;

    /**
     * @brief 获取线程ID
     * @return 线程ID，线程未运行返回-1
     * 
     * @note 多线程安全
     */
    virtual int32_t GetThreadId() const = 0;

    /**
     * @brief 获取最后一次运行线程函数的时间
     * @return 最后一次运行线程函数的纳秒时间戳，线程未运行返回0
     * 
     * @note 多线程安全，可以用于检测线程是否卡死
     */
    virtual uint64_t GetLastRunTimeNs() const = 0;
};

}
}

#endif // __CPPX_THREAD_H__
