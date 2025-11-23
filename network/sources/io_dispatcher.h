#ifndef __CPPX_NETWORK_IO_DISPATCHER_H__
#define __CPPX_NETWORK_IO_DISPATCHER_H__

#include "connection.h"
#include "dispatcher.h"
#include <cstdint>

namespace cppx
{
namespace network
{


class CIODispatcher final : public IDispatcher
{
public:
    CIODispatcher(NetworkLogger *pLogger, base::memory::IAllocatorEx *pAllocatorEx);
    ~CIODispatcher();

    int32_t Init(IEngine *pEngine);

    int32_t DoTask(Task &task) override;

private:
    static bool RunWrapper(void *pArg);
    void Run();

    void ProcessRecvEvent(IConnection *pConnection, uint32_t uSize);
    void ProcessSendEvent(IConnection *pConnection, uint32_t uSize);

    bool ProcessTask(const Task &task);

private:
    IEngine *m_pEngine{nullptr};
    uint32_t m_uBatchSendRecvSize{0};
};

}
}
#endif // __CPPX_NETWORK_IO_DISPATCHER_H__
