#ifndef __CPPX_NETWORK_CONNECTION_H__
#define __CPPX_NETWORK_CONNECTION_H__

#include "message.h"
#include <cstdint>
#include <utilities/json.h>

namespace cppx
{
namespace network
{
class IMessage;
using NetworkConfig = base::IJson;

class IConnection
{
protected:
    virtual ~IConnection() = default;

public:
    /**
     * @brief 创建消息
     * @param uLength 消息长度
     * @return 消息指针,失败返回NULL
     */
    virtual IMessage *NewMessage(uint32_t uLength) = 0;

    /**
     * @brief 释放消息
     * @param pMessage 消息指针
     */
    virtual void DeleteMessage(IMessage *pMessage) = 0;

    /**
     * @brief 发送消息
     * @param pMessage 消息指针
     * @param bPriority 优先发送
     * @return 0表示成功,否则失败
     */
    virtual int32_t Send(IMessage *pMessage, bool bPriority = false) = 0;

    /**
     * @brief 发送消息
     * @param pData 消息数据
     * @param uLength 消息长度
     * @param bPriority 优先发送
     * @return 0表示成功,否则失败
     */
    virtual int32_t Send(const uint8_t *pData, uint32_t uLength, bool bPriority = false) = 0;

    /**
     * @brief 接收消息
     * @param pMessage 消息指针
     * @param uTimeoutMs 超时时间，单位毫秒，0表示不超时
     * @return 0表示成功,否则失败
     */
    virtual int32_t Recv(IMessage **ppMessage, uint32_t uTimeoutMs = 0) = 0;

    /**
     * @brief 接收消息
     * @param pData 接收消息数据指针
     * @param uLength 接收消息长度
     * @param uTimeoutMs 超时时间，单位毫秒，0表示不超时
     * @return 0表示成功,否则失败
     */
    virtual int32_t Recv(void *pData, uint32_t uLength, uint32_t uTimeoutMs = 0) = 0;

    /**
     * @brief 零拷贝同步调用
     * @param pRequest 请求消息指针
     * @param pResponse 响应消息指针
     * @param uTimeoutMs 超时时间，单位毫秒，0表示不超时
     * @return 0表示成功,否则失败
     */
    virtual int32_t Call(IMessage *pRequest, IMessage *pResponse, uint32_t uTimeoutMs = 0) = 0;

    /**
     * @brief 非零拷贝同步调用
     * @param pRequest 请求消息数据
     * @param uRequestLength 请求消息长度
     * @param pResponse 响应消息指针
     * @param uTimeoutMs 超时时间，单位毫秒，0表示不超时
     * @return 0表示成功,否则失败
     */
    virtual int32_t Call(const uint8_t *pRequest, uint32_t uRequestLength, IMessage *pResponse, uint32_t uTimeoutMs = 0) = 0;

    /**
     * @brief 连接远程服务器
     * @param pRemoteIP 远程服务器IP地址
     * @param uRemotePort 远程服务器端口
     * @param uTimeoutMs 超时时间，单位毫秒，0表示不超时
     * @return 0表示成功,否则失败
     */
    virtual int32_t Connect(const char *pRemoteIP, uint16_t uRemotePort, uint32_t uTimeoutMs = 0) = 0;

    /**
     * @brief 关闭连接
     */
    virtual void Close() = 0;

    /**
     * @brief 是否连接成功
     * @return 是否连接成功
     */
    virtual bool IsConnected() const = 0;

    /**
     * @brief 获取连接ID
     * @return 连接ID
     */
    virtual uint64_t GetID() const = 0;

    /**
     * @brief 获取IO线程索引
     * @return IO线程索引
     */
    virtual uint32_t GetIOThreadIndex() const = 0;

    /**
     * @brief 获取对端IP地址
     * @return 对端IP地址
     */
    virtual const char *GetRemoteIP() const = 0;

    /**
     * @brief 获取对端端口
     * @return 对端端口
     */
    virtual uint16_t GetRemotePort() const = 0;

    /**
     * @brief 获取本地IP地址
     * @return 本地IP地址
     */
    virtual const char *GetLocalIP() const = 0;

    /**
     * @brief 获取本地端口
     * @return 本地端口
     */
    virtual uint16_t GetLocalPort() const = 0;

    /**
     * @brief 获取连接名称
     * @return 连接名称
     */
    virtual const char *GetName() const = 0;
};

}
}
#endif // __CPPX_NETWORK_CONNECTION_H__
