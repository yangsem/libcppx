#include "connection_impl.h"
#include "callback.h"
#include "dispatcher.h"
#include "engine.h"
#include "message.h"
#include "thread/spin_lock.h"
#include "utilities/common.h"
#include <cstdint>
#include <utilities/error_code.h>
#include <logger/logger_ex.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

namespace cppx
{
namespace network
{


constexpr uint32_t kNormalMessageMaxSize = 1024; // 常见消息的最大长度

CConnectionImpl::CConnectionImpl(uint64_t uID, ICallback *pCallback, IDispatcher *pDispatcher, 
                                 NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx)
    : m_uID(uID), m_pCallback(pCallback), m_pDispatcher(pDispatcher)
    , m_pLogger(pLogger), m_pAllocatorEx(pAllocatorEx)
{
}

CConnectionImpl::~CConnectionImpl()
{
    if (m_iFd != -1)
    {
        close(m_iFd);
        m_iFd = -1;
    }
    m_pCallback = nullptr;
    m_pDispatcher = nullptr;
    m_pLogger = nullptr;
    m_pAllocatorEx = nullptr;
    m_strRemoteIP.clear();
    m_strLocalIP.clear();
    m_uRemotePort = 0;
    m_uLocalPort = 0;
    m_uID = 0;
    m_uIOThreadIndex = 0;
}

int32_t CConnectionImpl::InitSendChannel()
{
    m_pSpinLockSend = base::SpinLock::Create();
    if (m_pSpinLockSend == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to create spin lock", m_strConnectionName.c_str());
        return ErrorCode::kSystemError;
    }

    base::channel::ChannelConfig stConfig;
    stConfig.uElementSize = sizeof(IMessage *);
    stConfig.uMaxElementCount = 1024;
    m_pChannelSend = base::channel::SPSCFixedBoundedChannel::Create(&stConfig);
    m_pChannelPrioritySend = base::channel::SPSCFixedBoundedChannel::Create(&stConfig);
    if (m_pChannelSend == nullptr || m_pChannelPrioritySend == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to create channel", m_strConnectionName.c_str());
        return ErrorCode::kSystemError;
    }

    return ErrorCode::kSuccess;
}

int32_t CConnectionImpl::Init(NetworkConfig *pConfig)
{
    if (pConfig == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        m_strConnectionName = pConfig->GetString(config::kConnectionName, default_value::kConnectionName);
        m_bIsSyncConnect = pConfig->GetBool(config::kIsSyncConnect, default_value::kIsSyncConnect);
        m_uConnectTimeoutMs = pConfig->GetUint32(config::kConnectTimeoutMs, default_value::kConnectTimeoutMs);
        m_strRemoteIP = pConfig->GetString(config::kConnectionRemoteIP, default_value::kConnectionRemoteIP);
        m_uRemotePort = pConfig->GetUint32(config::kConnectionRemotePort, default_value::kConnectionRemotePort);
        m_uSocketSendBufferSize = pConfig->GetUint32(config::kSocketSendBufferBytes, default_value::kSocketSendBufferBytes);
        m_uSocketRecvBufferSize = pConfig->GetUint32(config::kSocketRecvBufferBytes, default_value::kSocketRecvBufferBytes);
        m_uHeartbeatIntervalMs = pConfig->GetUint32(config::kHeartbeatIntervalMs, default_value::kHeartbeatIntervalMs);
        m_uHeartbeatTimeoutMs = pConfig->GetUint32(config::kHeartbeatTimeoutMs, default_value::kHeartbeatTimeoutMs);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to init connection", m_strConnectionName.c_str());
        return ErrorCode::kThrowException;
    }



    if (InitSendChannel() != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to init channel", m_strConnectionName.c_str());
        return ErrorCode::kSystemError;
    }

    m_iFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (m_iFd == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to create socket: {}", 
            m_strConnectionName.c_str(), strerror(errno));
        return ErrorCode::kSystemError;
    }

    if (m_uSocketSendBufferSize != 0)
    {
        if (setsockopt(m_iFd, SOL_SOCKET, SO_SNDBUF, &m_uSocketSendBufferSize, sizeof(m_uSocketSendBufferSize)) == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to set socket buffer size: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} set socket buffer size to {}", 
            m_strConnectionName.c_str(), Wrap(m_uSocketSendBufferSize));

        if (setsockopt(m_iFd, SOL_SOCKET, SO_RCVBUF, &m_uSocketRecvBufferSize, sizeof(m_uSocketRecvBufferSize)) == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to set socket buffer size: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} set socket buffer size to {}", 
            m_strConnectionName.c_str(), Wrap(m_uSocketRecvBufferSize));
    }

    return ErrorCode::kSuccess;
}

