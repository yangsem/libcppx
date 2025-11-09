#include "channel_impl.h"
#include "memory/allocator_ex.h"
#include "utilities/error_code.h"

namespace cppx
{
namespace base
{
namespace channel
{

IChannel *IChannel::Create(const ChannelConfig *pConfig) noexcept
{
    auto pChannel = IAllocatorEx::GetInstance()->New<CChannelImpl>();
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

void IChannel::Destroy(IChannel *pChannel) noexcept
{
    auto pChannelImpl = dynamic_cast<CChannelImpl *>(pChannel);
    if (pChannelImpl != nullptr)
    {
        pChannelImpl->Exit();
        IAllocatorEx::GetInstance()->Delete(pChannelImpl);
    }
}

Entry *IChannel::NewEntry() noexcept
{
    return nullptr;
}

Entry *IChannel::NewEntry(uint32_t uElementSize) noexcept
{
    (void)uElementSize;
    return nullptr;
}

void IChannel::PostEntry(Entry *pEntry) noexcept
{
    (void)pEntry;
}

Entry *IChannel::GetEntry() noexcept
{
    return nullptr;
}

void IChannel::DeleteEntry(Entry *pEntry) noexcept
{
    (void)pEntry;
}

bool IChannel::IsEmpty() const noexcept
{
    return false;
}

uint32_t IChannel::GetSize() const noexcept
{
    return 0;
}

int32_t IChannel::GetStats(QueueStats &stStats) const noexcept
{
    memset(&stStats, 0, sizeof(stStats));
    return 0;
}

CChannelImpl::~CChannelImpl() noexcept
{
    Exit();
}

int32_t CChannelImpl::Init(const ChannelConfig *pConfig) noexcept
{
    if (pConfig == nullptr)
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    return ErrorCode::kSuccess;
}

void CChannelImpl::Exit() noexcept
{
}

}
}
}
