#ifndef __CPPX_CHANNEL_IMPL_H__
#define __CPPX_CHANNEL_IMPL_H__

#include <channel/channel.h>
#include <cstdint>

namespace cppx
{
namespace base
{
namespace channel
{

constexpr uint8_t kMagic = 0x7F7F; // 魔数

enum EntryFlag : uint16_t
{
    kPlacehold = 1 << 0,
};

struct Entry
{
    uint16_t uMagic;  // 魔数
    uint16_t uFlags;  // 标志位
    uint32_t uLength; // 用户数据长度

    void *GetData()
    {
        return reinterpret_cast<uint8_t *>(this) + sizeof(Entry);
    }

    uint32_t GetDataLength() const { return uLength - sizeof(Entry); }

    uint32_t GetTotalLength() const { return uLength; }

    static inline uint32_t CalSize(uint32_t uSize)
    {
        return sizeof(Entry) + ALIGN8(uSize);
    }

    static inline Entry *GetEntry(void *pData)
    {
        return reinterpret_cast<Entry *>(reinterpret_cast<uint8_t *>(pData) - sizeof(Entry));
    }
};

struct ChannelStats
{
    uint64_t uCount{0};   // 操作次数
    uint64_t uFailed{0};  // 失败次数
    uint64_t uCount2{0};  // uCount成对操作次数
    uint64_t uFailed2{0}; // 失败次数

    void Reset() noexcept {
        uCount = 0;
        uFailed = 0;
        uCount2 = 0;
        uFailed2 = 0;
    }
};

inline uint64_t Up2PowerOf2(uint64_t uValue) noexcept
{
    if (uValue == 0)
    {
        return 0;
    }
    uValue--;
    uValue |= (uValue >> 1);
    uValue |= (uValue >> 2);
    uValue |= (uValue >> 4);
    uValue |= (uValue >> 8);
    uValue |= (uValue >> 16);
    uValue |= (uValue >> 32);
    return uValue + 1;
}

inline uint64_t GetIndex(uint64_t uIndex, uint64_t uSize) noexcept
{
    return uIndex & (uSize - uint64_t(1));
}

template class EXPORT IChannel<ChannelType::kSPSC, ElementType::kFixedSize, LengthType::kUnbounded>;
template class EXPORT IChannel<ChannelType::kSPSC, ElementType::kVariableSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kSPSC, ElementType::kVariableSize, LengthType::kUnbounded>;

template class EXPORT IChannel<ChannelType::kSPMC, ElementType::kFixedSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kSPMC, ElementType::kFixedSize, LengthType::kUnbounded>;
template class EXPORT IChannel<ChannelType::kSPMC, ElementType::kVariableSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kSPMC, ElementType::kVariableSize, LengthType::kUnbounded>;

template class EXPORT IChannel<ChannelType::kMPSC, ElementType::kFixedSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kMPSC, ElementType::kFixedSize, LengthType::kUnbounded>;
template class EXPORT IChannel<ChannelType::kMPSC, ElementType::kVariableSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kMPSC, ElementType::kVariableSize, LengthType::kUnbounded>;

template class EXPORT IChannel<ChannelType::kMPMC, ElementType::kFixedSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kMPMC, ElementType::kFixedSize, LengthType::kUnbounded>;
template class EXPORT IChannel<ChannelType::kMPMC, ElementType::kVariableSize, LengthType::kBounded>;
template class EXPORT IChannel<ChannelType::kMPMC, ElementType::kVariableSize, LengthType::kUnbounded>;

}
}
}

#endif // __CPPX_CHANNEL_IMPL_H__