int32_t CConnectionImpl::Connect(const char *pRemoteIP, uint16_t uRemotePort, uint32_t uTimeoutMs)
{
    if (pRemoteIP == nullptr || uRemotePort == 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        m_strRemoteIP = pRemoteIP;
        m_uRemotePort = uRemotePort;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to init connection {}:{}", 
            m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
        return ErrorCode::kThrowException;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_uRemotePort);
    addr.sin_addr.s_addr = inet_addr(m_strRemoteIP.c_str());
    auto iRet = connect(m_iFd, (struct sockaddr *)&addr, sizeof(addr));
    if (iRet == -1)
    {
        if (errno != EINPROGRESS)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to connect to {}:{}", 
                m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
            return ErrorCode::kSystemError;
        }
    }

    // 异步连接
    if (!m_bIsSyncConnect)
    {
        Task task;
        task.eTaskType = iRet == 0 ? TaskType::kConnected : TaskType::kAddConnection;
        task.funcCallback = nullptr;
        task.pCtx = this;
        if (m_pDispatcher->PostTask(task) != ErrorCode::kSuccess)
        {
            Close();
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strConnectionName.c_str());
            return ErrorCode::kSystemError;
        }
        return ErrorCode::kSuccess;
    }

    // 同步连接
    struct pollfd pfd;
    pfd.fd = m_iFd;
    pfd.events = POLLIN;
    iRet = poll(&pfd, 1, uTimeoutMs == 0 ? m_uConnectTimeoutMs : uTimeoutMs);
    if (iRet == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to poll: {}", 
            m_strConnectionName.c_str(), strerror(errno));
        return ErrorCode::kSystemError;
    }
    else if (iRet == 0)
    {
        Close();
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} connect to {}:{} timeout", 
            m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
        return ErrorCode::kSystemError;
    }
    else
    {
        // 查询连接状态
        int32_t iStatus = 0;
        socklen_t iStatusLen = sizeof(iStatus);
        if (getsockopt(m_iFd, SOL_SOCKET, SO_ERROR, &iStatus, &iStatusLen) == -1)
        {
            Close();
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to get socket status: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        if (iStatus != 0)
        {
            Close();
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} connect to {}:{} failed: {}", 
                m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort), strerror(iStatus));
            return ErrorCode::kSystemError;
        }

        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} connected to {}:{} successfully", 
            m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
    }

    // 同步连接，不需要回调
    return ErrorCode::kSuccess;
}

void CConnectionImpl::Close()
{
    if (m_iFd == -1)
    {
        LOG_INFO(m_pLogger, ErrorCode::kInvalidState, "{} is not connected", m_strConnectionName.c_str());
        return;
    }

    Task task;
    task.funcCallback = nullptr;
    task.pCtx = this;
    task.eTaskType = TaskType::kDoDisconnect;
    if (!m_bIsSyncConnect)
    {
        // 异步断开， 触发回调
        if (m_pDispatcher->PostTask(task) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strConnectionName.c_str());
            return;
        }
    }
    else
    {
        // 同步断开，不触发回调
        if (m_pDispatcher->DoTask(task) != ErrorCode::kInvalidCall)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to disconnect", m_strConnectionName.c_str());
            return;
        }
    }
}

int32_t CConnectionImpl::Send(IMessage *pMessage, bool bPriority)
{
    if (unlikely(pMessage == nullptr || m_uIOThreadIndex == std::numeric_limits<uint32_t>::max()))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} invalid call", 
            m_strConnectionName.c_str());
        return ErrorCode::kInvalidCall;
    }

    base::SpinLockGuard guard(m_pSpinLockSend);
    auto channel = bPriority ? m_pChannelPrioritySend : m_pChannelSend;
    auto pData = channel->New();
    if (likely(pData != nullptr))
    {
        *reinterpret_cast<IMessage **>(pData) = pMessage;
        channel->Post(pData);
        return ErrorCode::kSuccess;
    }

    return ErrorCode::kOutOfMemory;
}

