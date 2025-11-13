#include "spin_lock_impl.h"
#include <memory/allocator_ex.h>

namespace cppx
{
namespace base
{

SpinLock *SpinLock::Create()
{
    return IAllocatorEx::GetInstance()->New<CSpinLockImpl>();
}

void SpinLock::Destroy(SpinLock *pSpinLock)
{
    IAllocatorEx::GetInstance()->Delete(reinterpret_cast<CSpinLockImpl *>(pSpinLock));
}

void SpinLock::Lock()
{
    reinterpret_cast<CSpinLockImpl *>(this)->Lock();
}

void SpinLock::Unlock()
{
    reinterpret_cast<CSpinLockImpl *>(this)->Unlock();
}

}
}
