#include <cppx_common.h>
#include <cppx_last_error.h>
#include <cppx_error_code.h>
#include <cppx_get_time.h>
#include "task_scheduler_impl.h"

namespace cppx
{
namespace base
{

static const char *szTaskTypeArr[] = {"RunOnce", "RunFixedCount", "RunPeriodic"};
static const char *szTaskStatusArr[] = {"Waiting", "Running"};

ITaskScheduler *ITaskScheduler::Create(const char *pSchedulerName, uint32_t uThreadNum)
{
    CTaskSchedulerImpl *pScheduler = NEW CTaskSchedulerImpl();
    if (pScheduler == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kNoMemory, "No memory to create TaskScheduler");
        return nullptr;
    }

    auto iErrorNo = pScheduler->Init(pSchedulerName, uThreadNum);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        delete pScheduler;
        return nullptr;
    }

    return pScheduler;
}

void ITaskScheduler::Destroy(ITaskScheduler *pScheduler)
{
    if (pScheduler != nullptr)
    {
        delete pScheduler;
    }
}

CTaskSchedulerImpl::~CTaskSchedulerImpl()
{
    Stop();
}

int32_t CTaskSchedulerImpl::Init(const char *pSchedulerName, uint32_t uThreadNum)
{
    if (pSchedulerName == nullptr || uThreadNum == 0)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, 
                        "Invalid parameter, SchedulerName: %p, ThreadNum: %u", 
                        pSchedulerName, uThreadNum);
        return ErrorCode::kInvalidParam;
    }

    try
    {
        m_uThreadNum = uThreadNum;
        m_strSchedulerName = pSchedulerName;
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "%s throw exception %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CTaskSchedulerImpl::Start()
{
    try
    {
        std::lock_guard<std::mutex> guard(m_lockTasks);
        if (m_bRunning)
        {
            SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s is running", m_strSchedulerName.c_str());
            return ErrorCode::kInvalidCall;
        }

        m_bRunning = true;
        for (uint32_t i = 0; i < m_uThreadNum; i++)
        {
            m_vecThreads.push_back(std::thread(&CTaskSchedulerImpl::Run, this));
        }
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

void CTaskSchedulerImpl::Stop()
{
    std::lock_guard<std::mutex> guard(m_lockTasks);
    if (m_bRunning == false)
    {
        return;
    }

    m_bRunning = false;
    for (auto &thread : m_vecThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

int64_t CTaskSchedulerImpl::PostTask(const char *pTaskName, TaskFunc func, uint32_t uDelayMs)
{
    if (unlikely(pTaskName == nullptr))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, 
                    "Scheduler %s invalid taskname", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    std::lock_guard<std::mutex> guard(m_lockTasks);
    if (unlikely(m_bRunning == false))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s is not running", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    auto uCurrentNano = GetTimeTool::GetCurrentNano();
    while (m_mapTasks.count(uCurrentNano) != 0)
    {
        uCurrentNano++;
    }

    try
    {
        Task task;
        task.eType = TaskType::kOnce;
        task.uFixedCount = 0;
        task.uDelayMs = uDelayMs;
        task.uIntervalMs = 0;
        task.funcTask = std::move(func);
        task.uFixedCount = 0;
        task.uExecedCount = 0;
        task.uTaskId = m_iNextTaskId;
        m_mapTasks.emplace(std::make_pair(uCurrentNano, std::move(task)));

        {
            std::lock_guard<std::mutex> guard(m_lockStats);
            auto &status = m_mapStats[m_iNextTaskId];
            status.Reset();
            status.iTaskId = m_iNextTaskId;
            status.pTaskName = pTaskName;
            status.pTaskStatus = szTaskStatusArr[0];
            status.pTaskType = szTaskTypeArr[0];
            status.uDelayMs = uDelayMs;
            status.uIntervalMs = 0;
            status.uPostTime = uCurrentNano;
        }
    }
    catch(std::exception &e)
    {
        m_mapTasks.erase(uCurrentNano);
        SET_LAST_ERROR(ErrorCode::kThrowException, 
                       "Scheduler %s post task %s Failed", 
                       m_strSchedulerName.c_str(), pTaskName);
        return kInvalidTaskId;
    }

    return m_iNextTaskId++;
}

int64_t CTaskSchedulerImpl::PostFixedCountTask(const char *pTaskName, TaskFunc func, uint32_t uIntervalMs, 
                                                uint32_t uCount, bool bRunImmediately)
{
    if (unlikely(pTaskName == nullptr))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, 
                    "Scheduler %s invalid taskname", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    std::lock_guard<std::mutex> guard(m_lockTasks);
    if (unlikely(m_bRunning == false))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s is not running", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    auto uCurrentNano = GetTimeTool::GetCurrentNano();
    auto uPostNano = uCurrentNano;
    uCurrentNano += (bRunImmediately ? 0 : uIntervalMs);
    while (m_mapTasks.count(uCurrentNano) != 0)
    {
        uCurrentNano++;
    }

    try
    {
        Task task;
        task.eType = TaskType::kFixedCount;
        task.uDelayMs = 0;
        task.uIntervalMs = uIntervalMs;
        task.funcTask = std::move(func);
        task.uFixedCount = uCount;
        task.uExecedCount = 0;
        task.uTaskId = m_iNextTaskId;
        m_mapTasks.emplace(std::make_pair(uCurrentNano, std::move(task)));

        {
            std::lock_guard<std::mutex> guard(m_lockStats);
            auto &status = m_mapStats[m_iNextTaskId];
            status.Reset();
            status.iTaskId = m_iNextTaskId;
            status.pTaskName = pTaskName;
            status.pTaskStatus = szTaskStatusArr[0];
            status.pTaskType = szTaskTypeArr[0];
            status.uDelayMs = 0;
            status.uIntervalMs = uIntervalMs;
            status.uPostTime = uPostNano;
            status.uFixedCount = uCount;
        }
    }
    catch(std::exception &e)
    {
        m_mapTasks.erase(uCurrentNano);
        SET_LAST_ERROR(ErrorCode::kThrowException, 
                       "Scheduler %s post task %s Failed", 
                       m_strSchedulerName.c_str(), pTaskName);
        return kInvalidTaskId;
    }

    return m_iNextTaskId++;
}

int64_t CTaskSchedulerImpl::PostPeriodicTask(const char *pTaskName, TaskFunc func, uint32_t uIntervalMs, 
                                             bool bRunImmediately)
{
    if (unlikely(pTaskName == nullptr))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, 
                    "Scheduler %s invalid taskname", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    std::lock_guard<std::mutex> guard(m_lockTasks);
    if (unlikely(m_bRunning == false))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s is not running", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    auto uCurrentNano = GetTimeTool::GetCurrentNano();
    auto uPostNano = uCurrentNano;
    uCurrentNano += (bRunImmediately ? 0 : uIntervalMs);
    while (m_mapTasks.count(uCurrentNano) != 0)
    {
        uCurrentNano++;
    }

    try
    {
        Task task;
        task.eType = TaskType::kPeriodic;
        task.uFixedCount = 0;
        task.uDelayMs = 0;
        task.uIntervalMs = uIntervalMs;
        task.funcTask = std::move(func);
        task.uFixedCount = 0;
        task.uExecedCount = 0;
        task.uTaskId = m_iNextTaskId;
        m_mapTasks.emplace(std::make_pair(uCurrentNano, std::move(task)));

        {
            std::lock_guard<std::mutex> guard(m_lockStats);
            auto &status = m_mapStats[m_iNextTaskId];
            status.Reset();
            status.iTaskId = m_iNextTaskId;
            status.pTaskName = pTaskName;
            status.pTaskStatus = szTaskStatusArr[0];
            status.pTaskType = szTaskTypeArr[0];
            status.uDelayMs = 0;
            status.uIntervalMs = 0;
            status.uPostTime = uPostNano;
        }
    }
    catch(std::exception &e)
    {
        m_mapTasks.erase(uCurrentNano);
        SET_LAST_ERROR(ErrorCode::kThrowException, 
                       "Scheduler %s post task %s Failed", 
                       m_strSchedulerName.c_str(), pTaskName);
        return kInvalidTaskId;
    }

    return m_iNextTaskId++;
}

int32_t CTaskSchedulerImpl::CancleTask(int64_t iTaskId)
{
    std::lock_guard<std::mutex> guard(m_lockTasks);
    if (unlikely(m_bRunning == false))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Scheduler %s is not running", m_strSchedulerName.c_str());
        return kInvalidTaskId;
    }

    {
        
    }
}

}
}