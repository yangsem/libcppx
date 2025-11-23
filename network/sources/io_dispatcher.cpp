#include "io_dispatcher.h"
#include <utilities/error_code.h>
#include <logger/logger_ex.h>
#include "connection_impl.h"
#include "utilities/common.h"

namespace cppx
{
namespace network
{

CIODispatcher::CIODispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx)
    : IDispatcher(pLogger, pAllocatorEx)
{
}

CIODispatcher::~CIODispatcher()
{
    m_pEngine = nullptr;
}

int32_t CIODispatcher::Init(IEngine *pEngine)
{
    if (pEngine == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    m_pEngine = pEngine;
    auto iErrorNo = IDispatcher::Init(64);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} failed to init io dispatcher {}", 
            m_pEngine->GetName(), Wrap(iErrorNo));
        return iErrorNo;
    }

    char szThreadName[16];
    snprintf(szThreadName, sizeof(szThreadName), "io_disp_%s", m_pEngine->GetName());
    if (m_pThread->Bind(szThreadName, &CIODispatcher::RunWrapper, this) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to bind io dispatcher {}", 
            m_pEngine->GetName(), strerror(errno));
        return ErrorCode::kSysCallFailed;
    }

    return ErrorCode::kSuccess;
}

int32_t CIODispatcher::DoTask(Task &task)
{
    return IDispatcher::DoTask(task);
}

bool CIODispatcher::RunWrapper(void *pArg)
{
    auto pIODispatcher = static_cast<CIODispatcher *>(pArg);
    pIODispatcher->Run();
    return true;
}

void CIODispatcher::ProcessRecvEvent(IConnection *pConnection, uint32_t uSize)
{
    auto pConnectionImpl = static_cast<CConnectionImpl *>(pConnection);
    auto iRecvLen = pConnectionImpl->Recv(uSize);
    if (iRecvLen < 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to recv {}", 
            pConnection->GetName(), strerror(errno));
        Task task;
        task.funcCallback = nullptr;
        task.pCtx = pConnection;
        task.eTaskType = TaskType::kRemoveRecv;
        if (DoTask(task) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", pConnection->GetName());
            return;
        }
        pConnection->Close();
    }
    else if (iRecvLen == 0)
    {
        LOG_INFO(m_pLogger, ErrorCode::kSuccess, "{} recv 0 bytes", pConnection->GetName());
        Task task;
        task.funcCallback = nullptr;
        task.pCtx = pConnection;
        task.eTaskType = TaskType::kRemoveRecv;
        if (DoTask(task) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", pConnection->GetName());
            return;
        }
        pConnection->Close();
    }
    else
    {
        LOG_DEBUG(m_pLogger, ErrorCode::kSuccess, "{} recv {} bytes", 
            pConnection->GetName(), Wrap(iRecvLen));
    }
}

void CIODispatcher::ProcessSendEvent(IConnection *pConnection, uint32_t uSize)
{
    auto pConnectionImpl = static_cast<CConnectionImpl *>(pConnection);
    auto iSendLen = pConnectionImpl->Send(uSize);
    if (iSendLen < 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} failed to send {}", 
            pConnection->GetName(), strerror(errno));
        pConnection->Close();
    }
    else if (iSendLen == 0)
    {
        LOG_INFO(m_pLogger, ErrorCode::kSuccess, "{} send 0 bytes", pConnection->GetName());
        Task task;
        task.funcCallback = nullptr;
        task.pCtx = pConnection;
        task.eTaskType = TaskType::kRemoveSend;
        if (DoTask(task) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", pConnection->GetName());
            return;
        }
        pConnection->Close();
    }
    else
    {
        LOG_DEBUG(m_pLogger, ErrorCode::kSuccess, "{} send {} bytes", 
            pConnection->GetName(), Wrap(iSendLen));
    }
}

void CIODispatcher::Run()
{
    int32_t iRet = m_EpollImpl.Wait(m_vecEpollEvents.data(), m_vecEpollEvents.size(), 1);
    if (iRet != -1)
    {
        for (int32_t i = 0; i < iRet; i++)
        {
            auto &epollEvent = m_vecEpollEvents[i];
            auto pConnection = static_cast<CConnectionImpl *>(epollEvent.data.ptr);
            if (epollEvent.events & EPOLLIN)
            {
                ProcessRecvEvent(pConnection,m_uBatchSendRecvSize);
            }

            if (epollEvent.events & EPOLLOUT)
            {
                ProcessSendEvent(pConnection, m_uBatchSendRecvSize);
            }

            if (unlikely(epollEvent.events & EPOLLERR))
            {
                LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "{} invalid event: {}", 
                    pConnection->GetName(), strerror(errno));
                pConnection->Close();
            }
        }
    }
    else
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSysCallFailed, "Failed to wait epoll %s", strerror(errno));
    }

    if (likely(m_TaskQueue.IsEmpty()))
    {
        return;
    }

    Task task;
    constexpr uint32_t kBatchTaskSize = 16;
    for (uint32_t i = 0; i < kBatchTaskSize; i++)
    {
        if (m_TaskQueue.GetTask(task) == ErrorCode::kSuccess)
        {
            ProcessTask(task);
        }
        else
        {
            break;
        }
    }
}

}
}