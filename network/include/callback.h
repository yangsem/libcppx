#ifndef __CPPX_NETWORK_CALLBACK_H__
#define __CPPX_NETWORK_CALLBACK_H__

#include <cstdint>

namespace cppx
{
namespace network
{

class IMessage;
class IConnection;

class ICallback
{
protected:
    virtual ~ICallback() = default;

public:
    /**
     * @brief 获取消息长度
     * @param pData 消息数据
     * @param uLength 消息长度
     * @return 完整消息长度，0表示无法识别，UINT32_MAX表示异常数据断开连接
     */
    virtual uint32_t OnMessageLength(const uint8_t *pData, uint32_t uLength) = 0;

    /**
     * @brief 处理消息
     * @param pMessage 消息
     */
    virtual void OnMessage(IConnection *pConnection, const IMessage *pMessage) = 0;

    /**
     * @brief 连接成功回调
     * @param pConnection 连接
     * @return 0表示成功,否则断开连接
     */
    virtual int32_t OnConnected(const IConnection *pConnection) = 0;

    /**
     * @brief 连接断开回调
     * @param pConnection 连接
     */
    virtual void OnDisconnected(const IConnection *pConnection) = 0;

    /**
     * @brief 接收连接回调
     * @param pConnection 连接
     * @return 0表示成功,否则断开连接
     */
    virtual int32_t OnAccept(const IConnection *pConnection) = 0;

    /**
     * @brief 心跳超时回调
     * @param pConnection 连接
     * @return 0表示成功,否则断开连接
     */
    virtual int32_t OnHeartbeatTimeout(const IConnection *pConnection) = 0;

    /**
     * @brief 错误回调
     * @param pConnection 连接
     * @param pErrorMsg 错误消息
     */
    virtual void OnError(const IConnection *pConnection, const char *pErrorMsg) = 0;
};

}
}
#endif // __CPPX_NETWORK_CALLBACK_H__
