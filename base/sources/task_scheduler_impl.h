#ifndef __CPPX_TASK_SCHEDULER_IMPL_H__
#define __CPPX_TASK_SCHEDULER_IMPL_H__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <string>

#include <task_scheduler.h>

namespace cppx
{
namespace base
{

class CTaskSchedulerImpl : public ITaskScheduler
{
public:
    CTaskSchedulerImpl() = default;
    ~CTaskSchedulerImpl() override;

    CTaskSchedulerImpl(const CTaskSchedulerImpl &) = delete;
    CTaskSchedulerImpl &operator=(const CTaskSchedulerImpl &) = delete;
    CTaskSchedulerImpl(CTaskSchedulerImpl &&) = delete;
    CTaskSchedulerImpl &operator=(CTaskSchedulerImpl &&) = delete;

    int32_t Init(const char *pSchedulerName, uint32_t uThreadNum);

    int32_t Start() override;
    void Stop() override;

    int64_t PostTask(const char *pTaskName, TaskFunc func, uint32_t uDelayMs = 0) override;
    int64_t PostFixedCountTask(const char *pTaskName, TaskFunc func, uint32_t uIntervalMs, 
                                uint32_t uCount, bool bRunImmediately = false) override;
    int64_t PostPeriodicTask(const char *pTaskName, TaskFunc func, uint32_t uIntervalMs, 
                                bool bRunImmediately = false) override;
    int32_t CancleTask(int64_t iTaskId) override;
    
    int32_t GetStats(std::vector<Stats> &vecStats) override;

private:
    void Run();

private:
    enum class TaskType : uint8_t
    {
        kOnce = 0,
        kFixedCount,
        kPeriodic
    };

    struct Task
    {
        TaskType eType;
        uint32_t uDelayMs;
        uint32_t uIntervalMs;
        TaskFunc funcTask;
        uint32_t uExecedCount;
        uint32_t uFixedCount;
        int64_t uTaskId;
    };

private:
    volatile bool m_bRunning {false};
    uint32_t m_uThreadNum {0};
    std::string m_strSchedulerName;
    std::vector<std::thread> m_vecThreads;

    std::mutex m_lockTasks;
    std::condition_variable m_condTasks;
    std::map<uint64_t, Task> m_mapTasks;

    int64_t m_iNextTaskId {0};

    std::mutex m_lockStats;
    std::map<uint64_t, Stats> m_mapStats;
};

}
}

#endif // __CPPX_TASK_SCHEDULER_IMPL_H__