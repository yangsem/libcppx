#ifndef __CPPX_NETWORK_DISPATCHER_H__
#define __CPPX_NETWORK_DISPATCHER_H__

#include <functional>
#include <mutex>
#include <queue>
#include <utilities/common.h>
#include <thread/thread_manager.h>
#include <memory/allocator_ex.h>
#include <engine.h>
#include "epoll_impl.h"

namespace cppx
{
namespace network
{


using CallbackFunc = std::function<void(bool bResult)>;

enum class TaskType 
{
    kAddAcceptor = 0, // 添加监听器到epoll
    kRemoveAcceptor,  // 从epoll移除监听器
    kAddConnection,   // 添加连接到epoll
    kRemoveConnection,// 从epoll移除连接
    kDoDisconnect,    // 断开连接
    kConnected,       // 连接成功
    kDisconnected,    // 连接断开
    kAddRecv, // 添加读事件到epoll
    kRemoveRecv, // 从epoll移除读事件
    kAddSend, // 添加写事件到epoll
    kRemoveSend, // 从epoll移除写事件
};

struct Task
{
    TaskType eTaskType;  // 任务类型
    CallbackFunc funcCallback;  // 回调函数
    union {
        void *pCtx;  // 上下文
    };
};



class CTaskQueue
{
public:
    CTaskQueue() = default;
    ~CTaskQueue() = default;

    void Clear()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_queueTasks.empty())
        {
            auto &task = m_queueTasks.front();
            if (task.funcCallback != nullptr)
            {
                task.funcCallback(false);
            }
            m_queueTasks.pop();
        }
    }

    int32_t PostTask(Task &task)
    {
        try
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_queueTasks.push(task);
        }
        catch(const std::exception& e)
        {
            return ErrorCode::kThrowException;
        }
        return ErrorCode::kSuccess;
    }

    int32_t GetTask(Task &task)
    {
        try
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_queueTasks.empty())
            {
                return ErrorCode::kInvalidState;
            }
            task = std::move(m_queueTasks.front());
            m_queueTasks.pop();
        }
        catch(const std::exception& e)
        {
            return ErrorCode::kThrowException;
        }
        return ErrorCode::kSuccess;
    }

    bool IsEmpty() const
    {
        return m_queueTasks.empty();
    }

private:
    std::queue<Task> m_queueTasks;
    std::mutex m_mutex;
};

class IDispatcher
{
public:
    IDispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx)
    : m_pLogger(pLogger), m_pAllocatorEx(pAllocatorEx)
    {
    }

    ~IDispatcher()
    {
        if (m_pThread != nullptr)
        {
            m_pThreadManager->DestroyThread(m_pThread);
            m_pThread = nullptr;
        }

        m_pThreadManager = nullptr;
        m_pLogger = nullptr;
        m_pAllocatorEx = nullptr;
        m_pThread = nullptr;
    }

    int32_t Init(uint32_t uEventSize)
    {
        m_pThreadManager = base::IThreadManager::GetInstance();
        if (m_pThreadManager == nullptr)
        {
            SetLastError(ErrorCode::kInvalidParam);
            return ErrorCode::kInvalidParam;
        }

        m_pThread = m_pThreadManager->CreateThread();
        if (m_pThread == nullptr)
        {
            SetLastError(ErrorCode::kInvalidParam);
            return ErrorCode::kInvalidParam;
        }

        if (m_EpollImpl.Init() != ErrorCode::kSuccess)
        {
            return ErrorCode::kInvalidCall;
        }

        try
        {
            m_vecEpollEvents.resize(uEventSize);
        }
        catch(const std::exception& e)
        {
            return ErrorCode::kThrowException;
        }

        return ErrorCode::kSuccess;
    }

    int32_t Start()
    {
        if (m_pThread == nullptr)
        {
            SetLastError(ErrorCode::kInvalidParam);
            return ErrorCode::kInvalidParam;
        }

        return m_pThread->Start();
    }

    void Stop()
    {
        if (m_pThread == nullptr)
        {
            return;
        }

        m_pThread->Stop();
        m_TaskQueue.Clear();
    }

    virtual int32_t DoTask(Task &task)
    {
        return ErrorCode::kInvalidCall;
    }
    
    int32_t PostTask(Task &task)
    {
        return m_TaskQueue.PostTask(task);
    }

    bool IsRunning() const
    {
        if (m_pThread != nullptr)
        {
            return m_pThread->GetThreadState() == base::IThread::ThreadState::kRunning
                   || m_pThread->GetThreadState() == base::IThread::ThreadState::kPaused;
        }
        return false;
    }
    
    bool IsEmpty() const
    {
        return m_TaskQueue.IsEmpty();
    }

protected:
    NetworkLogger *m_pLogger{nullptr};
    base::memory::IAllocatorEx *m_pAllocatorEx{nullptr};
    base::IThreadManager *m_pThreadManager{nullptr};
    base::IThread *m_pThread{nullptr};
    CTaskQueue m_TaskQueue;

    CEpollImpl m_EpollImpl;
    std::vector<struct epoll_event> m_vecEpollEvents;
};

}
}
#endif // __CPPX_NETWORK_DISPATCHER_H__
