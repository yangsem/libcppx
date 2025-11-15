#ifndef __CPPX_MPSC_VARIABLE_BOUNDED_CHANNEL_H__
#define __CPPX_MPSC_VARIABLE_BOUNDED_CHANNEL_H__

#include "channel_common.h"

namespace cppx
{
namespace base
{
namespace channel
{

class CMPSCVariableBoundedChannel
{
public:
    CMPSCVariableBoundedChannel() = default;
    CMPSCVariableBoundedChannel(const CMPSCVariableBoundedChannel &) = delete;
    CMPSCVariableBoundedChannel &operator=(const CMPSCVariableBoundedChannel &) = delete;
    CMPSCVariableBoundedChannel(CMPSCVariableBoundedChannel &&) = delete;
    CMPSCVariableBoundedChannel &operator=(CMPSCVariableBoundedChannel &&) = delete;

    ~CMPSCVariableBoundedChannel();

    int32_t Init(uint64_t uMaxMemorySizeKB);

    Entry *New();
    Entry *New(uint32_t uSize);
    void Post(Entry *pEntry);

    Entry *Get();
    void Delete(Entry *pEntry);

    bool IsEmpty() const;
    uint32_t GetSize() const;

    int32_t GetStats(IJson *pStats) const;

private:
    void *NewEntry(uint32_t uNewSize);
    void *GetEntry();

private:
    ALIGN_AS_CACHELINE uint8_t *m_pDatap{nullptr};
    uint64_t m_uSizep{0};
    uint64_t m_uTail{0};
    uint64_t m_uHeadRef{0};
    ChannelStats m_Statsp;

    ALIGN_AS_CACHELINE uint8_t *m_pDatac{nullptr};
    uint64_t m_uSizec{0};
    uint64_t m_uHead{0};
    uint64_t m_uTailRef{0};
    ChannelStats m_Statsc;
};
}
}
}

#endif // __CPPX_MPSC_VARIABLE_BOUNDED_CHANNEL_H__
