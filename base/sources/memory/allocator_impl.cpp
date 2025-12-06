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
namespace memory
{

IAllocator *IAllocator::GetInstance()
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

IAllocator *IAllocator::Create()
{
    return NEW CAllocatorImpl();
}

void IAllocator::Destroy(IAllocator *pAllocator)
{
    if (pAllocator != nullptr)
    {
        delete pAllocator;
    }
}

int32_t CAllocatorImpl::Init(const IJson *pConfig)
{
    // 允许pConfig为nullptr，使用默认配置
    (void)pConfig;
    return ErrorCode::kSuccess;
}

void CAllocatorImpl::Exit()
{
}

void *CAllocatorImpl::Malloc(uint64_t uSize)
{
    return std::malloc(uSize);
}

void CAllocatorImpl::Free(const void *pMem)
{
    if (pMem != nullptr)
    {
        std::free(const_cast<void *>(pMem));
    }
}

int32_t CAllocatorImpl::GetStats(IJson *pJson) const
{
    if (pJson != nullptr)
    {
        pJson->Clear();
    }
    return ErrorCode::kSuccess;
}

}
}
}
