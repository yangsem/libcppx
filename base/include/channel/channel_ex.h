#ifndef __CPPX_CHANNEL_EX_H__
#define __CPPX_CHANNEL_EX_H__

#include <channel/channel.h>
#include <utility>

namespace cppx
{
namespace base
{
namespace channel
{

template<typename T, ChannelType eChannelType, LengthType eLengthType>
class IChannelEx final : public IChannel<eChannelType, ElementType::kFixedSize, eLengthType>
{
    using ChannelType = IChannel<eChannelType, ElementType::kFixedSize, eLengthType>;
protected:
    virtual ~IChannelEx() noexcept = default;

public:
    static IChannelEx *Create(const ChannelConfig *pConfig) noexcept
    {
        const_cast<ChannelConfig *>(pConfig)->uElementSize = sizeof(T);
        return reinterpret_cast<IChannelEx *>(ChannelType::Create(pConfig));
    }

    static void Destroy(IChannelEx *pChannel) noexcept
    {
        ChannelType::Destroy(reinterpret_cast<ChannelType *>(pChannel));
    }

    /**
     * @brief 推入一个元素
     * @param t 元素
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    int32_t Push(T &&t) noexcept
    {
        auto pData = this->New();
        if (likely(pData != nullptr))
        {
            try
            {
                new (pData) T(std::forward<T>(t));
                this->Post(pData);
                return 0;
            }
            catch (const std::exception &e)
            {
                this->Post(pData, false);
            }
        }
        return -1;
    }

    /**
     * @brief 弹出一个元素
     * @param t 元素
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    int32_t Pop(T &t) noexcept
    {
        auto pData = this->Get();
        if (likely(pData != nullptr))
        {
            try
            {
                t = std::move(*reinterpret_cast<T *>(pData));
                this->Delete(pData);
                return 0;
            }
            catch (const std::exception &e)
            {
            }
        }
        return -1;
    }
};

}
}
}
#endif // __CPPX_CHANNEL_EX_H__
