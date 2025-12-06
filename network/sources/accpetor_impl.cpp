#include "acceptor_impl.h"
#include <memory/allocator_ex.h>
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <logger/logger_ex.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <memory>

namespace cppx
{
namespace network
{

CAcceptorImpl::CAcceptorImpl(uint64_t uID, ICallback *pCallback, IDispatcher *pDispatcher,
                             NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx)
    : m_uID(uID), m_pCallback(pCallback), m_pDispatcher(pDispatcher)
    , m_pAllocatorEx(pAllocatorEx), m_pLogger(pLogger)
{
}

CAcceptorImpl::~CAcceptorImpl()
{
    if (m_iFd != -1)
    {
        close(m_iFd);
        m_iFd = -1;
    }
    m_pCallback = nullptr;
    m_pDispatcher = nullptr;
    m_pAllocatorEx = nullptr;
    m_strAcceptorName.clear();
    m_strAcceptorIP.clear();
    m_uAcceptorPort = 0;
    m_pLogger = nullptr;
    m_uID = 0;
}

int32_t CAcceptorImpl::Init(NetworkConfig *pConfig)
{
    if (pConfig == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        m_strAcceptorName = pConfig->GetString(config::kAcceptorName, default_value::kAcceptorName);
        m_strAcceptorIP = pConfig->GetString(config::kAcceptorIP, default_value::kAcceptorIP);
        m_uAcceptorPort = pConfig->GetUint32(config::kAcceptorPort, default_value::kAcceptorPort);
        m_uSocketSendBufferSize = pConfig->GetUint32(config::kSocketSendBufferBytes, default_value::kSocketSendBufferBytes);
        m_uSocketRecvBufferSize = pConfig->GetUint32(config::kSocketRecvBufferBytes, default_value::kSocketRecvBufferBytes);
        m_uHeartbeatIntervalMs = pConfig->GetUint32(config::kHeartbeatIntervalMs, default_value::kHeartbeatIntervalMs);
        m_uHeartbeatTimeoutMs = pConfig->GetUint32(config::kHeartbeatTimeoutMs, default_value::kHeartbeatTimeoutMs);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to init acceptor", m_strAcceptorName.c_str());
        return ErrorCode::kThrowException;
    }    

    m_iFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (m_iFd == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to create socket: {}", 
            m_strAcceptorName.c_str(), strerror(errno));
        return ErrorCode::kSystemError;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_uAcceptorPort);
    addr.sin_addr.s_addr = inet_addr(m_strAcceptorIP.c_str());
    if (bind(m_iFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to bind {}:{}: {}", 
            m_strAcceptorName.c_str(), m_strAcceptorIP.c_str(), Wrap(m_uAcceptorPort), strerror(errno));
        return ErrorCode::kSystemError;
    }

    return ErrorCode::kSuccess;
}

int32_t CAcceptorImpl::Start()
{
    if (m_iFd == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} is not initialized", m_strAcceptorName.c_str());
        return ErrorCode::kInvalidState;
    }

    Task task;
    task.eTaskType = TaskType::kAddAcceptor;
    task.funcCallback = nullptr;
    task.pCtx = this;
    if (m_pDispatcher->DoTask(task) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strAcceptorName.c_str());
        return ErrorCode::kSystemError;
    }

    return ErrorCode::kSuccess;
}

void CAcceptorImpl::Stop()
{
    if (m_iFd == -1)
    {
        LOG_INFO(m_pLogger, ErrorCode::kInvalidState, "{} is not initialized", m_strAcceptorName.c_str());
        return;
    }

    Task task;
    task.eTaskType = TaskType::kRemoveAcceptor;
    task.funcCallback = nullptr;
    task.pCtx = this;
    if (m_pDispatcher->DoTask(task) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strAcceptorName.c_str());
    }
}

CConnectionImpl *CAcceptorImpl::Accept()
{
    if (m_iFd == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} is not initialized", m_strAcceptorName.c_str());
        return nullptr;
    }

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int32_t iFd = accept4(m_iFd, (struct sockaddr *)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (iFd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            LOG_DEBUG(m_pLogger, ErrorCode::kDebug, "{} accept is busy", m_strAcceptorName.c_str());
            return nullptr;
        }

        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to accept: {}", m_strAcceptorName.c_str(), strerror(errno));
        return nullptr;
    }

    auto pRemoteIp = inet_ntoa(addr.sin_addr);
    auto uRemotePort = ntohs(addr.sin_port);
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} accept connection from {}:{}", m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort));

    std::unique_ptr<CConnectionImpl> upConnection(m_pAllocatorEx->New<CConnectionImpl>());
    if (upConnection == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create connection from {}:{}", m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort));
        return nullptr;
    }
    try
    {
        upConnection->m_iFd = iFd;
        upConnection->m_pCallback = m_pCallback;
        upConnection->m_pMessagePool = nullptr;
        clock_get_time_nano(upConnection->m_ulastRecvTimeNs);

        upConnection->m_pAllocatorEx = m_pAllocatorEx;
        upConnection->m_pDispatcher = m_pDispatcher;
        upConnection->m_pLogger = m_pLogger;

        upConnection->m_strRemoteIP = pRemoteIp;
        upConnection->m_uRemotePort = uRemotePort;
        upConnection->m_strLocalIP = m_strAcceptorIP;
        upConnection->m_uLocalPort = m_uAcceptorPort;

        upConnection->m_uSocketSendBufferSize = m_uSocketSendBufferSize;
        upConnection->m_uSocketRecvBufferSize = m_uSocketRecvBufferSize;
        upConnection->m_uHeartbeatIntervalMs = m_uHeartbeatIntervalMs;
        upConnection->m_uHeartbeatTimeoutMs = m_uHeartbeatTimeoutMs;

        if (upConnection->InitBuffer() != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to init buffer", m_strAcceptorName.c_str());
            return nullptr;
        }
    }
    catch(std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to init connection from {}:{}", 
            m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort));
        return nullptr;
    }

    if (m_uSocketSendBufferSize != 0)
    {
        if (setsockopt(iFd, SOL_SOCKET, SO_SNDBUF, &m_uSocketSendBufferSize, sizeof(m_uSocketSendBufferSize)) == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to set {}:{} socket buffer {}", 
                m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort), strerror(errno));
            return nullptr;
        }
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} set {}:{} socket buffer size to {}", 
            m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort), Wrap(m_uSocketSendBufferSize));

        if (setsockopt(iFd, SOL_SOCKET, SO_RCVBUF, &m_uSocketRecvBufferSize, sizeof(m_uSocketRecvBufferSize)) == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to set {}:{} socket buffer {}", 
                m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort), strerror(errno));
            return nullptr;
        }
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} set {}:{} socket buffer size to {}", 
            m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort), Wrap(m_uSocketRecvBufferSize));
    }

    if (m_pCallback->OnAccept(upConnection.get()) != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to accept connection from {}:{}", 
            m_strAcceptorName.c_str(), pRemoteIp, Wrap(uRemotePort));
        return nullptr;
    }

    return upConnection.release();
}

}
}