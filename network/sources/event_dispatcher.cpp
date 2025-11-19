#include "event_dispatcher.h"
#include <logger/logger_ex.h>
#include <sys/epoll.h>
#include "acceptor_impl.h"
#include "connection_impl.h"

namespace cppx
{
namespace network
{

CEventDispatcher::CEventDispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx)
    : IDispatcher(pLogger, pAllocatorEx)
{
}

CEventDispatcher::~CEventDispatcher()
{
}

int32_t CEventDispatcher::Init(const char *pEngineName)
{
    if (pEngineName == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        m_strEngineName = pEngineName;
        m_vecEpollEvents.resize(1024);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "Failed to resize epoll events");
        return ErrorCode::kThrowException;
    }

    if (IDispatcher::Init() != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} failed to init event dispatcher", m_strEngineName.c_str());
        return ErrorCode::kInvalidCall;
    }

    char szThreadName[16];
    snprintf(szThreadName, sizeof(szThreadName), "env_disp_%s", m_strEngineName.c_str());
    if (m_pThread->Bind(szThreadName, &CEventDispatcher::RunWrapper, this) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} failed to bind event dispatcher", m_strEngineName.c_str());
        return ErrorCode::kInvalidCall;
    }

    if (m_EpollImpl.Init() != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} failed to init epoll {}", m_strEngineName.c_str(), strerror(errno));
        return ErrorCode::kInvalidCall;
    }

    return ErrorCode::kSuccess;
}

void CEventDispatcher::Exit()
{
    m_EpollImpl.Exit();
    m_vecEpollEvents.clear();
    m_setAcceptor.clear();
    m_setConnection.clear();
    IDispatcher::Exit();
}

bool CEventDispatcher::RunWrapper(void *pArg)
{
    auto pEventDispatcher = static_cast<CEventDispatcher *>(pArg);
    pEventDispatcher->Run();
    return true;
}

void CEventDispatcher::Run()
{
    int32_t iRet = m_EpollImpl.Wait(m_vecEpollEvents.data(), m_vecEpollEvents.size(), 1);
    if (iRet != -1)
    {
        for (int32_t i = 0; i < iRet; i++)
        {
            ProcessEvent(m_vecEpollEvents[i]);
        }
    }
    else
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "Failed to wait epoll %s", strerror(errno));
    }

    Task task;
    constexpr uint32_t kBatchTaskSize = 16;
    for (uint32_t i = 0; i < kBatchTaskSize; i++)
    {
        if (m_TaskQueue.GetTask(task) != ErrorCode::kSuccess)
        {
            break;
        }
        ProcessTask(task);
    }
}

void CEventDispatcher::ProcessEvent(const struct epoll_event &epollEvent)
{
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kAddAcceptor>(const Task &task)
{
    auto pAcceptor = static_cast<CAcceptorImpl *>(task.pCtx);
    try
    {
        m_setAcceptor.insert(pAcceptor);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to add acceptor {} to set", 
            m_strEngineName.c_str(), pAcceptor->GetName());
        return false;
    }

    if (m_EpollImpl.Add(pAcceptor->GetFd(), pAcceptor, EPOLLIN) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to add acceptor {} to epoll {}", 
            m_strEngineName.c_str(), pAcceptor->GetName(), strerror(errno));
        return false;
    }

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} added acceptor {} to epoll", 
        m_strEngineName.c_str(), pAcceptor->GetName());
    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kRemoveAcceptor>(const Task &task)
{
    auto pAcceptor = static_cast<CAcceptorImpl *>(task.pCtx);
    if (m_EpollImpl.Del(pAcceptor->GetFd(), pAcceptor) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to remove acceptor {} from epoll {}", 
            m_strEngineName.c_str(), pAcceptor->GetName(), strerror(errno));
        return false;
    }
    m_setAcceptor.erase(pAcceptor);
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} removed acceptor {} from epoll", 
        m_strEngineName.c_str(), pAcceptor->GetName());
    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kAsyncConnect>(const Task &task)
{
    auto pConnection = static_cast<CConnectionImpl *>(task.pCtx);
    if (m_EpollImpl.Add(pConnection->GetFd(), pConnection, EPOLLIN) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to add connection {} to epoll {}", 
            m_strEngineName.c_str(), pConnection->GetName(), strerror(errno));
        return false;
    }
    m_setConnection.insert(pConnection);
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} added connection {} to epoll", 
        m_strEngineName.c_str(), pConnection->GetName());
    return true;
}

void CEventDispatcher::ProcessTask(const Task &task)
{
    bool bResult = false;
    switch (task.eTaskType)
    {
        case TaskType::kAddAcceptor:
            bResult = ProcessTask<TaskType::kAddAcceptor>(task);
            break;
        case TaskType::kRemoveAcceptor:
            bResult = ProcessTask<TaskType::kRemoveAcceptor>(task);
            break;
        case TaskType::kAsyncConnect:
            bResult = ProcessTask<TaskType::kAsyncConnect>(task);
            break;
        case TaskType::kAsyncDisconnect:
            bResult = ProcessTask<TaskType::kAsyncDisconnect>(task);
            break;
        default:
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "Invalid task type: {}", Wrap(uint32_t(task.eTaskType)));
            break;
    }

    if (task.funcCallback != nullptr)
    {
        task.funcCallback(bResult);
    }
}

}
}