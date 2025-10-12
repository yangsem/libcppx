#include "cppx_thread_impl.h"
#include <utilities/cppx_last_error.h>
#include <utilities/cppx_error_code.h>
#include <utilities/cppx_common.h>

namespace cppx
{
namespace base
{

IThread *IThread::Create(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadArg) noexcept
{
    if (pThreadName == nullptr)
    {
        pThreadName = "";
    }

    if (pThreadFunc == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "ThreadFunc is nullptr");
        return nullptr;
    }

    CThreadImpl *pThread = NEW CThreadImpl(pThreadName, pThreadFunc, pThreadArg);
    return pThread;
}

void IThread::Destroy(IThread *pThread) noexcept
{
    if (pThread != nullptr)
    {
        delete pThread;
    }
}

CThreadImpl::CThreadImpl(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadArg)
    : m_strThreadName(pThreadName)
    , m_pThreadFunc(pThreadFunc)
    , m_pThreadArg(pThreadArg)
{
}

CThreadImpl::~CThreadImpl()
{
    Stop();
}

int32_t CThreadImpl::Bind(const char *pThreadName, IThread::ThreadFunc pThreadFunc, void *pThreadArg) noexcept
{
    if (pThreadName == nullptr)
    {
        pThreadName = "";
    }

    if (pThreadFunc == nullptr)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "ThreadFunc is nullptr");
        return ErrorCode::kInvalidParam;
    }

    if (m_bRunning)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Thread %s is running", m_strThreadName.c_str());
        return ErrorCode::kInvalidCall;
    }

    m_strThreadName = pThreadName;
    m_pThreadFunc = pThreadFunc;
    m_pThreadArg = pThreadArg;
    return 0;
}

int32_t CThreadImpl::BindCpu(int32_t iCpuNo) noexcept
{
    if (m_bRunning)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Thread %s is running", m_strThreadName.c_str());
        return ErrorCode::kInvalidCall;
    }

    // 获取CPU数量，如果是负数或者大于CPU数量，返回错误
    auto iCpuCount = std::thread::hardware_concurrency();
    if (iCpuNo < 0 || iCpuNo >= (int32_t)iCpuCount)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "CPU number is out of range");
        return ErrorCode::kInvalidParam;
    }

    if (m_iNodeNo != -1)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Node number is already set");
        return ErrorCode::kInvalidParam;
    }

    m_iCpuNo = iCpuNo;
    return 0;
}

int32_t CThreadImpl::BindNode(int32_t iNodeNo) noexcept
{
    if (m_bRunning)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Thread %s is running", m_strThreadName.c_str());
        return ErrorCode::kInvalidCall;
    }

    if (iNodeNo < 0)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Node number is out of range");
        return ErrorCode::kInvalidParam;
    }

    if (m_iCpuNo != -1)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "CPU number is already set");
        return ErrorCode::kInvalidParam;
    }

    m_iNodeNo = iNodeNo;
    return 0;
}

int32_t CThreadImpl::Start() noexcept
{
    if (m_bRunning)
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Thread %s is running", m_strThreadName.c_str());
        return ErrorCode::kInvalidCall;
    }

    try
    {
        m_bRunning = true;
        m_thThread = std::thread(&CThreadImpl::Run, this);
    }
    catch(std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Thread %s start failed: %s", m_strThreadName.c_str(), e.what());
        return ErrorCode::kThrowException;
    }
    return 0;
}

void CThreadImpl::Stop() noexcept
{
    if (m_bRunning == false)
    {
        return;
    }

    m_bRunning = false;
    if (m_thThread.joinable())
    {
        m_thThread.join();
    }
}

int32_t CThreadImpl::Pause() noexcept
{
    auto state = ACCESS_ONCE(m_eCurrState);
    if (state == IThread::ThreadState::kRunning)
    {
        m_eSetState = IThread::ThreadState::kPaused;
        while (ACCESS_ONCE(m_eCurrState) != IThread::ThreadState::kPaused)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    else
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Thread %s is not running, current state is %d", 
            m_strThreadName.c_str(), (int32_t)state);
        return ErrorCode::kInvalidCall;
    }
    return 0;
}

int32_t CThreadImpl::Resume() noexcept
{
    auto state = ACCESS_ONCE(m_eCurrState);
    if (state == IThread::ThreadState::kPaused)
    {
        m_eSetState = IThread::ThreadState::kRunning;
    }
    else
    {
        SET_LAST_ERROR(ErrorCode::kInvalidCall, "Thread %s is not paused, current state is %d", 
            m_strThreadName.c_str(), (int32_t)state);
        return ErrorCode::kInvalidCall;
    }
    return 0;
}

IThread::ThreadState CThreadImpl::GetThreadState() const noexcept
{
    return m_eCurrState;
}

int32_t CThreadImpl::GetThreadId() const noexcept
{
    return m_iThreadId;
}

uint64_t CThreadImpl::GetLastLoopTimeNs() const noexcept
{
    return m_uLastLoopTimeNs;
}

void CThreadImpl::Run()
{
    set_thread_name(m_strThreadName.c_str());
    if (m_iCpuNo != -1)
    {
        thread_bind_cpu(m_iCpuNo);
    }
    if (m_iNodeNo != -1)
    {
        // thread_bind_node(m_iNodeNo);
    }

    m_iThreadId = gettid();
    m_eSetState = IThread::ThreadState::kRunning;
    m_eCurrState = IThread::ThreadState::kRunning;

    while (likely(m_bRunning))
    {
        auto state = m_eSetState;
        clock_get_time_nano(m_uLastLoopTimeNs);
        if (likely(state == IThread::ThreadState::kRunning))
        {
            auto bRet = m_pThreadFunc(m_pThreadArg);
            if (unlikely(!bRet))
            {
                break;
            }
        }
        else
        {
            m_eCurrState = state;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    m_eCurrState = IThread::ThreadState::kStopped;
}

}
}
