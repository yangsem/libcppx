#include <cppx_common.h>
#include <cppx_last_error.h>
#include <cppx_error_code.h>
#include "task_scheduler_impl.h"

namespace cppx
{
namespace base
{

ITaskScheduler *ITaskScheduler::Create(const char *pSchedulerName, uint32_t uPrecisionUs) noexcept
{
    CTaskSchedulerImpl *pScheduler = NEW CTaskSchedulerImpl();
    if (pScheduler == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kNoMemory, "No memory to create TaskScheduler");
        return nullptr;
    }

    auto iErrorNo = pScheduler->Init(pSchedulerName, uPrecisionUs);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        delete pScheduler;
        return nullptr;
    }

    return pScheduler;
}

void ITaskScheduler::Destroy(ITaskScheduler *pScheduler) noexcept
{
    if (pScheduler != nullptr)
    {
        delete pScheduler;
    }
}

CTaskSchedulerImpl::~CTaskSchedulerImpl() noexcept
{
    Stop();
}

int32_t CTaskSchedulerImpl::Init(const char *pSchedulerName, uint32_t uPrecisionUs) noexcept
{
    if (pSchedulerName == nullptr)
    {
        pSchedulerName = "";
    }

    try
    {
        m_uCondWaitUs = uPrecisionUs;
        m_strSchedulerName = pSchedulerName;
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Scheduler throw exception %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CTaskSchedulerImpl::Start() noexcept
{
    try
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (m_bRunning)
            {
                SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s is running", m_strSchedulerName.c_str());
                return ErrorCode::kInvalidCall;
            }

            m_bRunning = true;
        }
        m_thScheduler = std::thread(&CTaskSchedulerImpl::Run, this);
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "%s create thread failed %s", 
                        m_strSchedulerName.c_str(), e.what());
        Stop();
        return ErrorCode::kThrowException;
    }

    return 0;
}

void CTaskSchedulerImpl::Stop() noexcept
{
    {
        std::lock_guard<std::mutex> guard(m_lock);
        if (m_bRunning == false)
        {
            return;
        }

        m_bRunning = false;
    }

    m_cond.notify_one();
    if (m_thScheduler.joinable())
    {
        m_thScheduler.join();
    }
}
int64_t CTaskSchedulerImpl::PostTask(Task *pTask) noexcept
{
    if (unlikely(pTask == nullptr))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, 
            "Scheduler %s invalid param", m_strSchedulerName.c_str());
        return TaskID::kInvalidTaskID;
    }

    if (unlikely(pTask->pTaskName == nullptr 
        || pTask->pTaskFunc == nullptr 
        || pTask->eVersion != TaskVersion::kVersion))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, 
            "Scheduler %s TaskName %p TaskFunc %p, Version(%u/%u)",
            m_strSchedulerName.c_str(), pTask->pTaskName, 
            pTask->pTaskFunc, pTask->eVersion, TaskVersion::kVersion);
        return TaskID::kInvalidTaskID;
    }

    uint64_t uExecTimeNs;
    clock_get_time_nano(uExecTimeNs);
    uExecTimeNs += (pTask->uDelayUs * kMicro);

    std::lock_guard<std::mutex> guard(m_lock);
    while (unlikely(m_mapTasks.count(uExecTimeNs)) != 0)
    {
        ++uExecTimeNs;
    }

    try
    {
        auto &taskEx = m_mapTasks[uExecTimeNs];
        taskEx.task = *pTask;
        taskEx.uTaskExecCount = 0;
        taskEx.iTaskID = m_iNextTaskID;

        if (pTask->uDelayUs == 0)
        {
            m_cond.notify_one();
        }
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Scheduler %s Post Task %s Failed",
            m_strSchedulerName.c_str(), pTask->pTaskName);
        return TaskID::kInvalidTaskID;
    }

    return m_iNextTaskID++;
}

int64_t CTaskSchedulerImpl::PostOnceTask(const char *pTaskName, TaskFunc pFunc, void *pCtx, uint32_t uDelayUs) noexcept
{
    Task task;
    task.pTaskName = pTaskName;
    task.pTaskFunc = pFunc;
    task.pTaskCtx = pCtx;
    task.eTaskType = TaskType::kRunFixedCount;
    task.eVersion = TaskVersion::kVersion;
    task.uFlags = 0;
    task.uTaskExecTimes = 1;
    task.uDelayUs = uDelayUs;
    task.uIntervalUs = 0;
    return PostTask(&task);
}

