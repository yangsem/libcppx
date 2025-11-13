#ifndef __CPPX_SPIN_LOCK_H__
#define __CPPX_SPIN_LOCK_H__

#include "utilities/common.h"

namespace cppx
{
namespace base
{

class SpinLock
{
public:
    SpinLock() = default;
    SpinLock(const SpinLock &) = delete;
    SpinLock &operator=(const SpinLock &) = delete;
    SpinLock(SpinLock &&) = delete;
    SpinLock &operator=(SpinLock &&) = delete;
    ~SpinLock() = default;

    static SpinLock *Create();
    static void Destroy(SpinLock *pSpinLock);

    void Lock();
    void Unlock();
};

class SpinLockGuard
{
public:
    SpinLockGuard(SpinLock *pSpinLock) : m_pSpinLock(pSpinLock)
    {
        if (likely(m_pSpinLock != nullptr))
        {
            m_pSpinLock->Lock();
        }
    }

    ~SpinLockGuard()
    {
        if (likely(m_pSpinLock != nullptr))
        {
            m_pSpinLock->Unlock();
        }
    }

private:
    SpinLock *m_pSpinLock {nullptr};
};

}
}
#endif // __CPPX_SPIN_LOCK_H__
