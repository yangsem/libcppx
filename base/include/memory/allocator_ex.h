#ifndef __CPPX_ALLOCATOR_EX_H__
#define __CPPX_ALLOCATOR_EX_H__

#include "utilities/common.h"
#include <memory/allocator.h>
#include <utility>

namespace cppx
{
namespace base
{

class AllocatorEx : public IAllocator
{
protected:
    virtual ~AllocatorEx() noexcept = default;

public:
    static AllocatorEx *GetInstance() noexcept
    {
        return reinterpret_cast<AllocatorEx *>(IAllocator::GetInstance());
    }

    static AllocatorEx *Create() noexcept
    {
        return reinterpret_cast<AllocatorEx *>(IAllocator::Create());
    }

    static void Destroy(AllocatorEx *pAllocator) noexcept
    {
        IAllocator::Destroy(reinterpret_cast<IAllocator *>(pAllocator));
    }
    
    template<typename T, typename... Args>
    T *New(Args&&... args) noexcept
    {
        auto pMem = IAllocator::New(sizeof(T));
        if (likely(pMem != nullptr))
        {
            try
            {
                return new(pMem) T(std::forward<Args>(args)...);
            }
            catch (std::exception &e)
            {
                IAllocator::Delete(pMem);
                return nullptr;
            }
        }
        return nullptr;
    }

    template<typename T>
    void Delete(T *pMem) noexcept
    {
        if (likely(pMem != nullptr))
        {
            pMem->~T();
            IAllocator::Delete(pMem);
        }
    }
};

}
}
#endif // __CPPX_ALLOCATOR_EX_H__
