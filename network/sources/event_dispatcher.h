#ifndef __CPPX_NETWORK_EVENT_DISPATCHER_H__
#define __CPPX_NETWORK_EVENT_DISPATCHER_H__

#include "dispatcher.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <engine.h>

namespace cppx
{
namespace network
{

class CEventDispatcher final : public IDispatcher
{
public:
    CEventDispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CEventDispatcher();

    int32_t Init(IEngine *pEngine);

    int32_t DoTask(Task &task) override;

private:
    static bool RunWrapper(void *pArg);
    void Run();

    void ProcessEvent(const struct epoll_event &epollEvent);
    void ProcessAcceptorEvent(const struct epoll_event &epollEvent);
    void ProcessConnectionEvent(const struct epoll_event &epollEvent);

    bool ProcessTask(const Task &task);
    template<TaskType eTaskType>
    bool ProcessTask(const Task &task);
    static void DetachCallback(IConnection *pConnection, void *pCtx);

private:

    std::unordered_set<void *> m_setAcceptor;
    std::unordered_set<void *> m_setConnection;

    IEngine *m_pEngine{nullptr};
};

}
}
#endif // __CPPX_NETWORK_EVENT_DISPATCHER_H__
