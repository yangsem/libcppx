#ifndef __CPPX_ALLOCATOR_EX_H__
#define __CPPX_ALLOCATOR_EX_H__

#include "utilities/common.h"
#include <memory/allocator.h>
#include <utility>

namespace cppx
{
namespace base
{
namespace memory
{

class IAllocatorEx : public IAllocator
{
protected:
    virtual ~IAllocatorEx() = default;

public:
    static IAllocatorEx *GetInstance()
    {
        return reinterpret_cast<IAllocatorEx *>(IAllocator::GetInstance());
    }

    static IAllocatorEx *Create()
    {
        return reinterpret_cast<IAllocatorEx *>(IAllocator::Create());
    }

    static void Destroy(IAllocatorEx *pAllocator)
    {
        IAllocator::Destroy(reinterpret_cast<IAllocator *>(pAllocator));
    }
    
    template<typename T, typename... Args>
    T *New(Args&&... args)
    {
        auto pMem = Malloc(sizeof(T));
        if (likely(pMem != nullptr))
        {
            try
            {
                return new(pMem) T(std::forward<Args>(args)...);
            }
            catch (std::exception &e)
            {
                Free(pMem);
                return nullptr;
            }
        }
        return nullptr;
    }

    template<typename T>
    void Delete(T *pMem)
    {
        if (likely(pMem != nullptr))
        {
            pMem->~T();
            Free(pMem);
        }
    }
};

}
}
}
#endif // __CPPX_ALLOCATOR_EX_H__