int64_t CTaskSchedulerImpl::PostPeriodicTask(const char* pTaskName, TaskFunc pFunc, void* pCtx, 
                                             uint32_t uDelayUs, uint32_t uInternalUs) noexcept 
{
    Task task;
    task.pTaskName = pTaskName;
    task.pTaskFunc = pFunc;
    task.pTaskCtx = pCtx;
    task.eTaskType = TaskType::kRunPeriodic;
    task.eVersion = TaskVersion::kVersion;
    task.uFlags = 0;
    task.uTaskExecTimes = 0;
    task.uDelayUs = uDelayUs;
    task.uIntervalUs = uInternalUs;
    return PostTask(&task);
}

int32_t CTaskSchedulerImpl::CancleTask(int64_t iTaskID) noexcept
{
    /**
     * 暂时先这么实现，问题点：
     * 遍历时间过长导致其他任务无法及时执行
    */
    std::lock_guard<std::mutex> guard(m_lock);
    for (auto &taskEx : m_mapTasks)
    {
        if (taskEx.second.iTaskID == iTaskID)
        {
            taskEx.second.task.uFlags |= TaskFlag::kTaskCancel;
            return 0;
        }
    }

    SET_LAST_ERROR(ErrorCode::kInvalidParam, "Scheduler %s Cancel Task %ld Failed", 
        m_strSchedulerName.c_str(), iTaskID);
    return ErrorCode::kInvalidParam;
}

const char *CTaskSchedulerImpl::GetStats() noexcept
{
    // 暂不实现
    SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s Not Implement %s", 
        m_strSchedulerName.c_str(), __FUNCTION__);
    return nullptr;
}

void CTaskSchedulerImpl::Run()
{
    char szThreadName[16];
    snprintf(szThreadName, sizeof(szThreadName), "task_sch_%s", m_strSchedulerName.c_str());
    set_thread_name(szThreadName);

    constexpr uint32_t uBatchTask = 16;
    uint32_t uCurrTaskSize = 0;
    const std::pair<const uint64_t, TaskEx> *batchTaskEx[uBatchTask];

    while (m_bRunning)
    {
        {
            std::unique_lock<std::mutex> lock(m_lock);
            m_cond.wait_for(lock, std::chrono::microseconds(m_uCondWaitUs), [&] () 
                { 
                    uint64_t uExecTimeNs;
                    clock_get_time_nano(uExecTimeNs);
                    return (!m_mapTasks.empty() && m_mapTasks.begin()->first <= uExecTimeNs)
                            || !m_bRunning;
                });
        }
        
        uCurrTaskSize = 0;

        uint64_t uCurrNano = 0;
        clock_get_time_nano(uCurrNano);

        {
            std::lock_guard<std::mutex> guard(m_lock);
            for (auto iter = m_mapTasks.begin(); iter != m_mapTasks.end();)
            {
                // 如果任务被取消，则删除
                if (ACCESS_ONCE(iter->second.task.uFlags) & TaskFlag::kTaskCancel)
                {
                    iter = m_mapTasks.erase(iter);
                    continue;
                }

                // 还没到执行时间，则跳出
                if (likely(iter->first > uCurrNano))
                {
                    break;
                }

                // 任务可以执行，则添加到批量执行队列
                iter->second.task.uFlags |= TaskFlag::kTaskRunning;
                batchTaskEx[uCurrTaskSize++] = &(*iter);
                iter++;

                // 批量执行队列满了，则跳出
                if (unlikely(uCurrTaskSize >= uBatchTask))
                {
                    break;
                }
            }
        }

        for (uint32_t i = 0; i < uCurrTaskSize; i++)
        {
            auto &taskEx = const_cast<TaskEx &>(batchTaskEx[i]->second);
            taskEx.task.pTaskFunc(taskEx.task.pTaskCtx);
            taskEx.uTaskExecCount++;
        }

        for (uint32_t i = 0; i < uCurrTaskSize; i++)
        {
            auto &taskEx = const_cast<TaskEx &>(batchTaskEx[i]->second);
            std::lock_guard<std::mutex> guard(m_lock);
            taskEx.task.uFlags &= ~(TaskFlag::kTaskRunning);
            if (taskEx.task.eTaskType == TaskType::kRunPeriodic
                || taskEx.uTaskExecCount < taskEx.task.uTaskExecTimes)
            {
                uint64_t uExecTimeNs;
                clock_get_time_nano(uExecTimeNs);
                uExecTimeNs += (taskEx.task.uIntervalUs * kMicro);
                while (unlikely(m_mapTasks.count(uExecTimeNs)) != 0)
                {
                    ++uExecTimeNs;
                }

                try
                {
                    m_mapTasks[uExecTimeNs] = taskEx;
                }
                catch(std::exception &e)
                {
                    SET_LAST_ERROR(ErrorCode::kThrowException, "Scheduler %s Throw Exception %s",
                        m_strSchedulerName.c_str(), e.what());
                }
            }
            // 使用完再删除，避免taskEx引用失效
            m_mapTasks.erase(batchTaskEx[i]->first);
        }
    }
}

}
}