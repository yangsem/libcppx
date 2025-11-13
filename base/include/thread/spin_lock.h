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
    SpinLock() noexcept = default;
    SpinLock(const SpinLock &) = delete;
    SpinLock &operator=(const SpinLock &) = delete;
    SpinLock(SpinLock &&) = delete;
    SpinLock &operator=(SpinLock &&) = delete;
    ~SpinLock() noexcept = default;

    static SpinLock *Create() noexcept;
    static void Destroy(SpinLock *pSpinLock) noexcept;

    void Lock() noexcept;
    void Unlock() noexcept;
};

class SpinLockGuard
{
public:
    SpinLockGuard(SpinLock *pSpinLock) noexcept : m_pSpinLock(pSpinLock)
    {
        if (likely(m_pSpinLock != nullptr))
        {
            m_pSpinLock->Lock();
        }
    }

    ~SpinLockGuard() noexcept
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
