#ifndef __CPPX_SPIN_LOCK_IMPL_H__
#define __CPPX_SPIN_LOCK_IMPL_H__

#include <thread/spin_lock.h>
#include <atomic>
#include <thread>

namespace cppx
{
namespace base
{

class CSpinLockImpl final : public SpinLock
{
public:
    CSpinLockImpl() = default;
    ~CSpinLockImpl() = default;

    CSpinLockImpl(const CSpinLockImpl &) = delete;
    CSpinLockImpl &operator=(const CSpinLockImpl &) = delete;
    CSpinLockImpl(CSpinLockImpl &&) = delete;
    CSpinLockImpl &operator=(CSpinLockImpl &&) = delete;

    inline void Lock()
    {
        while (m_bLocked.test_and_set())
        {
            std::this_thread::yield();
        }
    }

    inline void Unlock()
    {
        m_bLocked.clear();
    }

private:
    std::atomic_flag m_bLocked {ATOMIC_FLAG_INIT};
};

}
}
#endif // __CPPX_SPIN_LOCK_IMPL_H__
