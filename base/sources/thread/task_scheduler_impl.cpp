#include <utilities/common.h>
#include <utilities/error_code.h>
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
        SetLastError(ErrorCode::kOutOfMemory);
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

    m_uCondWaitUs = uPrecisionUs;
    m_strSchedulerName = pSchedulerName;
    m_pThreadManager = IThreadManager::GetInstance();
    m_pThread = m_pThreadManager->CreateThread();
    if (m_pThread == nullptr || m_pThread->Bind(pSchedulerName, RunWrapper, this) != 0)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    return 0;
}

int32_t CTaskSchedulerImpl::Start() noexcept
{
    if (m_pThread == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    return m_pThread->Start();
}

void CTaskSchedulerImpl::Stop() noexcept
{
    if (m_pThread == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return;
    }

    m_cond.notify_one();
    m_pThread->Stop();
}
int64_t CTaskSchedulerImpl::PostTask(Task *pTask) noexcept
{
    if (unlikely(pTask == nullptr))
    {
        SetLastError(ErrorCode::kInvalidParam);
        return TaskID::kInvalidTaskID;
    }

    if (unlikely(pTask->pTaskName == nullptr 
        || pTask->pTaskFunc == nullptr 
        || pTask->eVersion != TaskVersion::kVersion))
    {
        SetLastError(ErrorCode::kInvalidParam);
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
        SetLastError(ErrorCode::kThrowException);
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

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

int32_t CTaskSchedulerImpl::GetStats(IJson *pJson) const noexcept
{
    if (pJson == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    pJson->Clear();
    return 0;
}

bool CTaskSchedulerImpl::RunWrapper(void *ptr)
{
    auto pScheduler = static_cast<CTaskSchedulerImpl *>(ptr);
    pScheduler->Run();
    return true;
}

void CTaskSchedulerImpl::Run()
{
    constexpr uint32_t uBatchTask = 16;
    const std::pair<const uint64_t, TaskEx> *batchTaskEx[uBatchTask];

    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_cond.wait_for(lock, std::chrono::microseconds(m_uCondWaitUs), [&] () 
            { 
                uint64_t uExecTimeNs;
                clock_get_time_nano(uExecTimeNs);
                return (!m_mapTasks.empty() && m_mapTasks.begin()->first <= uExecTimeNs)
                        || !(m_pThread->GetThreadState() == IThread::ThreadState::kRunning);
            });
    }

    uint32_t uCurrTaskSize = 0;
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
                SetLastError(ErrorCode::kThrowException);
            }
        }
        // 使用完再删除，避免taskEx引用失效
        m_mapTasks.erase(batchTaskEx[i]->first);
    }
}

}
}