#include "mpsc_variable_bounded_channel.h"
#include <memory/allocator_ex.h>
#include <utilities/error_code.h>
#include <atomic>

namespace cppx
{
namespace base
{
namespace channel
{

CMPSCVariableBoundedChannel::~CMPSCVariableBoundedChannel()
{
    if (m_pDatap != nullptr)
    {
        IAllocator::GetInstance()->Free(m_pDatap);
        m_pDatap = nullptr;
        m_pDatac = nullptr;
    }
}

int32_t CMPSCVariableBoundedChannel::Init(uint64_t uMaxMemorySizeKB)
{
    m_uSizep = Up2PowerOf2(uMaxMemorySizeKB * 1024);
    m_uSizec = m_uSizep;
    m_uTail = 0;
    m_uHeadRef = 0;
    m_Statsp.Reset();
    m_Statsc.Reset();

    auto pData = reinterpret_cast<uint8_t *>(IAllocator::GetInstance()->Malloc(m_uSizep));
    if (pData == nullptr)
    {
        return ErrorCode::kOutOfMemory;
    }

    m_pDatap = pData;
    m_pDatac = pData;

    return ErrorCode::kSuccess;
}

Entry *CMPSCVariableBoundedChannel::New()
{
    return nullptr;
}

Entry *CMPSCVariableBoundedChannel::New(uint32_t uSize)
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

}
}
}
