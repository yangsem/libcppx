#include "allocator_impl.h"
#include "memory/allocator.h"
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <cstdlib>
#include <mutex>

namespace cppx
{
namespace base
{

IAllocator *IAllocator::GetInstance() noexcept
{
    static std::mutex s_mutex;
    static CAllocatorImpl *s_pAllocator = nullptr;
    if (likely(s_pAllocator != nullptr))
    {
        return s_pAllocator;
    }

    std::lock_guard<std::mutex> lock(s_mutex);

    if (likely(s_pAllocator != nullptr))
    {
        return s_pAllocator;
    }

    s_pAllocator = NEW CAllocatorImpl();
    if (s_pAllocator == nullptr)
    {
        SetLastError(ErrorCode::kOutOfMemory);
        return nullptr;
    }

    auto iErrorNo = s_pAllocator->Init(nullptr);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        delete s_pAllocator;
        SetLastError((ErrorCode)iErrorNo);
        return nullptr;
    }

    return s_pAllocator;
}

IAllocator *IAllocator::Create() noexcept
{
    return NEW CAllocatorImpl();
}

void IAllocator::Destroy(IAllocator *pAllocator) noexcept
{
    if (pAllocator != nullptr)
    {
        delete pAllocator;
    }
}

int32_t CAllocatorImpl::Init(IJson *pConfig) noexcept
{
    return ErrorCode::kSuccess;
}

void CAllocatorImpl::Exit() noexcept
{
}

void *CAllocatorImpl::New(uint64_t uSize) noexcept
{
    return std::malloc(uSize);
}

void CAllocatorImpl::Delete(void *pMem) noexcept
{
    if (pMem != nullptr)
    {
        std::free(pMem);
    }
}

int32_t CAllocatorImpl::GetStats(IJson *pJson) const noexcept
{
    if (pJson != nullptr)
    {
        pJson->Clear();
    }
    return ErrorCode::kSuccess;
}

}
}
