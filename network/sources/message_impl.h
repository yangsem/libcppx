#ifndef __CPPX_NETWORK_MESSAGE_IMPL_H__
#define __CPPX_NETWORK_MESSAGE_IMPL_H__

#include <atomic>
#include <cstdint>
#include <message.h>
#include <utilities/common.h>

namespace cppx {
namespace network {

class CMessageImpl final : public IMessage {
public:
    CMessageImpl();
    ~CMessageImpl() override;

    uint8_t *GetData() override { return m_pData; }

    const uint8_t *GetData() const override { return m_pData; }

    uint32_t GetDataLength() const override { return m_uSize; }

    IConnection *GetConnection() const override
    {
        return ClearPointerLastBit(m_pConnection);
    }

    void Hold() const override
    {
        m_pConnection = SetPointerLastBit(m_pConnection);
    }

    int32_t SetSize(uint32_t uSize) override
    {
        m_uSize = uSize;
        return 0;
    }

    int32_t Append(const void *pData, uint32_t uLength) override 
    {
        memcpy(m_pData + m_uSize, pData, uLength);
        m_uSize += uLength;
        return 0;
    }

private:
    uint8_t *m_pData{nullptr};
    uint32_t m_uSize{0};
    uint32_t m_uCapacity{0};
    mutable IConnection *m_pConnection{nullptr};
};

} // namespace network
} // namespace cppx
#endif // __CPPX_NETWORK_MESSAGE_IMPL_H__
