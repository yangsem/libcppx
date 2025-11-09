#ifndef __CPPX_ALLOCATOR_IMPL_H__
#define __CPPX_ALLOCATOR_IMPL_H__

#include <memory/allocator.h>

namespace cppx
{
namespace base
{

class CAllocatorImpl final : public IAllocator
{
public:
    CAllocatorImpl() noexcept = default;
    CAllocatorImpl(const CAllocatorImpl &) = delete;
    CAllocatorImpl &operator=(const CAllocatorImpl &) = delete;
    CAllocatorImpl(CAllocatorImpl &&) = delete;
    CAllocatorImpl &operator=(CAllocatorImpl &&) = delete;

    ~CAllocatorImpl() noexcept override = default;

    int32_t Init(const IJson *pConfig) noexcept override;
    void Exit() noexcept override;

    void *Malloc(uint64_t uSize) noexcept override;
    void Free(const void *pMem) noexcept override;

    int32_t GetStats(IJson *pJson) const noexcept override;

private:

};

}
}
#endif // __CPPX_ALLOCATOR_IMPL_H__
