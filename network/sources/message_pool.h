#ifndef __CPPX_NETWORK_MESSAGE_POOL_H__
#define __CPPX_NETWORK_MESSAGE_POOL_H__

#include "message_impl.h"
#include <memory/allocator_ex.h>

namespace cppx
{
namespace network
{

class CMessagePool
{
public:
    CMessagePool(base::memory::IAllocatorEx *pAllocatorEx);
    ~CMessagePool();

    int32_t Init(uint32_t uMaxMessageCount, uint32_t uMessageSize);
    void Exit();

    IMessage *NewMessage(uint32_t uLength)
    {
        return reinterpret_cast<IMessage *>(m_pAllocator->Malloc(uLength));
    }

    void DeleteMessage(IMessage *pMessage)
    {
        m_pAllocator->Free(pMessage);
    }

private:
    base::memory::IAllocator *m_pAllocator{nullptr};
};

}
}
#endif // __CPPX_NETWORK_MESSAGE_POOL_H__
