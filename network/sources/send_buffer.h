#ifndef __CPPX_NETWORK_SEND_BUFFER_H__
#define __CPPX_NETWORK_SEND_BUFFER_H__

#include <channel/channel.h>
#include <thread/spin_lock.h>
#include <engine.h>

namespace cppx
{
namespace network
{

class CSendBuffer
{
public:
    CSendBuffer() = default;
    ~CSendBuffer()
    {
        if (m_pSpinLockSend != nullptr)
        {
            base::SpinLock::Destroy(m_pSpinLockSend);
            m_pSpinLockSend = nullptr;
        }
        if (m_pChannelSend != nullptr)
        {
            base::channel::SPSCFixedBoundedChannel::Destroy(m_pChannelSend);
            m_pChannelSend = nullptr;
        }
        if (m_pChannelPrioritySend != nullptr)
        {
            base::channel::SPSCFixedBoundedChannel::Destroy(m_pChannelPrioritySend);
            m_pChannelPrioritySend = nullptr;
        }
    }

    int32_t Init()
    {
        m_pSpinLockSend = base::SpinLock::Create();
        if (m_pSpinLockSend == nullptr)
        {
            return ErrorCode::kOutOfMemory;
        }
    
        base::channel::ChannelConfig stConfig;
        stConfig.uElementSize = sizeof(IMessage *);
        stConfig.uMaxElementCount = 1024;
        m_pChannelSend = base::channel::SPSCFixedBoundedChannel::Create(&stConfig);
        m_pChannelPrioritySend = base::channel::SPSCFixedBoundedChannel::Create(&stConfig);
        if (m_pChannelSend == nullptr || m_pChannelPrioritySend == nullptr)
        {
            return ErrorCode::kOutOfMemory;
        }

        return ErrorCode::kSuccess;
    }

    int32_t Send(IMessage *pMessage, bool bPriority = false)
    {
        auto channel = bPriority ? m_pChannelPrioritySend : m_pChannelSend;
        base::SpinLockGuard guard(m_pSpinLockSend);
        auto pData = channel->New();
        if (likely(pData != nullptr))
        {
            *reinterpret_cast<IMessage **>(pData) = pMessage;
            channel->Post(pData);
            return ErrorCode::kSuccess;
        }

        return ErrorCode::kOutOfMemory;
    }

private:
    base::SpinLock *m_pSpinLockSend{nullptr};
    base::channel::SPSCFixedBoundedChannel *m_pChannelSend{nullptr};
    base::channel::SPSCFixedBoundedChannel *m_pChannelPrioritySend{nullptr};
};

}
}
#endif // __CPPX_NETWORK_SEND_BUFFER_H__
