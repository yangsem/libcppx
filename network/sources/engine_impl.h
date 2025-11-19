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
#include "event_dispatcher.h"
#include "io_dispatcher.h"

namespace cppx
{
namespace network
{

class CEngineImpl : public IEngine
{
public:
    CEngineImpl(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
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
    CMessagePool m_MessagePool;
    base::memory::IAllocatorEx *m_pAllocatorEx{nullptr};

    NetworkLogger *m_pLogger{nullptr};
    std::string m_strEngineName;
    
    CEventDispatcher m_EventDispatcher;
    std::vector<std::unique_ptr<CIODispatcher>> m_vecIODispatchers;

    std::mutex m_mutex;
    uint64_t m_uNextAcceptorID{1};
    std::unordered_map<uint64_t, std::unique_ptr<CAcceptorImpl>> m_umapAcceptor;
    uint64_t m_uNextConnectionID{1};
    std::unordered_map<uint64_t, std::unique_ptr<CConnectionImpl>> m_umapConnection;
};

}
}
#endif // __CPPX_NETWORK_ENGINE_IMPL_H__
