#ifndef __CPPX_NETWORK_ACCEPTOR_IMPL_H__
#define __CPPX_NETWORK_ACCEPTOR_IMPL_H__

#include <acceptor.h>
#include <cstdint>
#include <engine.h>
#include <string>
#include <netinet/in.h>
#include "connection_impl.h"
#include "memory/allocator_ex.h"
#include "dispatcher.h"

namespace cppx
{
namespace network
{

class CAcceptorImpl final : public IAcceptor {
public:
    CAcceptorImpl(uint64_t uID, ICallback *pCallback, IDispatcher *pDispatcher,
                  NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocator);
    ~CAcceptorImpl() override;

    int32_t Init(NetworkConfig *pConfig);

    int32_t Start() override;
    void Stop() override;

    CConnectionImpl *Accept();
    
    uint64_t GetID() const override { return m_uID; }

    int32_t GetFd() const { return m_iFd; }
    const char *GetName() const override { return m_strAcceptorName.c_str(); }
    const char *GetIP() const { return m_strAcceptorIP.c_str(); }
    uint16_t GetPort() const { return m_uAcceptorPort; }

    int32_t GetStats(NetworkStats *pStats) const;

private:
    int32_t m_iFd{-1};
    uint64_t m_uID{0};
    ICallback *m_pCallback{nullptr};
    IDispatcher *m_pDispatcher{nullptr};
    base::memory::IAllocatorEx *m_pAllocatorEx{nullptr};
    NetworkLogger *m_pLogger{nullptr};
    std::string m_strAcceptorName;
    std::string m_strAcceptorIP;
    uint16_t m_uAcceptorPort{0};

    uint32_t m_uSocketSendBufferSize{0};
    uint32_t m_uSocketRecvBufferSize{0};
    uint32_t m_uHeartbeatIntervalMs{0};
    uint32_t m_uHeartbeatTimeoutMs{0};
};

}
}
#endif // __CPPX_NETWORK_ACCEPTOR_IMPL_H__
