#ifndef __CPPX_TASK_SCHEDULER_IMPL_H__
#define __CPPX_TASK_SCHEDULER_IMPL_H__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <string>
#include <vector>

#include <thread/task_scheduler.h>
#include <thread/thread_manager.h>

namespace cppx
{
namespace base
{

class CTaskSchedulerImpl final : public ITaskScheduler
{
public:
    CTaskSchedulerImpl() = default;
    ~CTaskSchedulerImpl() noexcept override;

    CTaskSchedulerImpl(const CTaskSchedulerImpl &) = delete;
    CTaskSchedulerImpl &operator=(const CTaskSchedulerImpl &) = delete;
    CTaskSchedulerImpl(CTaskSchedulerImpl &&) = delete;
    CTaskSchedulerImpl &operator=(CTaskSchedulerImpl &&) = delete;

    int32_t Init(const char *pSchedulerName, uint32_t uPrecisionUs) noexcept;

    int32_t Start() noexcept override;
    void Stop() noexcept override;

    int64_t PostTask(Task *pTask) noexcept override;
    int64_t PostOnceTask(const char* pTaskName, TaskFunc pFunc, 
                        void* pCtx, uint32_t uDelayUs) noexcept override;
    int64_t PostPeriodicTask(const char* pTaskName, TaskFunc pFunc, void* pCtx, 
                            uint32_t uDelayUs, uint32_t uInternalUs) noexcept override;
    int32_t CancleTask(int64_t iTaskID) noexcept override;

    int32_t GetStats(IJson *pJson) const noexcept override;

private:
    static bool RunWrapper(void *ptr);
    void Run();

private:
    struct TaskEx
    {
        Task task;
        int64_t uTaskExecCount;
        int64_t iTaskID;
    };

private:
    IThreadManager *m_pThreadManager {nullptr};
    IThread *m_pThread {nullptr};

    std::string m_strSchedulerName;

    std::mutex m_lock;
    std::condition_variable m_cond;
    uint32_t m_uCondWaitUs {10};

    std::map<uint64_t, TaskEx> m_mapTasks;

    int64_t m_iNextTaskID {0};
};

}
}

#endif // __CPPX_TASK_SCHEDULER_IMPL_H__