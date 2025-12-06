#ifndef __CPPX_NETWORK_RECEIVE_BUFFER_H__
#define __CPPX_NETWORK_RECEIVE_BUFFER_H__

#include <algorithm>
#include <cstdint>
#include <sys/socket.h>
#include <memory/allocator.h>
#include <utilities/common.h>
#include <engine.h>

namespace cppx
{
namespace network
{

class CReceiveBuffer
{
public:
    CReceiveBuffer();
    ~CReceiveBuffer()
    {
        if (m_pData != nullptr)
        {
            m_pAllocator->Free(m_pData);
            m_pData = nullptr;
        }
        m_pAllocator = nullptr;
        m_uSize = 0;
        m_uTail = 0;
        m_uHead = 0;
    }

    int32_t Init(base::memory::IAllocator *pAllocator, uint32_t uSize)
    {
        if (pAllocator == nullptr)
        {
            return ErrorCode::kInvalidParam;
        }
        m_pAllocator = pAllocator;
        m_pData = reinterpret_cast<uint8_t *>(m_pAllocator->Malloc(uSize));
        if (m_pData == nullptr)
        {
            return ErrorCode::kOutOfMemory;
        }
        m_uSize = uSize;
        m_uTail = 0;
        m_uHead = 0;
        return ErrorCode::kSuccess;
    }

    int32_t Recv(int32_t iRecvFd, uint32_t uMaxRecvLength, uint32_t &uRecvLength, bool &bEOF)
    {
        bEOF = false;
        uRecvLength = 0;

        if (unlikely(m_uTail + 1024 >= m_uSize))
        {
            if (Expand(m_uSize) != ErrorCode::kSuccess)
            {
                return ErrorCode::kOutOfMemory;
            }
        }

        auto pWriteBegin = m_pData + m_uTail;
        auto uWriteLength =std::min(m_uSize - m_uTail, uMaxRecvLength);
        auto iRecv = recv(iRecvFd, pWriteBegin, uWriteLength, 0);
        if (iRecv > 0)
        {
            m_uTail += iRecv;
            uRecvLength = uint32_t(iRecv);
            return ErrorCode::kSuccess;
        }
        else if (iRecv == 0)
        {
            bEOF = true;
            return ErrorCode::kSuccess;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return ErrorCode::kSuccess;
            }
            return ErrorCode::kSystemError;
        }
    }

    int32_t Expand(uint32_t uLength)
    {
        if (unlikely(m_uTail != m_uHead))
        {
            memmove(m_pData, m_pData + m_uHead, m_uTail - m_uHead);
            m_uTail -= m_uHead;
            m_uHead = 0;
        }

        auto pNewData = reinterpret_cast<uint8_t *>(m_pAllocator->Malloc(m_uSize + uLength));
        if (pNewData == nullptr)
        {
            return ErrorCode::kOutOfMemory;
        }

        memcpy(pNewData, m_pData, m_uSize);
        m_pAllocator->Free(m_pData);
        m_pData = pNewData;
        m_uSize += uLength;
        return ErrorCode::kSuccess;
    }

    uint8_t *GetReadBegin() const
    {
        return m_pData + m_uHead;
    }

    uint32_t GetReadLength() const
    {
        return m_uTail - m_uHead;
    }

    void Consume(uint32_t uLength)
    {
        m_uHead += uLength;
        if (unlikely(m_uHead == m_uTail))
        {
            // 所有数据都已消费，重置指针
            m_uHead = 0;
            m_uTail = 0;
        }
    }

private:
    base::memory::IAllocator *m_pAllocator{nullptr};
    uint8_t *m_pData{nullptr};
    uint32_t m_uSize{0};
    uint32_t m_uTail{0};
    uint32_t m_uHead{0};
};

}
}

#endif // __CPPX_NETWORK_RECEIVE_BUFFER_H__
