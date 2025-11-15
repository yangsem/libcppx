#ifndef __CPPX_NETWORK_ENGINE_H__
#define __CPPX_NETWORK_ENGINE_H__

#include "utilities/error_code.h"
#include <logger/logger.h>
#include <utilities/json.h>

namespace cppx
{
namespace network
{

constexpr const char *kModuleName = "network";

using NetworkLogger = base::logger::ILogger;
using NetworkConfig = base::IJson;
using NetworkStats = base::IJson;
using base::ErrorCode;

class IMessage;
class ICallback;
class IConnection;
class IAcceptor;

class EXPORT IEngine
{
protected:
    virtual ~IEngine() = default;
    
public:
    /**
     * @brief 创建网络引擎
     * @param pLogger 日志器
     * @return 网络引擎指针,失败返回NULL
     */
    static IEngine *Create(NetworkLogger *pLogger);

    /**
     * @brief 销毁网络引擎
     * @param pEngine 网络引擎指针
     */
    static void Destroy(IEngine *pEngine);

    /**
     * @brief 初始化网络引擎
     * @param pConfig 配置
     * @return 0表示成功,否则失败
     */
    virtual int32_t Init(NetworkConfig *pConfig) = 0;

    /**
     * @brief 退出网络引擎
     */
    virtual void Exit() = 0;

    /**
     * @brief 启动网络引擎线程
     * @return 0表示成功,否则失败
     */
    virtual int32_t Start() = 0;

    /**
     * @brief 停止网络引擎线程
     */
    virtual void Stop() = 0;

    /**
     * @brief 创建接收器
     * @param pConfig 配置
     * @param pCallback 回调
     * @return 0表示成功,否则失败
     */
    virtual int32_t CreateAcceptor(NetworkConfig *pConfig, ICallback *pCallback) = 0;

    /**
     * @brief 销毁接收器
     * @param pAcceptor 接收器指针
     */
    virtual void DestroyAcceptor(IAcceptor *pAcceptor) = 0;

    /**
     * @brief 创建连接
     * @param pConfig 配置
     * @param pCallback 回调
     * @return 0表示成功,否则失败
     */
    virtual int32_t CreateConnection(NetworkConfig *pConfig, ICallback *pCallback) = 0;

    /**
     * @brief 销毁连接
     * @param pConnection 连接指针
     */
    virtual void DestroyConnection(IConnection *pConnection) = 0;

    /**
     * @brief 将连接从IO线程中分离通过Send/Recv同步处理
     * @param pConnection 连接指针
     * @return 0表示成功,否则失败
     */
    virtual int32_t DetachConnection(IConnection *pConnection) = 0;

    /**
     * @brief 将连接附加到IO线程中异步处理Send/Recv
     * @param pConnection 连接指针
     * @return 0表示成功,否则失败
     */
    virtual int32_t AttachConnection(IConnection *pConnection) = 0;

    /**
     * @brief 创建消息
     * @param uLength 消息长度
     * @return 消息指针,失败返回NULL
     */
    virtual IMessage *NewMessage(uint32_t uLength) = 0;

    /**
     * @brief 销毁消息
     * @param pMessage 消息指针
     */
    virtual void DeleteMessage(IMessage *pMessage) = 0;

    /**
     * @brief 获取网络引擎统计信息
     * @param pStats 统计信息
     * @return 0表示成功,否则失败
     */
    virtual int32_t GetStats(NetworkStats *pStats) const = 0;
};

namespace config
{
/* ============================== 网络引擎配置 ============================== */
constexpr const char *kEngineName = "engine_name"; // 网络引擎名称，类型: string
constexpr const char *kIOThreadCount = "io_thread_count"; // IO线程数量，类型: uint32_t
constexpr const char *kIOReadWriteBytes = "io_read_write_bytes"; // IO线程读写策略，每一轮读取的数据量大小，类型: uint32_t

/* ============================== 网络引擎Listener配置 ============================== */
constexpr const char *kAcceptorName = "acceptor_name"; // 接收器名称，类型: string
constexpr const char *kAcceptorIP = "acceptor_ip"; // 接收器IP地址，类型: string
constexpr const char *kAcceptorPort = "acceptor_port"; // 接收器端口，类型: uint16_t

/* ============================== 网络引擎Connection配置 ============================== */
constexpr const char *kConnectionName = "connection_name"; // 连接名称，类型: string
constexpr const char *kConnectionRemoteIP = "connection_remote_ip"; // 连接远程IP地址，类型: string
constexpr const char *kConnectionRemotePort = "connection_remote_port"; // 连接远程端口，类型: uint16_t
constexpr const char *kIsSyncConnect = "is_sync_connect"; // 是否同步连接，类型: bool
constexpr const char *kConnectTimeoutMs = "connect_timeout_ms"; // 建立连接超时，类型: uint32_t

/* ============================== 网络引擎公共配置,全局和监听器、连接都可以使用 ============================== */
constexpr const char *kProtocol = "protocol"; // 协议，类型: string
constexpr const char *kIsASyncSend = "is_async_send"; // 是否异步发送，类型: bool
constexpr const char *kSocketBufferBytes = "socket_buffer_bytes"; // 套接字缓冲区字节大小，类型: uint32_t
constexpr const char *kHeartbeatIntervalMs = "heartbeat_interval_ms"; // 心跳间隔，类型: uint32_t
constexpr const char *kHeartbeatTimeoutMs = "heartbeat_timeout_ms"; // 心跳超时，类型: uint32_t
}

namespace default_value
{
/* ============================== 网络引擎默认值 ============================== */
constexpr const char *kEngineName = ""; // 网络引擎名称，默认匿名网络引擎
constexpr const uint32_t kIOThreadCount = 1; // IO线程数量，默认1
constexpr const uint32_t kIOReadWriteBytes = 0; // IO线程读写策略，每一轮读取的数据量大小，默认0，不限制

/* ============================== 网络引擎Listener默认值 ============================== */
constexpr const char *kAcceptorName = ""; // 接收器名称，默认匿名接收器
constexpr const char *kAcceptorIP = "0.0.0.0"; // 接收器IP地址，默认所有IP
constexpr const uint32_t kAcceptorPort = 8080; // 接收器端口，默认8080

/* ============================== 网络引擎Connection默认值 ============================== */
constexpr const char *kConnectionName = ""; // 连接名称，默认匿名连接
constexpr const char *kConnectionRemoteIP = "127.0.0.1"; // 连接远程IP地址，默认127.0.0.1
constexpr const uint32_t kConnectionRemotePort = 8080; // 连接远程端口，默认8080
constexpr const uint32_t kIsSyncConnect = false; // 是否同步连接，默认false
constexpr const uint32_t kConnectTimeoutMs = 30000; // 建立连接超时，默认30秒

/* ============================== 网络引擎公共默认值 ============================== */
constexpr const char *kProtocol = "tcp"; // 协议，默认tcp，可选tcp、udp
constexpr const bool kIsASyncSend = true; // 是否异步发送，默认true
constexpr const uint32_t kSocketBufferBytes = 0; // 套接字缓冲区字节大小，默认不设置,使用系统默认值
constexpr const uint32_t kHeartbeatIntervalMs = 1000; // 心跳间隔，默认1秒
constexpr const uint32_t kHeartbeatTimeoutMs = 30000; // 心跳超时，默认30秒
}

}
}
#endif // __CPPX_NETWORK_ENGINE_H__