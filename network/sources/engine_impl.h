#ifndef __CPPX_NETWORK_ENGINE_IMPL_H__
#define __CPPX_NETWORK_ENGINE_IMPL_H__

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

#include <engine.h>
#include <memory/allocator_ex.h>
#include <thread/thread_manager.h>

#include "acceptor_impl.h"
#include "connection_impl.h"
#include "message_pool.h"

namespace cppx
{
namespace network
{

class CEngineImpl : public IEngine
{
public:
    CEngineImpl(NetworkLogger *pLogger);
    ~CEngineImpl() override;

    int32_t Init(NetworkConfig *pConfig) override;
    void Exit() override;
    int32_t Start() override;
    void Stop() override;
    int32_t CreateAcceptor(NetworkConfig *pConfig, ICallback *pCallback) override;
    void DestroyAcceptor(IAcceptor *pAcceptor) override;
    int32_t CreateConnection(NetworkConfig *pConfig, ICallback *pCallback) override;
    void DestroyConnection(IConnection *pConnection) override;
    int32_t DetachConnection(IConnection *pConnection) override;
    int32_t AttachConnection(IConnection *pConnection) override;
    int32_t GetStats(NetworkStats *pStats) const override;

private:
    static bool ManagerThreadFunc(void *pThreadArg);
    static bool IOThreadFunc(void *pThreadArg);

private:
    int32_t m_iEpollFd{-1};
    std::vector<struct pollfd> m_vecPollFds;
    base::memory::IAllocatorEx *m_pAllocatorEx{nullptr};
    CTaskQueue m_TaskQueue;
    CMessagePool m_MessagePool;
    std::mutex m_mutex;
    uint64_t m_uNextID{1};
    std::unordered_map<uint64_t, std::unique_ptr<CAcceptorImpl>> m_umapAcceptor;
    std::unordered_map<uint64_t, std::unique_ptr<CConnectionImpl>> m_umapConnection;
    NetworkLogger *m_pLogger{nullptr};
    std::string m_strEngineName;
    base::IThreadManager *m_pThreadManager{nullptr};
    base::IThread *m_pManagerThread{nullptr};
    std::vector<base::IThread *> m_vecIOThreads;
    uint32_t m_uIOThreadCount{0};
    uint32_t m_uIOReadWriteBytes{0};
    volatile bool m_bRunning{false};

};

}
}
#endif // __CPPX_NETWORK_ENGINE_IMPL_H__
