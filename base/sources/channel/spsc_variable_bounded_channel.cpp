#include "spsc_variable_bounded_channel.h"
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

CSPSCVariableBoundedChannel::~CSPSCVariableBoundedChannel()
{
    if (m_pDatap != nullptr)
    {
        memory::IAllocator::GetInstance()->Free(m_pDatap);
        m_pDatap = nullptr;
        m_pDatac = nullptr;
    }
}

int32_t CSPSCVariableBoundedChannel::Init(uint64_t uMaxMemorySizeKB)
{
    m_uSizep = Up2PowerOf2(uMaxMemorySizeKB * 1024);
    m_uSizec = m_uSizep;
    m_uTail = 0;
    m_uHeadRef = 0;
    m_Statsp.Reset();
    m_Statsc.Reset();

    auto pData = reinterpret_cast<uint8_t *>(memory::IAllocator::GetInstance()->Malloc(m_uSizep));
    if (pData == nullptr)
    {
        return ErrorCode::kOutOfMemory;
    }

    m_pDatap = pData;
    m_pDatac = pData;

    return ErrorCode::kSuccess;
}

Entry *CSPSCVariableBoundedChannel::New()
{
    return nullptr;
}

Entry *CSPSCVariableBoundedChannel::New(uint32_t uSize)
{
    auto uEntrySize = Entry::CalSize(uSize);
    auto pData = NewEntry(uEntrySize);
    if (likely(pData != nullptr))
    {
        auto pEntry = reinterpret_cast<Entry *>(pData);
        pEntry->uMagic = kMagic;
        pEntry->uFlags = 0;
        pEntry->uLength = uSize;
        m_Statsp.uCount++;
        return pEntry;
    }

    m_uHeadRef = ACCESS_ONCE(m_uHead);
    pData = NewEntry(uEntrySize);
    if (likely(pData != nullptr))
    {
        auto pEntry = reinterpret_cast<Entry *>(pData);
        pEntry->uMagic = kMagic;
        pEntry->uFlags = 0;
        pEntry->uLength = uSize;
        m_Statsp.uCount++;
        return pEntry;
    }

    m_Statsp.uFailed++;
    return nullptr;
}

void CSPSCVariableBoundedChannel::Post(Entry *pEntry)
{
    if (likely(pEntry != nullptr && pEntry->uMagic == kMagic))
    {
        std::atomic_thread_fence(std::memory_order_release);
        m_uTail += pEntry->GetTotalLength();
        m_Statsp.uCount++;
    }

    m_Statsp.uFailed2++;
}

Entry *CSPSCVariableBoundedChannel::Get()
{
    auto pData = GetEntry();
    if (likely(pData != nullptr))
    {
        assert(reinterpret_cast<Entry *>(pData)->uMagic == kMagic);
        assert((reinterpret_cast<Entry *>(pData)->uFlags & kPlacehold) == 0);
        m_Statsc.uCount++;
        return reinterpret_cast<Entry *>(pData);
    }

    m_uTailRef = ACCESS_ONCE(m_uTail);
    pData = GetEntry();
    if (likely(pData != nullptr))
    {
        assert(reinterpret_cast<Entry *>(pData)->uMagic == kMagic);
        assert((reinterpret_cast<Entry *>(pData)->uFlags & kPlacehold) == 0);
        m_Statsc.uCount++;
        return reinterpret_cast<Entry *>(pData);
    }

    m_Statsc.uFailed++;
    return nullptr;
}

void CSPSCVariableBoundedChannel::Delete(Entry *pEntry)
{
    if (likely(pEntry != nullptr && pEntry->uMagic == kMagic))
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        m_uHead += pEntry->GetTotalLength();
        m_Statsc.uCount2++;
    }

    m_Statsc.uFailed2++;
}

bool CSPSCVariableBoundedChannel::IsEmpty() const
{
    return ACCESS_ONCE(m_Statsp.uCount2) == ACCESS_ONCE(m_Statsc.uCount2);
}

uint32_t CSPSCVariableBoundedChannel::GetSize() const
{
    return ACCESS_ONCE(m_Statsp.uCount2) - ACCESS_ONCE(m_Statsc.uCount2);
}

int32_t CSPSCVariableBoundedChannel::GetStats(IJson *pStats) const
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

