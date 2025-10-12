#ifndef __CPPX_THREAD_MANAGER_IMPL_H__
#define __CPPX_THREAD_MANAGER_IMPL_H__

#include <thread/cppx_thread_manager.h>
#include <thread/cppx_thread.h>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <atomic>
#include <unordered_map>

namespace cppx
{
namespace base
{

class CThreadManagerImpl final : public IThreadManager
{
public:
    CThreadManagerImpl() noexcept = default;
    ~CThreadManagerImpl() noexcept override = default;

    CThreadManagerImpl(const CThreadManagerImpl &) = delete;
    CThreadManagerImpl &operator=(const CThreadManagerImpl &) = delete;
    CThreadManagerImpl(CThreadManagerImpl &&) = delete;
    CThreadManagerImpl &operator=(CThreadManagerImpl &&) = delete;

    int32_t RegisterThreadEventFunc(ThreadEventFunc pThreadEventFunc, void *pUserParam) noexcept override;
    
    IThread *CreateThread() noexcept override;  
    void DestroyThread(IThread *pThread) noexcept override;

    int32_t CreateThread(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadParam) noexcept override;
    int32_t DestroyThread(const char *pThreadName) noexcept override;

    int32_t NewThreadLocalId() noexcept override;
    void FreeThreadLocalId(int32_t iThreadLocalId) noexcept override;

    void* GetThreadLocal(int32_t iThreadLocalId, uint64_t uThreadLocalSize) noexcept override;
    int32_t ForEachAllThreadLocal(int32_t iThreadLocalId, IThreadManager::ThreadLocalForEachFunc pThreadLocalForEachFunc, void *pUserParam) noexcept override;

private:
    std::atomic<int32_t> m_iThreadLocalId {0};
    // tid -> thread_local_id -> thread_local_data
    std::unordered_map<int32_t, std::unordered_map<int32_t, void *>> m_mapThreadLocals;

    std::mutex m_lock;
    // 线程事件函数和用户参数
    std::vector<std::pair<ThreadEventFunc, void *>> m_vecThreadEventFuncs;

    std::set<IThread *> m_setThreads;
    std::map<std::string, IThread *> m_mapThreads;
};

}
}

#endif // __CPPX_THREAD_MANAGER_IMPL_H__