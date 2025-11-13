#include "spsc_fixed_bounded_channel.h"
#include "utilities/common.h"
#include <memory/allocator_ex.h>
#include <utilities/error_code.h>
#include <atomic>

namespace cppx
{
namespace base
{
namespace channel
{

CSPSCFixedBoundedChannel::~CSPSCFixedBoundedChannel()
{
    if (m_pDatap != nullptr)
    {
        IAllocator::GetInstance()->Free(m_pDatap);
        m_pDatap = nullptr;
        m_pDatac = nullptr;
    }
}

int32_t CSPSCFixedBoundedChannel::Init(uint64_t uElemSize, uint64_t uSize)
{
    m_uElemSizep = ALIGN8(uElemSize);
    m_uElemSizec = m_uElemSizep;
    m_uSizep = Up2PowerOf2(uSize);
    m_uSizec = m_uSizep;
    m_uTail = 0;
    m_uHead = 0;
    m_uTailRef = m_uTail;
    m_uHeadRef = m_uHead;
    m_Statsp.Reset();
    m_Statsc.Reset();

    auto pData = reinterpret_cast<uint8_t *>(IAllocator::GetInstance()->Malloc(m_uSizep * m_uElemSizep));
    if (pData == nullptr)
    {
        SetLastError(ErrorCode::kOutOfMemory);
        return ErrorCode::kOutOfMemory;
    }
    m_pDatap = pData;
    m_pDatac = pData;

    return ErrorCode::kSuccess;
}

void *CSPSCFixedBoundedChannel::New()
{
    if (likely(m_uTail - m_uHeadRef < m_uSizep))
    {
        m_Statsp.uCount++;
        return &m_pDatap[GetIndex(m_uTail, m_uSizep) * m_uElemSizep];
    }

    m_uHeadRef = ACCESS_ONCE(m_uHead);
    if (likely(m_uTail - m_uHeadRef < m_uSizep))
    {
        m_Statsp.uCount++;
        return &m_pDatap[GetIndex(m_uTail, m_uSizep) * m_uElemSizep];
    }

    m_Statsp.uFailed++;
    return nullptr;
}

void *CSPSCFixedBoundedChannel::New(uint32_t uSize)
{
    UNSED(uSize);
    return nullptr;
}

void CSPSCFixedBoundedChannel::Post(void *pData)
{
    if (likely(pData != nullptr))
    {
        std::atomic_thread_fence(std::memory_order_release);
        m_uTail = (m_uTail + 1) % m_uSizep;
        m_Statsp.uCount++;
    }
    m_Statsp.uFailed2++;
}

void *CSPSCFixedBoundedChannel::Get()
{
    if (likely(m_uHead < m_uTailRef))
    {
        m_Statsc.uCount++;
        return &m_pDatac[GetIndex(m_uHead, m_uSizec) * m_uElemSizec];
    }

    m_uTailRef = ACCESS_ONCE(m_uTail);
    if (likely(m_uHead < m_uTailRef))
    {
        m_Statsc.uCount++;
        return &m_pDatac[GetIndex(m_uHead, m_uSizec) * m_uElemSizec];
    }

    m_Statsc.uFailed2++;
    return nullptr;
}

void CSPSCFixedBoundedChannel::Delete(void *pData)
{
    if (likely(pData != nullptr))
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        m_uHead = (m_uHead + 1) % m_uSizec;
        m_Statsc.uCount2++;
    }
    m_Statsc.uFailed2++;
}

bool CSPSCFixedBoundedChannel::IsEmpty() const
{
    return ACCESS_ONCE(m_uHead) == ACCESS_ONCE(m_uTail);
}

uint32_t CSPSCFixedBoundedChannel::GetSize() const
{
    return ACCESS_ONCE(m_uTail) - ACCESS_ONCE(m_uHead);
}

int32_t CSPSCFixedBoundedChannel::GetStats(IJson *pStats) const
{
    if (likely(pStats != nullptr))
    {
        auto pStatsp = pStats->SetObject("producer");
        if (likely(pStatsp != nullptr))
        {
            pStatsp->SetUint32("New", m_Statsp.uCount);
            pStatsp->SetUint32("NewFailed", m_Statsp.uFailed);
            pStatsp->SetUint32("Post", m_Statsp.uCount2);
            pStatsp->SetUint32("PostFailed", m_Statsp.uFailed2);
        }
        auto pStatsc = pStats->SetObject("consumer");
        if (likely(pStatsc != nullptr))
        {
            pStatsc->SetUint32("Get", m_Statsc.uCount);
            pStatsc->SetUint32("GetFailed", m_Statsc.uFailed);
            pStatsc->SetUint32("Delete", m_Statsc.uCount2);
            pStatsc->SetUint32("DeleteFailed", m_Statsc.uFailed2);
        }
        return ErrorCode::kSuccess;
    }

    return ErrorCode::kInvalidParam;
}

template<>
SPSCFixedBoundedChannel *SPSCFixedBoundedChannel::Create(const ChannelConfig *pConfig)
{
    auto pChannel = IAllocatorEx::GetInstance()->New<CSPSCFixedBoundedChannel>();
    if (likely(pChannel != nullptr))
    {
        auto iErrorNo = pChannel->Init(pConfig->uElementSize, pConfig->uMaxElementCount);
        if (iErrorNo != ErrorCode::kSuccess)
        {
            IAllocatorEx::GetInstance()->Delete(pChannel);
            return nullptr;
        }
        return reinterpret_cast<SPSCFixedBoundedChannel *>(pChannel);
    }
    return nullptr;
}

template<>
void SPSCFixedBoundedChannel::Destroy(IChannel *pChannel)
{
    IAllocatorEx::GetInstance()->Delete(reinterpret_cast<CSPSCFixedBoundedChannel *>(pChannel));
}

template<>
void *SPSCFixedBoundedChannel::New()
{
    return reinterpret_cast<CSPSCFixedBoundedChannel *>(this)->New();
}

template<>
void *SPSCFixedBoundedChannel::New(uint32_t uSize)
{
    return reinterpret_cast<CSPSCFixedBoundedChannel *>(this)->New(uSize);
}

template<>
void SPSCFixedBoundedChannel::Post(void *pData)
{
    reinterpret_cast<CSPSCFixedBoundedChannel *>(this)->Post(pData);
}

template<>
void *SPSCFixedBoundedChannel::Get()
{
    return reinterpret_cast<CSPSCFixedBoundedChannel *>(this)->Get();
}

template<>
void SPSCFixedBoundedChannel::Delete(void *pData)
{
    reinterpret_cast<CSPSCFixedBoundedChannel *>(this)->Delete(pData);
}

template<>
bool SPSCFixedBoundedChannel::IsEmpty() const
{
    return reinterpret_cast<const CSPSCFixedBoundedChannel *>(this)->IsEmpty();
}

template<>
uint32_t SPSCFixedBoundedChannel::GetSize() const
{
    return reinterpret_cast<const CSPSCFixedBoundedChannel *>(this)->GetSize();
}

template<>
int32_t SPSCFixedBoundedChannel::GetStats(IJson *pStats) const
{
    return reinterpret_cast<const CSPSCFixedBoundedChannel *>(this)->GetStats(pStats);
}
}
}
}
