#ifndef __CPPX_THREAD_MANAGER_H__
#define __CPPX_THREAD_MANAGER_H__

#include <utilities/export.h>
#include <thread/thread.h>
#include <utilities/json.h>

namespace cppx
{
namespace base
{

class EXPORT IThreadManager
{
public:
    enum class ThreadEventType
    {
        kThreadStart,
        kThreadStop,
        kThreadBlock,
        kThreadUnblock,
    };

    /**
     * @brief 线程事件函数
     * @param iThreadId 线程ID
     * @param pThreadName 线程名字
     * @param eEventType 线程事件类型
     * @param pUserParam 用户参数
     */
    using ThreadEventFunc = void (*) (int32_t iThreadId, const char *pThreadName, ThreadEventType eEventType, void *pUserParam);
    
    /**
     * @brief 线程本地数据遍历函数
     * @param pThreadLocal 线程本地数据指针
     * @param pUserParam 用户参数
     * @return 返回true继续遍历，返回false停止遍历
     */
    using ThreadLocalForEachFunc = bool (*) (void *pThreadLocal, void *pUserParam);

protected:
    virtual ~IThreadManager() noexcept = default;

public:
    /**
     * @brief 创建一个线程管理器实例
     * @return 成功返回线程管理器实例指针，失败返回 nullptr
     * @note 多线程安全
     */
    static IThreadManager *Create() noexcept;

    /**
     * @brief 销毁一个线程管理器实例
     * @param pThreadManager 线程管理器实例指针
     * @note 多线程安全
     */
    static void Destroy(IThreadManager *pThreadManager) noexcept;
    
    /**
     * @brief 获取线程管理器实例
     * @return 线程管理器实例指针
     * @note 多线程安全
     */
    static IThreadManager *GetInstance() noexcept;

    /**
     * @brief 注册线程事件函数
     * @param pThreadEventFunc 线程事件函数
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t RegisterThreadEventFunc(ThreadEventFunc pThreadEventFunc, void *pUserParam) noexcept = 0;

    /**
     * @brief 创建一个线程
     * @param pThreadName 线程名字
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual IThread *CreateThread() noexcept = 0;

    /**
     * @brief 销毁一个线程
     * @param pThreadName 线程名字
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual void DestroyThread(IThread *pThread) noexcept = 0;

    /**
     * @brief 创建一个线程并启动
     * @param pThreadName 线程名字
     * @param pThreadFunc 线程函数
     * @param pThreadParam 线程参数
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t CreateThread(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadParam) noexcept = 0;

    /**
     * @brief 停止并销毁一个线程
     * @param pThreadName 线程名字
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t DestroyThread(const char *pThreadName) noexcept = 0;

    /**
     * @brief 创建一个线程本地数据ID
     * @return 成功返回线程本地数据ID，失败返回-1
     * @note 多线程安全
     */
    virtual int32_t NewThreadLocalId() noexcept = 0;

    /**
     * @brief 释放一个线程本地数据ID
     * @param iThreadLocalId 线程本地数据ID
     * @note 多线程安全
     */
    virtual void FreeThreadLocalId(int32_t iThreadLocalId) noexcept = 0;

    /**
     * @brief 分配或者获取线程本地内存
     * @param uThreadLocalId 线程本地数据ID
     * @param uThreadLocalSize 线程本地数据大小
     * @return 成功返回线程本地数据指针，失败返回 nullptr
     * @note 多线程安全，首次获取会分配内存，后续获取会返回已分配的内存，如果是管理器创建的线程退出时会自动释放该内存
     */
    virtual void* GetThreadLocal(int32_t iThreadLocalId, uint64_t uThreadLocalSize) noexcept = 0;

    /**
     * @brief 遍历所有线程uThreadLocalId相同的线程本地数据，即遍历所有数据副本
     * @param uThreadLocalId 线程本地数据ID
     * @param pThreadLocalForEachFunc 线程本地数据遍历函数
     * @param pUserParam 用户参数
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t ForEachAllThreadLocal(int32_t iThreadLocalId, ThreadLocalForEachFunc pThreadLocalForEachFunc, void *pUserParam) noexcept = 0;

    /**
     * @brief 获取线程管理器统计信息
     * @param pJson 统计信息对象
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t GetStats(IJson *pJson) const noexcept = 0;
};

}
}

#endif // __CPPX_THREAD_MANAGER_H__
