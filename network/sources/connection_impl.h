#ifndef __CPPX_NETWORK_CONNECTION_IMPL_H__
#define __CPPX_NETWORK_CONNECTION_IMPL_H__

#include <connection.h>
#include <callback.h>
#include <cstdint>
#include <engine.h>
#include <string>
#include "message_pool.h"
#include "task_queue.h"
#include "memory/allocator_ex.h"

namespace cppx
{
namespace network
{

class CConnectionImpl final : public IConnection 
{
    friend class CAcceptorImpl;
public:
    CConnectionImpl() = default;
    CConnectionImpl(uint64_t uID, ICallback *pCallback, CTaskQueue *pTaskQueue, 
                    NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CConnectionImpl() override;

    int32_t Init(NetworkConfig *pConfig);

    void SetID(uint64_t uID) { m_uID = uID; }
    void SetIOThreadIndex(uint32_t uIOThreadIndex) { m_uIOThreadIndex = uIOThreadIndex; }

    int32_t Connect(const char *pRemoteIP, uint16_t uRemotePort) override;
    void Close() override;

    IMessage *NewMessage(uint32_t uLength) override { return m_pMessagePool->NewMessage(uLength); }
    void DeleteMessage(IMessage *pMessage) override { m_pMessagePool->DeleteMessage(pMessage); }

    int32_t Send(IMessage *pMessage, bool bPriority = false) override;
    int32_t Send(const uint8_t *pData, uint32_t uLength, bool bPriority = false) override;
    int32_t Recv(IMessage *pMessage) override;
    int32_t Recv(void *pData, uint32_t uLength) override;

    int32_t Call(IMessage *pRequest, IMessage *pResponse) override;
    int32_t Call(const uint8_t *pRequest, uint32_t uRequestLength, IMessage *pResponse) override;

    bool IsConnected() const override { return m_iFd != -1; }
    uint64_t GetID() const override { return m_uID; }
    uint32_t GetIOThreadIndex() const override { return m_uIOThreadIndex; }
    const char *GetRemoteIP() const override { return m_strRemoteIP.c_str(); }
    uint16_t GetRemotePort() const override { return m_uRemotePort; }
    const char *GetLocalIP() const override { return m_strLocalIP.c_str(); }
    uint16_t GetLocalPort() const override { return m_uLocalPort; }

    int32_t GetFd() const { return m_iFd; }

private:
    uint64_t m_uID{0};
    int32_t m_iFd{-1};
    uint32_t m_uIOThreadIndex{0};
    ICallback *m_pCallback{nullptr};
    CMessagePool *m_pMessagePool{nullptr};
    uint64_t m_ulastRecvTimeNs{0};

    bool m_bIsASyncSend{false};
    // queue for async send


    CTaskQueue *m_pTaskQueue{nullptr};
    base::memory::IAllocatorEx *m_pAllocatorEx{nullptr};
    NetworkLogger *m_pLogger{nullptr};

    std::string m_strConnectionName;
    bool m_bIsSyncConnect{false};
    uint32_t m_uConnectTimeoutMs{30000};
    std::string m_strRemoteIP;
    std::string m_strLocalIP;
    uint16_t m_uRemotePort{0};
    uint16_t m_uLocalPort{0};

    uint32_t m_uSocketBufferSize{0};
    uint32_t m_uHeartbeatIntervalMs{0};
    uint32_t m_uHeartbeatTimeoutMs{0};
};

}
}
#endif // __CPPX_NETWORK_CONNECTION_IMPL_H__
