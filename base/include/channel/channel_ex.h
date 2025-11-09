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

template<ChannelType eChannelType>
class IChannelEx final : public IChannel<eChannelType>
{
    using ChannelBase = IChannel<eChannelType>;
protected:
    virtual ~IChannelEx() noexcept = default;

public:
    static IChannelEx *Create(const ChannelConfig *pConfig) noexcept
    {
        return reinterpret_cast<IChannelEx *>(ChannelBase::Create(pConfig));
    }

    static void Destroy(IChannelEx *pChannel) noexcept
    {
        ChannelBase::Destroy(reinterpret_cast<ChannelBase *>(pChannel));
    }

    /**
     * @brief 推入一个元素
     * @param t 元素
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    template<typename T>
    int32_t Push(T &&t) noexcept
    {
        Entry *pEntry = this->NewEntry();
        if (likely(pEntry != nullptr))
        {
            try
            {
                new (pEntry) T(std::forward<T>(t));
                this->PostEntry(pEntry);
                return 0;
            }
            catch (const std::exception &e)
            {
                const_cast<uint16_t &>(pEntry->uFlags) |= kInvalid;
                this->DeleteEntry(pEntry);
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
    template<typename T>
    int32_t Pop(T &t) noexcept
    {
        Entry *pEntry = this->GetEntry();
        if (likely(pEntry != nullptr))
        {
            try
            {
                t = std::move(*reinterpret_cast<T *>(pEntry->data()));
                this->DeleteEntry(pEntry);
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
