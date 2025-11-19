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
    CIODispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CIODispatcher();

    int32_t Init();
    void Exit();

};

}
}
#endif // __CPPX_NETWORK_IO_DISPATCHER_H__
