#include "connection_impl.h"
#include "callback.h"
#include "engine.h"
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

int32_t CConnectionImpl::InitChannel()
{
    m_pSpinLockSend = base::SpinLock::Create();
    if (m_pSpinLockSend == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to create spin lock", m_strConnectionName.c_str());
        return ErrorCode::kSystemError;
    }

    if (m_bIsASyncSend)
    {
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
    }
    else
    {
        m_pChannelSend = nullptr;
        m_pChannelPrioritySend = nullptr;
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
        m_bIsASyncSend = pConfig->GetBool(config::kIsASyncSend, default_value::kIsASyncSend);
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

    if (InitChannel() != ErrorCode::kSuccess)
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
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to connect to {}:{}", 
            m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
        return ErrorCode::kSystemError;
    }
    else if (iRet == 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kEvent, "{} connected to {}:{}", 
            m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
        return ErrorCode::kSystemError;
    }

    if (m_bIsSyncConnect)
    {
        struct pollfd pfd;
        pfd.fd = m_iFd;
        pfd.events = POLLIN;
        auto iRet = poll(&pfd, 1, uTimeoutMs == 0 ? m_uConnectTimeoutMs : uTimeoutMs);
        if (iRet == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to poll: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        else if (iRet == 0)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} connect to {}:{} timeout", 
                m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
            return ErrorCode::kInvalidState;
        }
        else
        {
            // 查询连接状态
            int32_t iStatus = 0;
            socklen_t iStatusLen = sizeof(iStatus);
            if (getsockopt(m_iFd, SOL_SOCKET, SO_ERROR, &iStatus, &iStatusLen) == -1)
            {
                LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to get socket status: {}", 
                    m_strConnectionName.c_str(), strerror(errno));
                return ErrorCode::kSystemError;
            }
            if (iStatus != 0)
            {
                LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} connect to {}:{} failed: {}", 
                    m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort), strerror(iStatus));
                return ErrorCode::kSystemError;
            }
            LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} connected to {}:{} successfully", 
                m_strConnectionName.c_str(), m_strRemoteIP.c_str(), Wrap(m_uRemotePort));
        }
    }
    else
    {
        Task task;
        task.eTaskType = TaskType::kConnect;
        task.funcCallback = nullptr;
        task.iFd = m_iFd;
        if (m_pDispatcher->Post(task) != 0)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strConnectionName.c_str());
            return ErrorCode::kSystemError;
        }
    }

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
    task.eTaskType = TaskType::kDisconnect;
    task.funcCallback = nullptr;
    task.iFd = m_iFd;
    volatile bool bTaskDone = false;
    volatile bool bTaskResult = false;
    if (m_bIsSyncConnect)
    {
        task.funcCallback = [&] (bool bResult) { 
            bTaskResult = bResult;
            bTaskDone = true;
        };
    }
    if (m_pDispatcher->Post(task) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strConnectionName.c_str());
        return;
    }

    if (m_bIsSyncConnect)
    {
        while (m_pDispatcher->IsRunning() && !bTaskDone)
        {
            usleep(1000); // 1ms
        }

        if (!m_pDispatcher->IsRunning())
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} is not running", m_strConnectionName.c_str());
            return;
        }

        if (!bTaskResult)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to disconnect", m_strConnectionName.c_str());
            return;
        }
    }
}

int32_t CConnectionImpl::SendData(const uint8_t *pData, uint32_t uLength)
{
    uint32_t uSent = 0;
    while (uSent < uLength)
    {
        auto iRet = write(m_iFd, pData + uSent, uLength - uSent);
        if (unlikely(iRet == -1))
        {
            if (errno == EAGAIN)
            {
                usleep(0);
                continue;
            }
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to send message: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        uSent += iRet;
    }
    return ErrorCode::kSuccess;
}

int32_t CConnectionImpl::Send(IMessage *pMessage, bool bPriority)
{
    if (unlikely(pMessage == nullptr))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    base::SpinLockGuard guard(m_pSpinLockSend);
    if (!m_bIsASyncSend)
    {
        return SendData(pMessage->GetData(), pMessage->GetDataLength());
    }   
    else
    {
        auto channel = bPriority ? m_pChannelPrioritySend : m_pChannelSend;
        auto pData = channel->New();
        if (likely(pData != nullptr))
        {
            *reinterpret_cast<IMessage **>(pData) = pMessage;
            channel->Post(pData);
            return ErrorCode::kSuccess;
        }
        else
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to send message", m_strConnectionName.c_str());
            return ErrorCode::kOutOfMemory;
        }
    }
}

int32_t CConnectionImpl::Send(const uint8_t *pData, uint32_t uLength, bool bPriority)
{
    if (unlikely(pData == nullptr || uLength == 0))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    if (!m_bIsASyncSend)
    {
        return SendData(pData, uLength);
    }
    else
    {
        auto pMessage = NewMessage(uLength);
        if (likely(pMessage != nullptr))
        {
            pMessage->Append(pData, uLength);
            return Send(pMessage, bPriority);
        }
        else
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create message", m_strConnectionName.c_str());
            return ErrorCode::kOutOfMemory;
        }
    }
}

int32_t CConnectionImpl::Recv(IMessage **ppMessage, uint32_t uTimeoutMs)
{
    if (unlikely(ppMessage == nullptr))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    constexpr uint32_t kMessageBufferSize = 1024;
    auto pMessage = NewMessage(kMessageBufferSize);
    if (unlikely(pMessage == nullptr))
    {
        LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create message", m_strConnectionName.c_str());
        return ErrorCode::kOutOfMemory;
    }

    auto pData = pMessage->GetData();
    auto uLength = pMessage->GetDataLength();
    auto uReceived = 0;
    uint64_t uStartTimeNs = 0;
    uint64_t uEndTimeNs = 0;
    clock_get_time_nano(uStartTimeNs);
    uEndTimeNs = uStartTimeNs + uTimeoutMs * kMill;
    while (uStartTimeNs < uEndTimeNs)
    {
        auto iRet = read(m_iFd, pData + uReceived, kMessageBufferSize - uReceived);
        if (unlikely(iRet == -1))
        {
            if (errno == EAGAIN)
            {
                usleep(0);
                clock_get_time_nano(uStartTimeNs);
                continue;
            }
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to recv message: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        uReceived += iRet;
        auto uMsgLength = m_pCallback->OnMessageLength(pData, uReceived);
        if (uMsgLength == 0)
        {
            continue;
        }
        if (uMsgLength > uLength)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidState, "{} message length is too long", m_strConnectionName.c_str());
        }
        if (uReceived >= uLength)
        {
            break;
        }
        clock_get_time_nano(uStartTimeNs);
    }
    return ErrorCode::kSuccess;
}


}
}
