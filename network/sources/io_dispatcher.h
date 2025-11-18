#ifndef __CPPX_NETWORK_IO_DISPATCHER_H__
#define __CPPX_NETWORK_IO_DISPATCHER_H__

#include "dispatcher.h"

namespace cppx
{
namespace network
{


class CIODispatcher final : public IDispatcher
{
public:
    CIODispatcher() = default;
    CIODispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CIODispatcher() override;

    int32_t Init();
    void Exit();

    int32_t Start() override;
    void Stop() override;
    int32_t Post(Task &task) override;
    bool IsRunning() const override;
};

}
}
#endif // __CPPX_NETWORK_IO_DISPATCHER_H__
