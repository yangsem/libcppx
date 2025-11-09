#include "channel_impl.h"
#include "memory/allocator_ex.h"
#include "utilities/error_code.h"

namespace cppx
{
namespace base
{
namespace channel
{

template<ChannelType eChannelType>
IChannel<eChannelType> *IChannel<eChannelType>::Create(const ChannelConfig *pConfig) noexcept
{
    auto pChannel = IAllocatorEx::GetInstance()->New<CChannelImpl<eChannelType>>();
    if (pChannel == nullptr)
    {
        SetLastError(ErrorCode::kOutOfMemory);
        return nullptr;
    }

    auto iErrorNo = pChannel->Init(pConfig);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        IAllocatorEx::GetInstance()->Delete(pChannel);
        SetLastError((ErrorCode)iErrorNo);
        return nullptr;
    }

    return pChannel;
}

template<ChannelType eChannelType>
void IChannel<eChannelType>::Destroy(IChannel<eChannelType> *pChannel) noexcept
{
    auto pChannelImpl = dynamic_cast<CChannelImpl<eChannelType> *>(pChannel);
    if (pChannelImpl != nullptr)
    {
        pChannelImpl->Exit();
        IAllocatorEx::GetInstance()->Delete(pChannelImpl);
    }
}

template<ChannelType eChannelType>
Entry *IChannel<eChannelType>::NewEntry() noexcept
{
    return nullptr;
}

template<ChannelType eChannelType>
Entry *IChannel<eChannelType>::NewEntry(uint32_t uElementSize) noexcept
{
    (void)uElementSize;
    return nullptr;
}

template<ChannelType eChannelType>
void IChannel<eChannelType>::PostEntry(Entry *pEntry) noexcept
{
    (void)pEntry;
}

template<ChannelType eChannelType>
Entry *IChannel<eChannelType>::GetEntry() noexcept
{
    return nullptr;
}

template<ChannelType eChannelType>
void IChannel<eChannelType>::DeleteEntry(Entry *pEntry) noexcept
{
    (void)pEntry;
}

template<ChannelType eChannelType>
bool IChannel<eChannelType>::IsEmpty() const noexcept
{
    return false;
}

template<ChannelType eChannelType>
uint32_t IChannel<eChannelType>::GetSize() const noexcept
{
    return 0;
}

template<ChannelType eChannelType>
int32_t IChannel<eChannelType>::GetStats(QueueStats *pStats) const noexcept
{
    if (pStats != nullptr)
    {
        memset(pStats, 0, sizeof(QueueStats));
    }
    return ErrorCode::kSuccess;
}

template<ChannelType eChannelType>
CChannelImpl<eChannelType>::~CChannelImpl<eChannelType>() noexcept
{
    Exit();
}

template<ChannelType eChannelType>
int32_t CChannelImpl<eChannelType>::Init(const ChannelConfig *pConfig) noexcept
{
    if (pConfig == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    return ErrorCode::kSuccess;
}

template<ChannelType eChannelType>
void CChannelImpl<eChannelType>::Exit() noexcept
{
}

template class EXPORT IChannel<ChannelType::kSPSC>;
template class EXPORT IChannel<ChannelType::kSPMC>;
template class EXPORT IChannel<ChannelType::kMPSC>;
template class EXPORT IChannel<ChannelType::kMPMC>;

}
}
}
