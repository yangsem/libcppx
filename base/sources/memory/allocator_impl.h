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
    CAllocatorImpl() = default;
    CAllocatorImpl(const CAllocatorImpl &) = delete;
    CAllocatorImpl &operator=(const CAllocatorImpl &) = delete;
    CAllocatorImpl(CAllocatorImpl &&) = delete;
    CAllocatorImpl &operator=(CAllocatorImpl &&) = delete;

    ~CAllocatorImpl() override = default;

    int32_t Init(const IJson *pConfig) override;
    void Exit() override;

    void *Malloc(uint64_t uSize) override;
    void Free(const void *pMem) override;

    int32_t GetStats(IJson *pJson) const override;

private:

};

}
}
#endif // __CPPX_ALLOCATOR_IMPL_H__
