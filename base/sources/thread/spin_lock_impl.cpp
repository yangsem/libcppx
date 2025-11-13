#include "spin_lock_impl.h"
#include <memory/allocator_ex.h>

namespace cppx
{
namespace base
{

SpinLock *SpinLock::Create() noexcept
{
    return IAllocatorEx::GetInstance()->New<CSpinLockImpl>();
}

void SpinLock::Destroy(SpinLock *pSpinLock) noexcept
{
    IAllocatorEx::GetInstance()->Delete(reinterpret_cast<CSpinLockImpl *>(pSpinLock));
}

void SpinLock::Lock() noexcept
{
    reinterpret_cast<CSpinLockImpl *>(this)->Lock();
}

void SpinLock::Unlock() noexcept
{
    reinterpret_cast<CSpinLockImpl *>(this)->Unlock();
}

}
}