int32_t CConnectionImpl::Send(const uint8_t *pData, uint32_t uLength, bool bPriority)
{
    if (unlikely(pData == nullptr || uLength == 0 || m_uIOThreadIndex == std::numeric_limits<uint32_t>::max()))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} invalid call", 
            m_strConnectionName.c_str());
        return ErrorCode::kInvalidCall;
    }

    auto pMessage = NewMessage(uLength);
    if (unlikely(pMessage == nullptr))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create message", m_strConnectionName.c_str());
        return ErrorCode::kOutOfMemory;
    }

    pMessage->Append(pData, uLength);
    if (likely(Send(pMessage, bPriority) == ErrorCode::kSuccess))
    {
        return ErrorCode::kSuccess;
    }
    else
    {
        DeleteMessage(pMessage);
        return ErrorCode::kOutOfMemory;
    }
}

int32_t CConnectionImpl::Recv(IMessage **ppMessage, uint32_t uTimeoutMs)
{
    UNSED(ppMessage);
    UNSED(uTimeoutMs);
    return ErrorCode::kInvalidCall;
}

int32_t CConnectionImpl::Recv(void *pData, uint32_t uLength, uint32_t uTimeoutMs)
{
    UNSED(pData);
    UNSED(uLength);
    UNSED(uTimeoutMs);
    return ErrorCode::kInvalidCall;
}

int32_t CConnectionImpl::Call(IMessage *pRequest, IMessage *pResponse, uint32_t uTimeoutMs)
{
    UNSED(pRequest);
    UNSED(pResponse);
    UNSED(uTimeoutMs);
    return ErrorCode::kInvalidCall;
}

int32_t CConnectionImpl::Call(const uint8_t *pRequest, uint32_t uRequestLength, IMessage *pResponse, uint32_t uTimeoutMs)
{
    UNSED(pRequest);
    UNSED(uRequestLength);
    UNSED(pResponse);
    UNSED(uTimeoutMs);
    return ErrorCode::kInvalidCall;
}

int32_t CConnectionImpl::OnConnected()
{
    if (m_pCallback != nullptr)
    {
        return m_pCallback->OnConnected(this);
    }
    return ErrorCode::kSuccess;
}

void CConnectionImpl::OnDisconnected()
{
    if (m_iFd != -1)
    {
        close(m_iFd);
        m_iFd = -1;
    }

    if (m_pCallback != nullptr)
    {
        m_pCallback->OnDisconnected(this);
    }
}

int32_t CConnectionImpl::DeliverMessage(IMessage *pMessage)
{
    uint32_t uDeliverLength = 0;
    auto uMessageLength = m_pCallback->OnMessageLength(pMessage->GetData(), pMessage->GetDataLength());
    if (unlikely(uMessageLength == 0))
    {
        return ErrorCode::kSuccess;
    }
    else if (unlikely(uMessageLength == UINT32_MAX))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} invalid message", m_strConnectionName.c_str());
        return ErrorCode::kInvalidState;
    }
    else
    {
        if (likely(pMessage->GetDataLength() == uMessageLength))
        {
            m_pCallback->OnMessage(this, pMessage);
            return ErrorCode::kSuccess;
        }
        else if (pMessage->GetDataLength() > uMessageLength)
        {
            // 不止一个消息
            while (uDeliverLength < pMessage->GetDataLength())
            {
                auto pNewMessage = NewMessage(uMessageLength);
                if (unlikely(pNewMessage == nullptr))
                {
                    LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create message", m_strConnectionName.c_str());
                    return ErrorCode::kOutOfMemory;
                }
                pNewMessage->Append(pMessage->GetData() + uDeliverLength, uMessageLength);
                pNewMessage->SetSize(uMessageLength);
                uDeliverLength += uMessageLength;
                m_pCallback->OnMessage(this, pNewMessage);
            }
        }
        else
        {
            // 不足一个消息
            return ErrorCode::kSuccess;
        }
    }
}

int32_t CConnectionImpl::Recv(uint32_t uSize)
{
    int32_t iTotalRecv = 0;
    while (iTotalRecv < uSize)
    {
        auto pData = m_pMessageRecv->GetData();
        auto uCapacity = m_pMessageRecv->GetCapacity();
        auto iRecv = recv(m_iFd, pData + iTotalRecv, uCapacity - iTotalRecv, 0);
        if (likely(iRecv > 0))
        {
            iTotalRecv += iRecv;
            m_pMessageRecv->SetSize(m_pMessageRecv->GetDataLength() + iRecv);
            auto iRet = DeliverMessage(m_pMessageRecv);
            if (unlikely(iRet != ErrorCode::kSuccess))
            {
                LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to deliver message", 
                    m_strConnectionName.c_str());
                return -1;
            }
        }
        else if (iRecv == 0)
        {
            return 0;
        }
        else if (iRecv == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 可能 return 0？
                return iTotalRecv;
            }

            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to recv: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return -1;
        }
    }
    return iTotalRecv;
}

}
}
