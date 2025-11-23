#include "event_dispatcher.h"
#include <cstdint>
#include <logger/logger_ex.h>
#include <sys/epoll.h>
#include "acceptor_impl.h"
#include "connection_impl.h"
#include "dispatcher.h"

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
    m_setAcceptor.clear();
    m_setConnection.clear();
    m_pEngine = nullptr;
}

int32_t CEventDispatcher::Init(IEngine *pEngine)
{
    if (pEngine == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    m_pEngine = pEngine;
    auto iErrorNo = IDispatcher::Init(16);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} failed to init event dispatcher {}", 
            m_pEngine->GetName(), Wrap(iErrorNo));
        return iErrorNo;
    }

    char szThreadName[16];
    snprintf(szThreadName, sizeof(szThreadName), "env_disp_%s", m_pEngine->GetName());
    if (m_pThread->Bind(szThreadName, &CEventDispatcher::RunWrapper, this) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to bind event dispatcher {}", 
            m_pEngine->GetName(), strerror(errno));
        return ErrorCode::kSysCallFailed;
    }

    return ErrorCode::kSuccess;
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

void CEventDispatcher::ProcessAcceptorEvent(const struct epoll_event &epollEvent)
{
    auto pAcceptor = static_cast<CAcceptorImpl *>(epollEvent.data.ptr);
    if (epollEvent.events & EPOLLIN)
    {
        CConnectionImpl *pConnection = nullptr;
        while ((pConnection = pAcceptor->Accept()) != nullptr)
        {
            Task taskAddConnection;
            taskAddConnection.eTaskType = TaskType::kAddConnection;
            taskAddConnection.pCtx = pConnection;
            taskAddConnection.funcCallback = nullptr;
            if (DoTask(taskAddConnection) != ErrorCode::kSuccess)
            {
                LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to do task", pConnection->GetName());
                m_pAllocatorEx->Delete(pConnection);
                pConnection = nullptr;
                continue;
            }
        }
    }
    else
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} invalid event: {}", pAcceptor->GetName(), strerror(errno));
    }
}

void CEventDispatcher::ProcessConnectionEvent(const struct epoll_event &epollEvent)
{
    auto pConnection = static_cast<CConnectionImpl *>(epollEvent.data.ptr);
    if (!(epollEvent.events & EPOLLIN || epollEvent.events & EPOLLERR))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} invalid event: {}", pConnection->GetName(), strerror(errno));
        return;
    }

    // 查询连接状态
    int32_t iStatus = 0;
    socklen_t iStatusLen = sizeof(iStatus);
    if (getsockopt(epollEvent.data.fd, SOL_SOCKET, SO_ERROR, &iStatus, &iStatusLen) == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to get socket status: {}", 
            pConnection->GetName(), strerror(errno));
        pConnection->Close();
        return;
    }
    if (iStatus != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} connect to {}:{} failed: {}", 
            pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()), strerror(iStatus));
        pConnection->Close();
        return;
    }

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} connected to {}:{} successfully", 
        pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()));

    if (pConnection->OnConnected() != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} connected to {}:{} failed", 
            pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()));
        pConnection->Close();
    }
}

