#include "engine_impl.h"
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <logger/logger_ex.h>
#include <sys/epoll.h>

namespace cppx
{
namespace network
{

CEngineImpl::CEngineImpl(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx) 
    : m_MessagePool(pAllocatorEx), m_pAllocatorEx(pAllocatorEx)
    , m_pLogger(pLogger), m_EventDispatcher(pLogger, pAllocatorEx)
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
        auto uIOThreadCount = pConfig->GetUint32(config::kIOThreadCount, default_value::kIOThreadCount);
        uIOThreadCount = std::max(uIOThreadCount, 1u);
        m_vecIODispatchers.resize(uIOThreadCount);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kThrowException, "Failed to init engine");
        return ErrorCode::kThrowException;
    }

    m_MessagePool.Init(0, 0);

    auto iErrorNo = m_EventDispatcher.Init(m_strEngineName.c_str());
    if (iErrorNo != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to init event dispatcher");
        return iErrorNo;
    }
    for (auto &upIODispatcher : m_vecIODispatchers)
    {
        upIODispatcher.reset(m_pAllocatorEx->New<CIODispatcher>(m_pLogger, m_pAllocatorEx));
        if (upIODispatcher == nullptr)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "Failed to create io dispatcher");
            return ErrorCode::kOutOfMemory;
        }
        iErrorNo = upIODispatcher->Init();
        if (iErrorNo != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to init io dispatcher");
            return iErrorNo;
        }
    }

    return ErrorCode::kSuccess;
}

void CEngineImpl::Exit()
{
    m_EventDispatcher.Exit();
    for (auto &upIODispatcher : m_vecIODispatchers)
    {
        upIODispatcher->Exit();
    }
    m_vecIODispatchers.clear();

    m_umapAcceptor.clear();
    m_umapConnection.clear();

    m_pAllocatorEx = nullptr;
    m_pLogger = nullptr;
    m_strEngineName.clear();
}

int32_t CEngineImpl::Start()
{
    auto iErrorNo = m_EventDispatcher.Start();
    if (iErrorNo != ErrorCode::kSuccess)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to start event dispatcher");
        return iErrorNo;
    }
    for (auto &upIODispatcher : m_vecIODispatchers)
    {
        iErrorNo = upIODispatcher->Start();
        if (iErrorNo != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kInvalidCall, "Failed to start io dispatcher");
            return iErrorNo;
        }
    }
    return iErrorNo;
}

void CEngineImpl::Stop()
{
    m_EventDispatcher.Stop();
    for (auto &upIODispatcher : m_vecIODispatchers)
    {
        upIODispatcher->Stop();
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
            m_pAllocatorEx->New<CAcceptorImpl>(m_uNextAcceptorID, pCallback, &m_EventDispatcher, m_pLogger, m_pAllocatorEx));
        if (upAcceptor == nullptr || upAcceptor->Init(pConfig) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "Failed to create acceptor");
            return ErrorCode::kOutOfMemory;
        }
        m_umapAcceptor[m_uNextAcceptorID] = std::move(upAcceptor);

        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} create acceptor name: {}, id: {} bind {}:{}", 
            m_strEngineName.c_str(), upAcceptor->GetName(), Wrap(m_uNextAcceptorID), 
            upAcceptor->GetIP(), Wrap(upAcceptor->GetPort()));

        m_uNextAcceptorID++;
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
    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} destroy acceptor name: {}, id: {} bind {}:{}", 
        m_strEngineName.c_str(), it->second->GetName(), Wrap(pAcceptor->GetID()), 
        it->second->GetIP(), Wrap(it->second->GetPort()));

    m_umapAcceptor.erase(it);
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
        auto upConnection = std::unique_ptr<CConnectionImpl>(m_pAllocatorEx->New<CConnectionImpl>(m_uNextConnectionID, pCallback, &m_EventDispatcher, m_pLogger, m_pAllocatorEx));
        if (upConnection == nullptr || upConnection->Init(pConfig) != ErrorCode::kSuccess)
        {
            LOG_ERROR(m_pLogger, ErrorCode::kOutOfMemory, "{} failed to create connection", m_strEngineName.c_str());
            return ErrorCode::kOutOfMemory;
        }
        m_umapConnection[m_uNextConnectionID] = std::move(upConnection);

        LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} create connection name: {}, id: {} remote {}:{}", 
            m_strEngineName.c_str(), upConnection->GetName(), Wrap(m_uNextConnectionID), 
            upConnection->GetRemoteIP(), Wrap(upConnection->GetRemotePort()));

        m_uNextConnectionID++;
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

    LOG_EVENT(m_pLogger, ErrorCode::kEvent, "{} destroy connection name: {}, id: {} remote {}:{}", 
        m_strEngineName.c_str(), it->second->GetName(), Wrap(pConnection->GetID()), 
        it->second->GetRemoteIP(), Wrap(it->second->GetRemotePort()));

    m_umapConnection.erase(it);
}

int32_t CEngineImpl::DetachConnection(IConnection *pConnection)
{
    if (pConnection == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    auto pConnectionImpl = static_cast<CConnectionImpl *>(pConnection);
    return pConnectionImpl->Detach();
}

int32_t CEngineImpl::AttachConnection(IConnection *pConnection)
{
    if (pConnection == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }

    auto pConnectionImpl = static_cast<CConnectionImpl *>(pConnection);
    return pConnectionImpl->Attach();
}

int32_t CEngineImpl::GetStats(NetworkStats *pStats) const
{
    if (pStats == nullptr)
    {
        LOG_ERROR(m_pLogger, ErrorCode::kInvalidParam, "Invalid parameters");
        return ErrorCode::kInvalidParam;
    }
    
    for (auto &upAcceptor : m_umapAcceptor)
    {
        upAcceptor.second->GetStats(pStats);
    }
    for (auto &upConnection : m_umapConnection)
    {
        upConnection.second->GetStats(pStats);
    }
    return ErrorCode::kSuccess;
}

}
}