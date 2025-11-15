#include "engine_impl.h"
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <logger/logger_ex.h>
#include <sys/epoll.h>

namespace cppx
{
namespace network
{

CEngineImpl::CEngineImpl(NetworkLogger *pLogger) : m_pLogger(pLogger)
{
}

CEngineImpl::~CEngineImpl()
{
    Exit();
}

int32_t CEngineImpl::Init(NetworkConfig *pConfig)
{
    if (pConfig == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        m_strEngineName = pConfig->GetString(config::kEngineName, default_value::kEngineName);
        m_uIOThreadCount = pConfig->GetUint32(config::kIOThreadCount, default_value::kIOThreadCount);
        m_uIOThreadCount = std::max(m_uIOThreadCount, 1u);
        m_vecIOThreads.resize(m_uIOThreadCount);
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} io thread count: {}", m_strEngineName.c_str(), Wrap(m_uIOThreadCount));

        m_uIOReadWriteBytes = pConfig->GetUint32(config::kIOReadWriteBytes, default_value::kIOReadWriteBytes);
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} io read write bytes: {}", m_strEngineName.c_str(), Wrap(m_uIOReadWriteBytes));
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "Failed to init engine");
        return ErrorCode::kThrowException;
    }

    m_pAllocatorEx = base::memory::IAllocatorEx::GetInstance();

    m_pThreadManager = base::IThreadManager::GetInstance();
    m_pManagerThread = m_pThreadManager->CreateThread();
    if (m_pManagerThread == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "Failed to create manager thread");
        return ErrorCode::kOutOfMemory;
    }
    char szThreadName[16];
    snprintf(szThreadName, sizeof(szThreadName), "net_manager_%s", m_strEngineName.c_str());
    auto iErrorNo = m_pManagerThread->Bind(szThreadName, &CEngineImpl::ManagerThreadFunc, this);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to bind manager thread %s", szThreadName);
        return iErrorNo;
    }
    for (uint32_t i = 0; i < m_uIOThreadCount; ++i)
    {
        m_vecIOThreads[i] = m_pThreadManager->CreateThread();
        if (m_vecIOThreads[i] == nullptr)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "Failed to create io thread");
            return ErrorCode::kOutOfMemory;
        }
        char szThreadName[16];
        snprintf(szThreadName, sizeof(szThreadName), "net_io_%s_%u", m_strEngineName.c_str(), i);
        auto iErrorNo = m_vecIOThreads[i]->Bind(szThreadName, &CEngineImpl::IOThreadFunc, this);
        if (iErrorNo != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to bind io thread %s", szThreadName);
            return iErrorNo;
        }
    }

    return ErrorCode::kSuccess;
}

void CEngineImpl::Exit()
{
    if (m_bRunning)
    {
        Stop();
    }

    if (m_pManagerThread != nullptr)
    {
        m_pThreadManager->DestroyThread(m_pManagerThread);
        m_pManagerThread = nullptr;
    }
    for (auto &pIOThread : m_vecIOThreads)
    {
        if (pIOThread != nullptr)
        {
            m_pThreadManager->DestroyThread(pIOThread);
            pIOThread = nullptr;
        }
    }

    m_pAllocatorEx = nullptr;
    m_pLogger = nullptr;
    m_strEngineName.clear();
    m_pThreadManager = nullptr;
    m_pManagerThread = nullptr;
    m_vecIOThreads.clear();
    m_uIOThreadCount = 0;
    m_uIOReadWriteBytes = 0;
    m_bRunning = false;
}

int32_t CEngineImpl::Start()
{
    if (m_bRunning)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} is already started", m_strEngineName.c_str());
        return ErrorCode::kInvalidCall;
    }

    m_bRunning = true;
    if (m_pManagerThread->Start() != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to start manager thread");
        return ErrorCode::kInvalidCall;
    }
    for (auto &pIOThread : m_vecIOThreads)
    {
        if (pIOThread->Start() != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to start io thread");
            return ErrorCode::kInvalidCall;
        }
    }
    return ErrorCode::kSuccess;
}

void CEngineImpl::Stop()
{
    if (!m_bRunning)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "{} is not started", m_strEngineName.c_str());
        return;
    }

    m_bRunning = false;
    m_pManagerThread->Stop();
    for (auto &pIOThread : m_vecIOThreads)
    {
        pIOThread->Stop();
    }
}

int32_t CEngineImpl::CreateAcceptor(NetworkConfig *pConfig, ICallback *pCallback)
{
    if (pConfig == nullptr || pCallback == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto upAcceptor = std::unique_ptr<CAcceptorImpl>(
            m_pAllocatorEx->New<CAcceptorImpl>(m_uNextID, pCallback, &m_TaskQueue, m_pLogger, m_pAllocatorEx));
        if (upAcceptor == nullptr || upAcceptor->Init(pConfig) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "Failed to create acceptor");
            return ErrorCode::kOutOfMemory;
        }
        m_umapAcceptor[m_uNextID] = std::move(upAcceptor);
        m_uNextID++;
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} create acceptor id: {}", m_strEngineName.c_str(), Wrap(m_uNextID));
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "Failed to create listener");
        return ErrorCode::kThrowException;
    }

    return ErrorCode::kSuccess;
}

void CEngineImpl::DestroyAcceptor(IAcceptor *pAcceptor)
{
    if (pAcceptor == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_umapAcceptor.find(pAcceptor->GetID());
    if (it == m_umapAcceptor.end())
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Acceptor not found");
        return;
    }
    m_umapAcceptor.erase(it);
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} destroy acceptor id: {}", m_strEngineName.c_str(), Wrap(pAcceptor->GetID()));
}

int32_t CEngineImpl::CreateConnection(NetworkConfig *pConfig, ICallback *pCallback)
{
    if (pConfig == nullptr || pCallback == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto upConnection = std::unique_ptr<CConnectionImpl>(m_pAllocatorEx->New<CConnectionImpl>(m_uNextID, pCallback, &m_TaskQueue, m_pLogger, m_pAllocatorEx));
        if (upConnection == nullptr || upConnection->Init(pConfig) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create connection", m_strEngineName.c_str());
            return ErrorCode::kOutOfMemory;
        }
        m_umapConnection[m_uNextID] = std::move(upConnection);
        m_uNextID++;
        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} create connection id: {}", m_strEngineName.c_str(), Wrap(m_uNextID));
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "{} failed to create connection", m_strEngineName.c_str());
        return ErrorCode::kThrowException;
    }

    return ErrorCode::kSuccess;
}

void CEngineImpl::DestroyConnection(IConnection *pConnection)
{
    if (pConnection == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_umapConnection.find(pConnection->GetID());
    if (it == m_umapConnection.end())
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Connection not found");
        return;
    }
    m_umapConnection.erase(it);
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} destroy connection id: {}", m_strEngineName.c_str(), Wrap(pConnection->GetID()));
}

int32_t CEngineImpl::DetachConnection(IConnection *pConnection)
{
    if (pConnection == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }
    return ErrorCode::kSuccess;
}

int32_t CEngineImpl::GetStats(NetworkStats *pStats) const
{
    if (pStats == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }
    return ErrorCode::kSuccess;
}

}
}