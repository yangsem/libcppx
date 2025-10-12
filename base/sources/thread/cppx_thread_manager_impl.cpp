#include "cppx_thread_manager_impl.h"
#include <utilities/cppx_common.h>
#include <utilities/cppx_last_error.h>
#include <utilities/cppx_error_code.h>

namespace cppx
{
namespace base
{

IThreadManager *IThreadManager::Create() noexcept
{
    return NEW CThreadManagerImpl();
}

void IThreadManager::Destroy(IThreadManager *pThreadManager) noexcept
{
    if (pThreadManager != nullptr)
    {
        delete pThreadManager;
    }
}

IThreadManager *IThreadManager::GetInstance() noexcept
{
    static CThreadManagerImpl s_threadManager;
    return &s_threadManager;
}

int32_t CThreadManagerImpl::RegisterThreadEventFunc(ThreadEventFunc pThreadEventFunc, void *pUserParam) noexcept
{
    if (pThreadEventFunc == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "ThreadEventFunc is nullptr");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_vecThreadEventFuncs.emplace_back(pThreadEventFunc, pUserParam);
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "RegisterThreadEventFunc throw exception %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

IThread *CThreadManagerImpl::CreateThread() noexcept
{
    IThread *pThread = nullptr;
    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        pThread = IThread::Create("", nullptr, nullptr);
        if (pThread != nullptr)
        {
            m_setThreads.insert(pThread);
        }
    }
    catch(std::exception &e)
    {
        if (pThread != nullptr)
        {
            IThread::Destroy(pThread);
        }
        SET_LAST_ERROR(ErrorCode::kThrowException, "CreateThread throw exception %s", e.what());
        return nullptr;
    }

    return pThread;
}

void CThreadManagerImpl::DestroyThread(IThread *pThread) noexcept
{
    if (pThread != nullptr)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_setThreads.erase(pThread);
        IThread::Destroy(pThread);
    }
}

int32_t CThreadManagerImpl::CreateThread(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadParam) noexcept
{
    if (pThreadName == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Thread name is nullptr");
        return ErrorCode::kInvalidParam;
    }

    if (pThreadFunc == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Thread func is nullptr");
        return ErrorCode::kInvalidParam;
    }

    IThread *pThread = nullptr;
    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (m_mapThreads.count(pThreadName) != 0)
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Thread %s already exists", pThreadName);
            return ErrorCode::kInvalidParam;
        }
        pThread = IThread::Create(pThreadName, pThreadFunc, pThreadParam);
        if (pThread != nullptr)
        {
            m_mapThreads[pThreadName] = pThread;
        }
    }
    catch(std::exception &e)
    {
        if (pThread != nullptr)
        {
            IThread::Destroy(pThread);
        }
        SET_LAST_ERROR(ErrorCode::kThrowException, "CreateThread throw exception %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CThreadManagerImpl::DestroyThread(const char *pThreadName) noexcept
{
    if (pThreadName == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Thread name is nullptr");
        return ErrorCode::kInvalidParam;
    }

    std::lock_guard<std::mutex> lock(m_lock);
    auto iter = m_mapThreads.find(pThreadName);
    if (iter == m_mapThreads.end())
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Thread %s not found", pThreadName);
        return ErrorCode::kInvalidParam;
    }

    IThread *pThread = iter->second;
    IThread::Destroy(pThread);
    m_mapThreads.erase(iter);
    return 0;
}

int32_t CThreadManagerImpl::NewThreadLocalId() noexcept
{
    // 暂时先这么做，后续再优化
    return m_iThreadLocalId++;
}

void CThreadManagerImpl::FreeThreadLocalId(int32_t iThreadLocalId) noexcept
{
    // 暂时先这么做，后续再优化
    UNSED(iThreadLocalId);
}

void* CThreadManagerImpl::GetThreadLocal(int32_t iThreadLocalId, uint64_t uThreadLocalSize) noexcept
{
    // 暂时先这么做，后续再优化
    static thread_local int32_t tlsTid = -1;
    if (unlikely(tlsTid == -1))
    {
        tlsTid = gettid();
    }
    
    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        auto &threadLocals = m_mapThreadLocals[tlsTid];
        auto &threadLocal = threadLocals[iThreadLocalId];
        if (threadLocal != nullptr)
        {
            return threadLocal;
        }
        threadLocal = new uint8_t[uThreadLocalSize];
        return threadLocal;
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "GetThreadLocal throw exception %s", e.what());
        return nullptr;
    }
    return nullptr;
}

int32_t CThreadManagerImpl::ForEachAllThreadLocal(int32_t iThreadLocalId, IThreadManager::ThreadLocalForEachFunc pThreadLocalForEachFunc, void *pUserParam) noexcept
{
    if (pThreadLocalForEachFunc == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "ThreadLocalForEachFunc is nullptr");
        return ErrorCode::kInvalidParam;
    }

    std::lock_guard<std::mutex> lock(m_lock);
    for (auto &threadLocals : m_mapThreadLocals)
    {
        auto iter = threadLocals.second.find(iThreadLocalId);
        if (likely(iter != threadLocals.second.end()))
        {
            auto bRet = pThreadLocalForEachFunc(iter->second, pUserParam);
            if (unlikely(!bRet))
            {
                break;
            }
        }
    }
    return 0;
}

}
}
