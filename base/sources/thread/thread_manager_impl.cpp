#include "thread_manager_impl.h"
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <utilities/error_code.h>

namespace cppx
{
namespace base
{

IThreadManager *IThreadManager::Create()
{
    return NEW CThreadManagerImpl();
}

void IThreadManager::Destroy(IThreadManager *pThreadManager)
{
    if (pThreadManager != nullptr)
    {
        delete pThreadManager;
    }
}

IThreadManager *IThreadManager::GetInstance()
{
    static CThreadManagerImpl s_threadManager;
    return &s_threadManager;
}

int32_t CThreadManagerImpl::RegisterThreadEventFunc(ThreadEventFunc pThreadEventFunc, void *pUserParam)
{
    if (pThreadEventFunc == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_vecThreadEventFuncs.emplace_back(pThreadEventFunc, pUserParam);
    }
    catch(std::exception &e)
    {
        SetLastError(ErrorCode::kThrowException);
        return ErrorCode::kThrowException;
    }

    return 0;
}

IThread *CThreadManagerImpl::CreateThread()
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
        SetLastError(ErrorCode::kThrowException);
        return nullptr;
    }

    return pThread;
}

void CThreadManagerImpl::DestroyThread(IThread *pThread)
{
    if (pThread != nullptr)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_setThreads.erase(pThread);
        IThread::Destroy(pThread);
    }
}

int32_t CThreadManagerImpl::CreateThread(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadParam)
{
    if (pThreadName == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    if (pThreadFunc == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    IThread *pThread = nullptr;
    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (m_mapThreads.count(pThreadName) != 0)
        {
            SetLastError(ErrorCode::kInvalidParam);
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
        SetLastError(ErrorCode::kThrowException);
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CThreadManagerImpl::DestroyThread(const char *pThreadName)
{
    if (pThreadName == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    std::lock_guard<std::mutex> lock(m_lock);
    auto iter = m_mapThreads.find(pThreadName);
    if (iter == m_mapThreads.end())
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    IThread *pThread = iter->second;
    IThread::Destroy(pThread);
    m_mapThreads.erase(iter);
    return 0;
}

int32_t CThreadManagerImpl::NewThreadLocalId()
{
    // 暂时先这么做，后续再优化
    return m_iThreadLocalId++;
}

void CThreadManagerImpl::FreeThreadLocalId(int32_t iThreadLocalId)
{
    // 暂时先这么做，后续再优化
    UNSED(iThreadLocalId);
}

void* CThreadManagerImpl::GetThreadLocal(int32_t iThreadLocalId, uint64_t uThreadLocalSize)
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
        SetLastError(ErrorCode::kThrowException);
        return nullptr;
    }
    return nullptr;
}

int32_t CThreadManagerImpl::ForEachAllThreadLocal(int32_t iThreadLocalId, IThreadManager::ThreadLocalForEachFunc pThreadLocalForEachFunc, void *pUserParam)
{
    if (pThreadLocalForEachFunc == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
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

int32_t CThreadManagerImpl::GetStats(IJson *pJson) const
{
    if (pJson == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    pJson->Clear();
    return 0;
}

}
}