void *CSPSCVariableBoundedChannel::NewEntry(uint32_t uNewSize)
{
    if (unlikely(m_uTail - m_uHeadRef >= m_uSizep))
    {
        return nullptr;
    }

    auto uTail = GetIndex(m_uTail, m_uSizep);
    auto uHead = GetIndex(m_uHeadRef, m_uSizep);

    /**
        *             head              tail
        * _____________|_________________|_________
    */
    if (uTail >= uHead)
    {
        /**
        *               head              tail
        * _______________|_________________|________________
        *                                   ______
        */
        if (likely(uTail + uNewSize <= m_uSizep))
        {
            return &m_pDatap[uTail];
        }
        
        /** set placeholder and change tail
            * tail             head
            * |_______________|_____
            *                  ___________
        */
        if (likely(m_uSizep - uTail >= Entry::CalSize(0)))
        {
            auto pEntry = reinterpret_cast<Entry *>(&m_pDatap[uTail]);
            pEntry->uMagic = kMagic;
            pEntry->uFlags = kPlacehold;
            pEntry->uLength = m_uSizep - uTail;
        }
        std::atomic_thread_fence(std::memory_order_release);
        m_uTail += (m_uSizep - uTail);
        assert(GetIndex(m_uTail, m_uSizep) == 0);
        uTail = 0;

        /** 
            * tail             head
            * |_______________|______________________
            *  _______
        */
        if (uTail + uNewSize <= uHead)
        {
            return &m_pDatap[uTail];
        }
    }
    /**
    *             tail             head
    * _____________|________________|_________
    */
    else
    {
        /**
        *              tail             head
        *  _____________|_______________|______________________
        *                   _______
        */
        if (likely(uTail + uNewSize <= uHead))
        {
            return &m_pDatap[uTail];
        }
    }

    return nullptr;
}

void *CSPSCVariableBoundedChannel::GetEntry()
{
    if (unlikely(m_uHead < m_uTailRef))
    {
        return nullptr;
    }

    auto uHead = GetIndex(m_uHead, m_uSizec);
    auto uTail = GetIndex(m_uTailRef, m_uSizec);

    /**
        *             head              tail
        * _____________|_________________|_________
    */
    if (uHead < uTail)
    {
        return &m_pDatac[uHead];
    }
    /**
     *             tail             head
     * _____________|________________|_________
     */
    else if (uHead >= uTail)
    {
        /**
            *             tail             head
            * _____________|________________|__________
            *                                ____
        */
        auto pEntry = reinterpret_cast<Entry *>(&m_pDatac[uHead]);
        if (unlikely(m_uSizec - uHead > Entry::CalSize(0) 
            && pEntry->uFlags != kPlacehold))
        {
            return &m_pDatac[uHead];
        }
        
        /** set placeholder and change tail
            * head            tail
            *  |_______________|______________________
            *   _____
        */
        std::atomic_thread_fence(std::memory_order_acquire);
        m_uHead = (m_uSizec - uHead) > Entry::CalSize(0) ? (m_uSizec - uHead) : pEntry->GetTotalLength();
        assert(GetIndex(m_uHead, m_uSizec) == 0);
        uHead = 0;
        if (uHead < uTail)
        {
            return &m_pDatac[0];
        }
    }

    return nullptr;
}

template<>
SPSCVariableBoundedChannel *SPSCVariableBoundedChannel::Create(const ChannelConfig *pConfig)
{
    auto pChannel = memory::IAllocatorEx::GetInstance()->New<CSPSCVariableBoundedChannel>();
    if (likely(pChannel != nullptr))
    {
        auto iErrorNo = pChannel->Init(pConfig->uTotalMemorySizeMB);
        if (iErrorNo != ErrorCode::kSuccess)
        {
            memory::IAllocatorEx::GetInstance()->Delete(pChannel);
            return nullptr;
        }
        return reinterpret_cast<SPSCVariableBoundedChannel *>(pChannel);
    }
    return nullptr;
}

template<>
void SPSCVariableBoundedChannel::Destroy(IChannel *pChannel)
{
    memory::IAllocatorEx::GetInstance()->Delete(reinterpret_cast<CSPSCVariableBoundedChannel *>(pChannel));
}

template<>
void *SPSCVariableBoundedChannel::New()
{
    return reinterpret_cast<CSPSCVariableBoundedChannel *>(this)->New();
}

template<>
void *SPSCVariableBoundedChannel::New(uint32_t uSize)
{
    return reinterpret_cast<CSPSCVariableBoundedChannel *>(this)->New(uSize);
}

template<>
void SPSCVariableBoundedChannel::Post(void *pData)
{
    if (likely(pData != nullptr))
    {
        auto pEntry = reinterpret_cast<Entry *>(Entry::GetEntry(pData));
        reinterpret_cast<CSPSCVariableBoundedChannel *>(this)->Post(pEntry);
    }
}

template<>
void *SPSCVariableBoundedChannel::Get()
{
    return reinterpret_cast<CSPSCVariableBoundedChannel *>(this)->Get();
}

template<>
void SPSCVariableBoundedChannel::Delete(void *pData)
{
    if (likely(pData != nullptr))
    {
        auto pEntry = reinterpret_cast<Entry *>(Entry::GetEntry(pData));
        reinterpret_cast<CSPSCVariableBoundedChannel *>(this)->Delete(pEntry);
    }
}

template<>
bool SPSCVariableBoundedChannel::IsEmpty() const
{
    return reinterpret_cast<const CSPSCVariableBoundedChannel *>(this)->IsEmpty();
}

template<>
uint32_t SPSCVariableBoundedChannel::GetSize() const
{
    return reinterpret_cast<const CSPSCVariableBoundedChannel *>(this)->GetSize();
}

template<>
int32_t SPSCVariableBoundedChannel::GetStats(IJson *pStats) const
{
    return reinterpret_cast<const CSPSCVariableBoundedChannel *>(this)->GetStats(pStats);
}

}
}
}
