#ifndef __CPPX_THREAD_IMPL_H__
#define __CPPX_THREAD_IMPL_H__

#include <thread>
#include <string>
#include <thread/thread.h>

namespace cppx
{
namespace base
{

class CThreadImpl final : public IThread
{
public:
    CThreadImpl(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadArg);
    ~CThreadImpl() override;

    CThreadImpl(const CThreadImpl &) = delete;
    CThreadImpl &operator=(const CThreadImpl &) = delete;
    CThreadImpl(CThreadImpl &&) = delete;
    CThreadImpl &operator=(CThreadImpl &&) = delete;

    int32_t Bind(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadArg) override;

    int32_t BindCpu(int32_t iCpuNo) override;
    int32_t BindNode(int32_t iNodeNo) override;

    int32_t Start() override;
    void Stop() override;
    int32_t Pause() override;
    int32_t Resume() override;

    IThread::ThreadState GetThreadState() const override;
    int32_t GetThreadId() const override;
    uint64_t GetLastRunTimeNs() const override;
private:
    void Run();

private:
    std::thread m_thThread;

    volatile bool m_bRunning {false};
    volatile IThread::ThreadState m_eSetState {IThread::ThreadState::kCreated}; // 设置的线程状态
    uint64_t m_uLastLoopTimeNs {0};

    IThread::ThreadFunc m_pThreadFunc {nullptr};
    void *m_pThreadArg {nullptr};

    IThread::ThreadState m_eCurrState {IThread::ThreadState::kCreated}; // 当前线程状态
    int32_t m_iThreadId {-1};

    std::string m_strThreadName;
    int32_t m_iCpuNo {-1};
    int32_t m_iNodeNo {-1};
};

}
}
#endif // __CPPX_THREAD_IMPL_H__