void CEventDispatcher::ProcessEvent(const struct epoll_event &epollEvent)
{
    if (m_setAcceptor.count(epollEvent.data.ptr) != 0)
    {
        ProcessAcceptorEvent(epollEvent);
    }
    else if (m_setConnection.count(epollEvent.data.ptr) != 0)
    {
        ProcessConnectionEvent(epollEvent);
        m_EpollImpl.Del(epollEvent.data.fd, epollEvent.data.ptr);
        m_setConnection.erase(epollEvent.data.ptr);
    }
    else
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} not found", Wrap(uintptr_t(epollEvent.data.ptr)));
        m_EpollImpl.Del(epollEvent.data.fd, epollEvent.data.ptr);
        return;
    }
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kAddAcceptor>(const Task &task)
{
    auto pAcceptor = static_cast<CAcceptorImpl *>(task.pCtx);
    if (m_setAcceptor.count(pAcceptor) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} acceptor already exists", pAcceptor->GetName());
        return false;
    }

    try
    {
        m_setAcceptor.insert(pAcceptor);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to add acceptor {} to set", 
            m_pEngine->GetName(), pAcceptor->GetName());
        return false;
    }

    if (m_EpollImpl.Add(pAcceptor->GetFd(), pAcceptor, EPOLLIN) != 0)
    {
        m_setAcceptor.erase(pAcceptor);
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to add acceptor {} to epoll {}", 
            m_pEngine->GetName(), pAcceptor->GetName(), strerror(errno));
        return false;
    }

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} added acceptor {} to epoll", 
        m_pEngine->GetName(), pAcceptor->GetName());
    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kRemoveAcceptor>(const Task &task)
{
    auto pAcceptor = static_cast<CAcceptorImpl *>(task.pCtx);
    if (m_setAcceptor.count(pAcceptor) == 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} acceptor not found", pAcceptor->GetName());
        return false;
    }

    if (m_EpollImpl.Del(pAcceptor->GetFd(), pAcceptor) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to remove acceptor {} from epoll {}", 
            m_pEngine->GetName(), pAcceptor->GetName(), strerror(errno));
        return false;
    }
    m_setAcceptor.erase(pAcceptor);
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} removed acceptor {} from epoll", 
        m_pEngine->GetName(), pAcceptor->GetName());
    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kAddConnection>(const Task &task)
{
    auto pConnection = static_cast<CConnectionImpl *>(task.pCtx);
    if (m_setConnection.count(pConnection) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} connection already exists", pConnection->GetName());
        return false;
    }

    try
    {
        m_setConnection.insert(pConnection);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to add connection {} to set", 
            m_pEngine->GetName(), pConnection->GetName());
        return false;
    }

    if (m_EpollImpl.Add(pConnection->GetFd(), pConnection, EPOLLIN) != 0)
    {
        m_setConnection.erase(pConnection);
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to add connection {} to epoll {}", 
            m_pEngine->GetName(), pConnection->GetName(), strerror(errno));
        return false;
    }

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} added connection {} to epoll", 
        m_pEngine->GetName(), pConnection->GetName());
    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kRemoveConnection>(const Task &task)
{
    auto pConnection = static_cast<CConnectionImpl *>(task.pCtx);
    if (m_setConnection.count(pConnection) == 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} connection not found", pConnection->GetName());
        return false;
    }
    if (m_EpollImpl.Del(pConnection->GetFd(), pConnection) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to remove connection {} from epoll {}", 
            m_pEngine->GetName(), pConnection->GetName(), strerror(errno));
        return false;
    }
    m_setConnection.erase(pConnection);

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} removed connection {} from epoll", 
        m_pEngine->GetName(), pConnection->GetName());
    return true;
}

void CEventDispatcher::DetachCallback(IConnection *pConnection, void *pCtx)
{
    auto pEventDispatcher = static_cast<CEventDispatcher *>(pCtx);

    Task taskDetach;
    taskDetach.eTaskType = TaskType::kDisconnected;
    taskDetach.pCtx = pConnection;
    taskDetach.funcCallback = nullptr;
    if (pEventDispatcher->DoTask(taskDetach) != ErrorCode::kSuccess)
    {
        LOG_ERROR(pEventDispatcher->m_pLogger, ErrorCode::kSystemError, "{} failed to do task", pConnection->GetName());
    }
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kDoDisconnect>(const Task &task)
{
    auto pConnection = static_cast<CConnectionImpl *>(task.pCtx);

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} do disconnect from {}:{}", 
        pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()));

    if (m_pEngine->DetachConnection2(pConnection, &CEventDispatcher::DetachCallback, this) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} failed to detach connection", pConnection->GetName());
        return false;
    }

    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kConnected>(const Task &task)
{
    auto pConnection = static_cast<CConnectionImpl *>(task.pCtx);

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} connected to {}:{}", 
        pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()));

    if (pConnection->OnConnected() != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} connected to {}:{} failed", 
            pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()));

        pConnection->Close();
        return false;
    }

    if (m_pEngine->AttachConnection(pConnection) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} failed to attach connection", pConnection->GetName());
        pConnection->Close();
        return false;
    }
    return true;
}

template<>
bool CEventDispatcher::ProcessTask<TaskType::kDisconnected>(const Task &task)
{
    auto pConnection = static_cast<CConnectionImpl *>(task.pCtx);

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} disconnected from {}:{}", 
        pConnection->GetName(), pConnection->GetRemoteIP(), Wrap(pConnection->GetRemotePort()));

    pConnection->OnDisconnected();
    return true;
}

bool CEventDispatcher::ProcessTask(const Task &task)
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
        case TaskType::kAddConnection:
            bResult = ProcessTask<TaskType::kAddConnection>(task);
            break;
        case TaskType::kRemoveConnection:
            bResult = ProcessTask<TaskType::kRemoveConnection>(task);
            break;
        case TaskType::kDoDisconnect:
            bResult = ProcessTask<TaskType::kDoDisconnect>(task);
            break;
        case TaskType::kConnected:
            bResult = ProcessTask<TaskType::kConnected>(task);
            break;
        case TaskType::kDisconnected:
            bResult = ProcessTask<TaskType::kDisconnected>(task);
            break;
        default:
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "Invalid task type: {}", Wrap(uint32_t(task.eTaskType)));
            break;
    }

    if (task.funcCallback != nullptr)
    {
        task.funcCallback(bResult);
    }

    return bResult;
}

int32_t CEventDispatcher::DoTask(Task &task)
{
    if (ProcessTask(task))
    {
        return ErrorCode::kSuccess;
    }
    return ErrorCode::kInvalidCall;
}

}
}