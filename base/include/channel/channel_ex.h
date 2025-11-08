#ifndef __CPPX_CHANNEL_EX_H__
#define __CPPX_CHANNEL_EX_H__

#include <channel/channel.h>

namespace cppx
{
namespace base
{
namespace channel
{

class ChannelEx : public IChannel
{
protected:
    virtual ~ChannelEx() noexcept = default;

public:
    static ChannelEx *Create(const ChannelConfig &stConfig) noexcept
    {
        return reinterpret_cast<ChannelEx *>(IChannel::Create(stConfig));
    }

    static void Destroy(ChannelEx *pChannel) noexcept
    {
        IChannel::Destroy(reinterpret_cast<IChannel *>(pChannel));
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
        Entry *pEntry = NewEntry();
        if (likely(pEntry != nullptr))
        {
            try
            {
                new (pEntry) T(std::forward<T>(t));
                PostEntry(pEntry);
                return 0;
            }
            catch (const std::exception &e)
            {
                const_cast<uint16_t &>(pEntry->uFlags) |= kInvalid;
                DeleteEntry(pEntry);
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
        Entry *pEntry = GetEntry();
        if (likely(pEntry != nullptr))
        {
            try
            {
                t = std::move(*reinterpret_cast<T *>(pEntry->pData));
                DeleteEntry(pEntry);
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
