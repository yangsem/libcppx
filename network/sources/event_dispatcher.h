#ifndef __CPPX_NETWORK_EVENT_DISPATCHER_H__
#define __CPPX_NETWORK_EVENT_DISPATCHER_H__

#include "dispatcher.h"
#include <vector>
#include <string>
#include <unordered_set>
#include "epoll_impl.h"

namespace cppx
{
namespace network
{

class CEventDispatcher final : public IDispatcher
{
public:
    CEventDispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CEventDispatcher();

    int32_t Init(const char *pEngineName);
    void Exit();

private:
    static bool RunWrapper(void *pArg);
    void Run();

    void ProcessEvent(const struct epoll_event &epollEvent);
    void ProcessTask(const Task &task);

    template<TaskType eTaskType>
    bool ProcessTask(const Task &task);

private:
    CEpollImpl m_EpollImpl;
    std::vector<struct epoll_event> m_vecEpollEvents;

    std::unordered_set<void *> m_setAcceptor;
    std::unordered_set<void *> m_setConnection;

    std::string m_strEngineName;
};

}
}
#endif // __CPPX_NETWORK_EVENT_DISPATCHER_H__
