#ifndef __CPPX_NETWORK_DISPATCHER_H__
#define __CPPX_NETWORK_DISPATCHER_H__

#include <functional>
#include <mutex>
#include <queue>
#include <thread/thread_manager.h>
#include <memory/allocator_ex.h>
#include <engine.h>

namespace cppx
{
namespace network
{


using CallbackFunc = std::function<void(bool bResult)>;

enum class TaskType
{
    kAddAcceptor = 1,  // 添加监听器
    kRemoveAcceptor,  // 移除监听器
    kConnect,  // 创建连接
    kDisconnect,  // 销毁连接
    kAddConnection,  // 添加连接到调度器
    kRemoveConnection,  // 从调度器移除连接
};

struct Task
{
    TaskType eTaskType;  // 任务类型
    CallbackFunc funcCallback;  // 回调函数
    union {
        int32_t iFd;  // 文件描述符
    };
};



class CTaskQueue
{
public:
    CTaskQueue();
    ~CTaskQueue();

    bool IsRunning() const
    {
        return m_bRunning;
    }

    void Start()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_bRunning = true;
    }

    void Stop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_bRunning = false;
        
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

    int32_t PostTask(Task &pTask)
    {
        try
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            if (!m_bRunning)
            {
                return -1;
            }
            m_queueTasks.push(pTask);
        }
        catch(const std::exception& e)
        {
            return -1;
        }
        return 0;
    }

    int32_t GetTask(Task &pTask)
    {
        try
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_bRunning)
            {
                return -1;
            }
            if (m_queueTasks.empty())
            {
                return -1;
            }
            pTask = std::move(m_queueTasks.front());
            m_queueTasks.pop();
        }
        catch(const std::exception& e)
        {
            return -1;
        }
        return 0;
    }

private:
    volatile bool m_bRunning{true};
    std::queue<Task> m_queueTasks;
    std::mutex m_mutex;
};

class IDispatcher
{
protected:
    virtual ~IDispatcher() = default;

public:
    virtual int32_t Start() = 0;
    virtual void Stop() = 0;
    virtual int32_t Post(Task &task) = 0;

    virtual bool IsRunning() const = 0;

protected:
    bool m_bRunning{false};
    NetworkLogger *m_pLogger{nullptr};
    base::memory::IAllocatorEx *m_pAllocatorEx{nullptr};
    base::IThreadManager *m_pThreadManager{nullptr};
    base::IThread *m_pThread{nullptr};
    CTaskQueue m_TaskQueue;
};

}
}
#endif // __CPPX_NETWORK_DISPATCHER_H__
