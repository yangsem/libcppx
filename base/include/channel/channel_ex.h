#ifndef __CPPX_CHANNEL_EX_H__
#define __CPPX_CHANNEL_EX_H__

#include <channel/channel.h>
#include <cstring>
#include <utility>
#include <type_traits>

namespace cppx
{
namespace base
{
namespace channel
{

// SFINAE helper to detect if type T has SetInvalid() method
template<typename T>
class has_set_invalid
{
private:
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().SetInvalid(), std::true_type{});
    
    template<typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// Helper function to call SetInvalid if available
template<typename T>
typename std::enable_if<has_set_invalid<T>::value>::type
call_set_invalid(T *pObj)
{
    pObj->SetInvalid();
}

template<typename T>
typename std::enable_if<!has_set_invalid<T>::value>::type
call_set_invalid(T *pObj)
{
    // Do nothing for types without SetInvalid()
    (void)pObj;
}

template<typename T, ChannelType eChannelType, LengthType eLengthType>
class IChannelEx final : public IChannel<eChannelType, ElementType::kFixedSize, eLengthType>
{
    using ChannelType = IChannel<eChannelType, ElementType::kFixedSize, eLengthType>;
protected:
    virtual ~IChannelEx() = default;

public:
    static IChannelEx *Create(const ChannelConfig *pConfig)
    {
        const_cast<ChannelConfig *>(pConfig)->uElementSize = sizeof(T);
        return reinterpret_cast<IChannelEx *>(ChannelType::Create(pConfig));
    }

    static void Destroy(IChannelEx *pChannel)
    {
        ChannelType::Destroy(reinterpret_cast<ChannelType *>(pChannel));
    }

    /**
     * @brief 推入一个元素
     * @param t 元素
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    int32_t Push(T &&t)
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
                call_set_invalid(reinterpret_cast<T *>(pData));
                this->Post(pData);
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
    int32_t Pop(T &t)
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
