#ifndef __CPPX_CHANNEL_H__
#define __CPPX_CHANNEL_H__

#include <cstdint>
#include <cstdint>
#include <utilities/common.h>
#include <utilities/json.h>

namespace cppx
{
namespace base
{
namespace channel
{

enum class ChannelType : uint8_t
{
    kSPSC = 0, // 单生产者单消费者
    kSPMC,     // 单生产者多消费者
    kMPSC,     // 多生产者单消费者
    kMPMC,     // 多生产者多消费者
};

enum class ElementType : uint8_t
{
    kFixedSize = 0, // 固定大小
    kVariableSize,  // 动态大小
};

enum class LengthType : uint8_t
{
    kBounded = 0, // 固定长度
    kUnbounded,   // 动态长度
};

struct ChannelConfig 
{
    uint32_t uElementSize;
    uint32_t uMaxElementCount;
    uint32_t uTotalMemorySizeKB;
};

template<ChannelType eChannelType, ElementType eElementType, LengthType eLengthType>
class IChannel
{
protected:
    virtual ~IChannel() = default;

public:
    /**
     * @brief 创建一个通道
     * @param stConfig 通道配置
     * @return 成功返回通道指针，失败返回nullptr
     * @note 多线程安全
     */
    static IChannel *Create(const ChannelConfig *pConfig);

    /**
     * @brief 销毁一个通道
     * @param pChannel 通道指针
     * @note 多线程安全
     */
    static void Destroy(IChannel *pChannel);

    /**
     * @brief 创建一个元素
     * @return 成功返回元素指针，失败返回nullptr
     * @note 多线程安全
     */
    void *New();

    /**
     * @brief 创建一个元素
     * @param uSize 元素大小
     * @return 成功返回元素指针，失败返回nullptr
     * @note 多线程安全
     */
    void *New(uint32_t uSize);

    /**
     * @brief 发布一个元素
     * @param pData 元素指针
     * @note 多线程安全
     */
    void Post(void *pData);

    /**
     * @brief 获取一个元素
     * @return 成功返回元素指针，失败返回nullptr
     * @note 多线程安全
     */
    void *Get();

    /**
     * @brief 释放一个元素
     * @param pData 元素指针
     * @note 多线程安全
     */
    void Delete(void *pData);

    /**
     * @brief 判断通道是否为空
     * @return 为空返回true，否则返回false
     * @note 多线程安全
     */
    bool IsEmpty() const;

    /**
     * @brief 获取通道当前元素个数
     * @return 通道当前元素个数
     * @note 多线程安全
     */
    uint32_t GetSize() const;

    /**
     * @brief 获取通道统计信息
     * @param pStats 统计信息对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    int32_t GetStats(IJson *pStats) const;
};

using SPSCFixedBoundedChannel = IChannel<ChannelType::kSPSC, ElementType::kFixedSize, LengthType::kBounded>;
using SPSCFixedUnboundedChannel = IChannel<ChannelType::kSPSC, ElementType::kFixedSize, LengthType::kUnbounded>;
using SPSCVariableBoundedChannel = IChannel<ChannelType::kSPSC, ElementType::kVariableSize, LengthType::kBounded>;
using SPSCVariableUnboundedChannel = IChannel<ChannelType::kSPSC, ElementType::kVariableSize, LengthType::kUnbounded>;

using SPMCFixedBoundedChannel = IChannel<ChannelType::kSPMC, ElementType::kFixedSize, LengthType::kBounded>;
using SPMCFixedUnboundedChannel = IChannel<ChannelType::kSPMC, ElementType::kFixedSize, LengthType::kUnbounded>;
using SPMCVariableBoundedChannel = IChannel<ChannelType::kSPMC, ElementType::kVariableSize, LengthType::kBounded>;
using SPMCVariableUnboundedChannel = IChannel<ChannelType::kSPMC, ElementType::kVariableSize, LengthType::kUnbounded>;

using MPSCFixedBoundedChannel = IChannel<ChannelType::kMPSC, ElementType::kFixedSize, LengthType::kBounded>;
using MPSCFixedUnboundedChannel = IChannel<ChannelType::kMPSC, ElementType::kFixedSize, LengthType::kUnbounded>;
using MPSCVariableBoundedChannel = IChannel<ChannelType::kMPSC, ElementType::kVariableSize, LengthType::kBounded>;
using MPSCVariableUnboundedChannel = IChannel<ChannelType::kMPSC, ElementType::kVariableSize, LengthType::kUnbounded>;

using MPMCFixedBoundedChannel = IChannel<ChannelType::kMPMC, ElementType::kFixedSize, LengthType::kBounded>;
using MPMCFixedUnboundedChannel = IChannel<ChannelType::kMPMC, ElementType::kFixedSize, LengthType::kUnbounded>;
using MPMCVariableBoundedChannel = IChannel<ChannelType::kMPMC, ElementType::kVariableSize, LengthType::kBounded>;
using MPMCVariableUnboundedChannel = IChannel<ChannelType::kMPMC, ElementType::kVariableSize, LengthType::kUnbounded>;

} // namespace channel
} // namespace base
} // namespace cppx

#endif // __CPPX_CHANNEL_H__
