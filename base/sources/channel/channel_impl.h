#ifndef __CPPX_CHANNEL_IMPL_H__
#define __CPPX_CHANNEL_IMPL_H__

#include <channel/channel.h>

namespace cppx
{
namespace base
{
namespace channel
{

class CChannelImpl final : public IChannel
{
public:
    CChannelImpl() noexcept = default;
    CChannelImpl(const CChannelImpl &) = delete;
    CChannelImpl &operator=(const CChannelImpl &) = delete;
    CChannelImpl(CChannelImpl &&) = delete;
    CChannelImpl &operator=(CChannelImpl &&) = delete;

    ~CChannelImpl() noexcept;

    int32_t Init(const ChannelConfig *pConfig) noexcept;
    void Exit() noexcept;

    Entry *NewEntry() noexcept;
    Entry *NewEntry(uint32_t uElementSize) noexcept;
    void PostEntry(Entry *pEntry) noexcept;
    Entry *GetEntry() noexcept;
    void DeleteEntry(Entry *pEntry) noexcept;
    bool IsEmpty() const noexcept;
    uint32_t GetSize() const noexcept;
    int32_t GetStats(QueueStats &stStats) const noexcept;

private:

};

}
}
}

#endif // __CPPX_CHANNEL_IMPL_H__
