#ifndef __CPPX_NETWORK_EVENT_DISPATCHER_H__
#define __CPPX_NETWORK_EVENT_DISPATCHER_H__

#include "dispatcher.h"

namespace cppx
{
namespace network
{

class CEventDispatcher final : public IDispatcher
{
public:
    CEventDispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CEventDispatcher() override;

    int32_t Init();
    void Exit();
    int32_t Start() override;
    void Stop() override;
    int32_t Post(Task &task) override;
    bool IsRunning() const override;

private:
};

}
}
#endif // __CPPX_NETWORK_EVENT_DISPATCHER_H__
