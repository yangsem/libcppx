#ifndef __CPPX_NETWORK_EPOLL_IMPL_H__
#define __CPPX_NETWORK_EPOLL_IMPL_H__

#include <sys/epoll.h>
#include <unistd.h>

namespace cppx
{
namespace network
{

class CEpollImpl
{
public:
    CEpollImpl() = default;
    ~CEpollImpl()
    {
        Exit();
    }

    int32_t Init()
    {
        m_iEpollFd = epoll_create1(EPOLL_CLOEXEC);
        if (m_iEpollFd == -1)
        {
            return -1;
        }
        return 0;
    }

    void Exit()
    {
        if (m_iEpollFd != -1)
        {
            close(m_iEpollFd);
            m_iEpollFd = -1;
        }
    }

    int32_t Add(int32_t iFd, void *pCtx, uint32_t uEvents)
    {
        struct epoll_event epollEvent;
        epollEvent.events = uEvents;
        epollEvent.data.ptr = pCtx;
        if (epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, iFd, &epollEvent) == -1)
        {
            return -1;
        }
        return 0;
    }

    int32_t Mod(int32_t iFd, void *pCtx, uint32_t uEvents)
    {
        struct epoll_event epollEvent;
        epollEvent.events = uEvents;
        epollEvent.data.ptr = pCtx;
        if (epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, iFd, &epollEvent) == -1)
        {
            return -1;
        }
        return 0;
    }

    int32_t Del(int32_t iFd, void *pCtx)
    {
        struct epoll_event epollEvent;
        epollEvent.events = 0;
        epollEvent.data.ptr = pCtx;
        if (epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, iFd, &epollEvent) == -1)
        {
            return -1;
        }
        return 0;
    }

    int32_t Wait(struct epoll_event *pEpollEvents, int32_t iMaxEvents, int32_t iTimeoutMs)
    {
        return epoll_wait(m_iEpollFd, pEpollEvents, iMaxEvents, iTimeoutMs);
    }

private:
    int32_t m_iEpollFd{-1};
};

}
}
#endif // __CPPX_NETWORK_EPOLL_IMPL_H__
