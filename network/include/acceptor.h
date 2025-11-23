#ifndef __CPPX_NETWORK_ACCEPTOR_H__
#define __CPPX_NETWORK_ACCEPTOR_H__

#include <cstdint>
#include <utilities/json.h>

namespace cppx
{
namespace network
{

using NetworkConfig = base::IJson;

class IAcceptor
{
protected:
    virtual ~IAcceptor() = default;

public:
    /**
     * @brief 启动接收器
     * @return 0表示成功,否则失败
     */
    virtual int32_t Start() = 0;

    /**
     * @brief 停止接收器
     */
    virtual void Stop() = 0;

    /**
     * @brief 获取接收器ID
     * @return 接收器ID
     */
    virtual uint64_t GetID() const = 0;

    /**
     * @brief 获取接收器名称
     * @return 接收器名称
     */
    virtual const char *GetName() const = 0;
};

}
}
#endif // __CPPX_NETWORK_ACCEPTOR_H__
