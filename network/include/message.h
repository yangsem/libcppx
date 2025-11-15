#ifndef __CPPX_NETWORK_MESSAGE_H__
#define __CPPX_NETWORK_MESSAGE_H__

#include <cstdint>

namespace cppx
{
namespace network
{

class IConnection;

class IMessage
{
protected:
    virtual ~IMessage() = default;

public:
    /**
     * @brief 获取消息数据
     * @return 消息数据指针
     */
    virtual uint8_t *GetData() = 0;

    /**
     * @brief 获取消息数据常量指针
     * @return 消息数据指针
     */
    virtual const uint8_t *GetData() const = 0;

    /**
     * @brief 获取消息数据长度
     * @return 消息数据长度
     */
    virtual uint32_t GetDataLength() const = 0;

    /**
     * @brief 获取连接
     * @return 连接指针
     */
    virtual IConnection *GetConnection() const = 0;

    /**
     * @brief 持有消所有权，持有后需要通过DeleteMessage释放
     */
    virtual void Hold() const = 0;

    /**
     * @brief 设置消息数据长度
     * @param uSize 消息数据长度
     * @return 0表示成功,否则失败
     */
    virtual int32_t SetSize(uint32_t uSize) = 0;

    /**
     * @brief 追加消息数据
     * @param pData 消息数据指针
     * @param uLength 消息数据长度
     * @return 0表示成功,否则失败
     */
    virtual int32_t Append(const void *pData, uint32_t uLength) = 0;
};

}
}
#endif // __CPPX_NETWORK_MESSAGE_H__
