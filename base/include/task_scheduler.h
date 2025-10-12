#ifndef __CPPX_TASK_SCHEDULER_H__
#define __CPPX_TASK_SCHEDULER_H__

#include <stdint.h>
#include <cppx_export.h>

namespace cppx
{
namespace base
{

class EXPORT ITaskScheduler
{
protected:
    virtual ~ITaskScheduler() noexcept = default;

public:
    using TaskFunc = void (*) (void *);
    
    enum TaskID : int64_t
    {
        kInvalidTaskID = int64_t(-1)
    };

    enum class TaskVersion : uint8_t
    {
        kVersion = 0x01
    };

    enum TaskFlag : uint16_t
    {
        kTaskCancel = 1 << 0,
        kTaskRunning = 1 << 1
    };

    enum class TaskType : uint8_t
    {
        kRunFixedCount = 0,
        kRunPeriodic
    };

    struct Task
    {
        const char *pTaskName;
        TaskFunc pTaskFunc;
        void *pTaskCtx;
        TaskType eTaskType;
        TaskVersion eVersion;
        uint16_t uFlags;
        uint32_t uTaskExecTimes;
        uint32_t uDelayUs;
        uint32_t uIntervalUs;
    };
    
    /**
     * @brief 创建一个任务调度器实例
     * @param[in] pSchedulerName 任务调度器的名字
     * @param[in] uPrecisionUs 调度线程空闲休眠间隔时间
     * @return 成功返回调度器实例指针，失败返回 nullptr
     * @note 多线程安全
    */
    static ITaskScheduler *Create(const char *pSchedulerName, uint32_t uPrecisionUs = 10) noexcept;

    /**
     * @brief 释放任务调度器实例
     * @param[in] 调用Create接口创建的 pScheduler
     * @note 接口多线程安全，但不能多线程Destory同一个pScheduler
    */
    static void Destroy(ITaskScheduler *pScheduler) noexcept;

    /**
     * @brief 启动调度器线程
     * @return 成功返回 0 ，否则失败
    */
    virtual int32_t Start() noexcept = 0;

    /**
     * @brief 同步停止调度器线程
     * @note 多线程安全
    */
    virtual void Stop() = 0;

    /**
     * @brief 投递一个任务到调度器执行
     * @param[in] pTask 任务
     * @return 成功返回 TaskID，否则返回 kInvalidTaskID
     * @note 多线程安全
    */
    virtual int64_t PostTask(Task *pTask) noexcept = 0;
    virtual int64_t PostOnceTask(const char* pTaskName, TaskFunc pFunc, void* pCtx, 
                                uint32_t uDelayUs) noexcept = 0;
    virtual int64_t PostPeriodicTask(const char* pTaskName, TaskFunc pFunc, void* pCtx, 
                                     uint32_t uDelayUs, uint32_t uInternalUs) noexcept = 0;

    /**
     * @brief 根据TaskID取消一个任务
     * @param[in] PostTask 返回的有效 TaskID
     * @return 成功返回 0，否则失败
    */
    virtual int32_t CancleTask(int64_t iTaskID) noexcept = 0;

    /**
     * @brief 获取调度器任务队列中的任务调度信息
     * return 成功返回正在处理的任务调度信息，否则返回 nullptr
    */
    virtual const char *GetStats() noexcept = 0;
};

}
}

#endif // __CPPX_TASK_SCHEDULER_H__
