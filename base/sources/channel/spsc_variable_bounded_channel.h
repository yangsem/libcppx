#ifndef __CPPX_SPSC_VARIABLE_BOUNDED_CHANNEL_H__
#define __CPPX_SPSC_VARIABLE_BOUNDED_CHANNEL_H__

#include "channel_impl.h"
#include <cstdint>

namespace cppx
{
namespace base
{
namespace channel
{

class CSPSCVariableBoundedChannel
{
public:
    CSPSCVariableBoundedChannel() noexcept = default;
    CSPSCVariableBoundedChannel(const CSPSCVariableBoundedChannel &) = delete;
    CSPSCVariableBoundedChannel &operator=(const CSPSCVariableBoundedChannel &) = delete;
    CSPSCVariableBoundedChannel(CSPSCVariableBoundedChannel &&) = delete;
    CSPSCVariableBoundedChannel &operator=(CSPSCVariableBoundedChannel &&) = delete;

    ~CSPSCVariableBoundedChannel() noexcept;

    int32_t Init(uint64_t uMaxMemorySizeKB) noexcept;

    Entry *New() noexcept;
    Entry *New(uint32_t uSize) noexcept;
    void Post(Entry *pEntry) noexcept;

    Entry *Get() noexcept;
    void Delete(Entry *pEntry) noexcept;

    bool IsEmpty() const noexcept;
    uint32_t GetSize() const noexcept;

    int32_t GetStats(IJson *pStats) const noexcept;

private:
    void *NewEntry(uint32_t uNewSize) noexcept;
    void *GetEntry() noexcept;

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

#endif // __CPPX_SPSC_VARIABLE_BOUNDED_CHANNEL_H__
