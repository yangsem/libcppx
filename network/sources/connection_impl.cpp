#include "connection_impl.h"
#include "callback.h"
#include "engine.h"
#include "task_queue.h"
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

CConnectionImpl::CConnectionImpl(uint64_t uID, ICallback *pCallback, CTaskQueue *pTaskQueue, NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx)
    : m_uID(uID), m_pCallback(pCallback), m_pTaskQueue(pTaskQueue), m_pLogger(pLogger), m_pAllocatorEx(pAllocatorEx)
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
    m_pTaskQueue = nullptr;
    m_pLogger = nullptr;
    m_pAllocatorEx = nullptr;
    m_strRemoteIP.clear();
    m_strLocalIP.clear();
    m_uRemotePort = 0;
    m_uLocalPort = 0;
    m_uID = 0;
    m_uIOThreadIndex = 0;
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
        m_uSocketBufferSize = pConfig->GetUint32(config::kSocketBufferBytes, default_value::kSocketBufferBytes);
        m_uHeartbeatIntervalMs = pConfig->GetUint32(config::kHeartbeatIntervalMs, default_value::kHeartbeatIntervalMs);
        m_uHeartbeatTimeoutMs = pConfig->GetUint32(config::kHeartbeatTimeoutMs, default_value::kHeartbeatTimeoutMs);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to init connection", m_strConnectionName.c_str());
        return ErrorCode::kThrowException;
    }

    m_iFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (m_iFd == -1)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to create socket: {}", 
            m_strConnectionName.c_str(), strerror(errno));
        return ErrorCode::kSystemError;
    }

    if (m_uSocketBufferSize != 0)
    {
        if (setsockopt(m_iFd, SOL_SOCKET, SO_RCVBUF, &m_uSocketBufferSize, sizeof(m_uSocketBufferSize)) == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to set socket buffer size: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} set socket buffer size to {}", 
            m_strConnectionName.c_str(), Wrap(m_uSocketBufferSize));

        if (setsockopt(m_iFd, SOL_SOCKET, SO_SNDBUF, &m_uSocketBufferSize, sizeof(m_uSocketBufferSize)) == -1)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to set socket buffer size: {}", 
                m_strConnectionName.c_str(), strerror(errno));
            return ErrorCode::kSystemError;
        }
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} set socket buffer size to {}", 
            m_strConnectionName.c_str(), Wrap(m_uSocketBufferSize));
    }

    return ErrorCode::kSuccess;
}

int32_t CConnectionImpl::Connect(const char *pRemoteIP, uint16_t uRemotePort)
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
        auto iRet = poll(&pfd, 1, m_uConnectTimeoutMs);
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
        if (m_pTaskQueue->PostTask(task) != 0)
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
    if (m_pTaskQueue->PostTask(task) != 0)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kSystemError, "{} failed to post task", m_strConnectionName.c_str());
        return;
    }

    if (m_bIsSyncConnect)
    {
        while (m_pTaskQueue->IsRunning() && !bTaskDone)
        {
            usleep(1000); // 1ms
        }

        if (!m_pTaskQueue->IsRunning())
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

}
}
