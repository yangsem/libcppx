#ifndef __CPPX_TASK_SCHEDULER_H__
#define __CPPX_TASK_SCHEDULER_H__

#include <stdint.h>
#include <string.h>
#include <functional>
#include <vector>

namespace cppx
{
namespace base
{

class ITaskScheduler
{
protected:
    virtual ~ITaskScheduler() = default;

public:
    using TaskFunc = std::function<void()>;
    static const int64_t kInvalidTaskId = uint64_t(-1);

    struct Stats
    {
        int64_t iTaskId;
        const char *pTaskName;
        const char *pTaskStatus;
        const char *pTaskType;
        uint32_t uDelayMs;
        uint32_t uIntervalMs;
        uint64_t uPostTime;
        uint64_t uFixedCount;
        uint64_t uExecCount;
        uint64_t uTotalExecTimeMs;
        uint64_t uAvgExecTimeMs;
        uint64_t uMaxExecTimeMs;
        uint64_t uMinExecTimeMs;

        void Reset()
        {
            memset(this, 0, sizeof(Stats));
        }
    };
    
    /**
     * @brief Create a ITaskScheduler instance
     * @param pSchedulerName Name of the task scheduler (must be a string literal)
     * @param uThreadNum Number of threads in the thread pool
     * @return Pointer to the created ITaskScheduler instance or nullptr on failure
     *
    */
    static ITaskScheduler *Create(const char *pSchedulerName, uint32_t uThreadNum = 1);

    /**
     * @brief Destroy a ITaskScheduler instance
     * @param pScheduler Pointer to the ITaskScheduler instance to be destroyed
     * 
    */
    static void Destroy(ITaskScheduler *pScheduler);

    /**
     * @brief Start the task scheduler thread pool
     * @return 0 on success, negative value on failure
     * 
     * @note thread-safe
    */
    virtual int32_t Start() = 0;

    /**
     * @brief Stop the task scheduler thread pool
     * 
     * @note thread-safe
    */
    virtual void Stop() = 0;

    /**
     * @brief Post a task to be executed after a delay
     * @param pTaskName Optional name for the task (must be a string literal)
     * @param task The task to be executed
     * @param uDelayMs Delay in milliseconds before executing the task
     * 
     * @return Task ID for the posted task, negative value on failure
     * 
     * @note thread-safe
    */
    virtual int64_t PostTask(const char *pTaskName, TaskFunc func, uint32_t uDelayMs = 0) = 0;

    /**
     * @brief Post a fixed count periodic task to be executed at regular intervals
     * @param pTaskName Optional name for the task (must be a string literal)
     * @param func The task to be executed
     * @param uIntervalMs Interval in milliseconds between task executions
     * @param uCount Number of times the task should be executed
     * 
     * @return Task ID for the posted fixed count periodic task, negative value on failure
     * 
     * @note thread-safe
    */
    virtual int64_t PostFixedCountTask(const char *pTaskName, TaskFunc func, uint32_t uIntervalMs, 
                                       uint32_t uCount, bool bRunImmediately = false) = 0;

    /**
     * @brief Post a periodic task to be executed at regular intervals
     * @param pTaskName Optional name for the task (must be a string literal)
     * @param func The task to be executed
     * @param uIntervalMs Interval in milliseconds between task executions
     * 
     * @return Task ID for the posted periodic task, negative value on failure
     * 
     * @note thread-safe
    */
    virtual int64_t PostPeriodicTask(const char *pTaskName, TaskFunc func, uint32_t uIntervalMs, 
                                     bool bRunImmediately = false) = 0;

    /**
     * @brief Cancel a previously posted periodic task
     * @param iTaskId Task ID of the periodic task to be cancelled
     * @return 0 on success, negative value on failure
     * 
     * @note thread-safe
    */
    virtual int32_t CancleTask(int64_t iTaskId) = 0;

    /**
     * @brief Get statistics of all tasks
     * @param vecStats Vector to store the statistics of all tasks
     * @return 0 on success, negative value on failure
     * 
     * @note thread-safe
    */
    virtual int32_t GetStats(std::vector<Stats> &vecStats) = 0;
};

}
}

#endif // __CPPX_TASK_SCHEDULER_H__
