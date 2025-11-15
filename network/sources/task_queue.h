#ifndef __CPPX_NETWORK_TASK_QUEUE_H__
#define __CPPX_NETWORK_TASK_QUEUE_H__

#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace cppx
{
namespace network
{

using CallbackFunc = std::function<void(bool bResult)>;

enum class TaskType
{
    kAddAcceptor = 1,
    kRemoveAcceptor,
    kConnect,
    kDisconnect,
    kAddConnection,
    kRemoveConnection,
};

struct Task
{
    TaskType eTaskType;
    CallbackFunc funcCallback;
    union {
        int32_t iFd;
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

}
}
#endif // __CPPX_NETWORK_TASK_QUEUE_H__
