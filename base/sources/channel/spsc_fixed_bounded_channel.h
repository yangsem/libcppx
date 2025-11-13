#ifndef __CPPX_SPSC_FIXED_BOUNDED_CHANNEL_H__
#define __CPPX_SPSC_FIXED_BOUNDED_CHANNEL_H__

#include "channel_impl.h"

namespace cppx
{
namespace base
{
namespace channel
{

class CSPSCFixedBoundedChannel
{
public:
    CSPSCFixedBoundedChannel() = default;
    CSPSCFixedBoundedChannel(const CSPSCFixedBoundedChannel &) = delete;
    CSPSCFixedBoundedChannel &operator=(const CSPSCFixedBoundedChannel &) = delete;
    CSPSCFixedBoundedChannel(CSPSCFixedBoundedChannel &&) = delete;
    CSPSCFixedBoundedChannel &operator=(CSPSCFixedBoundedChannel &&) = delete;

    ~CSPSCFixedBoundedChannel();

    int32_t Init(uint64_t uElemSize, uint64_t uSize);

    void *New();
    void *New(uint32_t uSize);
    void Post(void *pData);

    void *Get();
    void Delete(void *pData);

    bool IsEmpty() const;
    uint32_t GetSize() const;

    int32_t GetStats(IJson *pStats) const;

private:
    // producer
    ALIGN_AS_CACHELINE uint8_t *m_pDatap{nullptr};
    uint64_t m_uElemSizep{0};
    uint64_t m_uSizep{0};
    uint64_t m_uTail{0};
    uint64_t m_uHeadRef{0};
    ChannelStats m_Statsp;

    // consumer
    ALIGN_AS_CACHELINE uint8_t *m_pDatac{nullptr};
    uint64_t m_uElemSizec{0};
    uint64_t m_uSizec{0};
    uint64_t m_uHead{0};
    uint64_t m_uTailRef{0};
    ChannelStats m_Statsc;
};

}
}
}

#endif // __CPPX_SPSC_FIXED_BOUNDED_CHANNEL_H__
