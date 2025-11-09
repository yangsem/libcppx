#ifndef __CPPX_CHANNEL_H__
#define __CPPX_CHANNEL_H__

#include <cstdint>
#include <cstdint>
#include <utilities/common.h>

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

enum class LengthType : uint8_t
{
    kBounded = 0, // 有界
    kUnbounded,   // 无界
};

enum class ElementType : uint8_t
{
    kFixedSize = 0, // 固定大小
    kVariableSize,  // 变长
};

struct ChannelConfig 
{
    ChannelType eChannelType;
    LengthType eLengthType;
    ElementType eElementType;
    uint32_t uElementSize;
    uint32_t uMaxElementCount;
    uint32_t uTotalMemorySizeMB;
};

struct QueueStats
{
    uint32_t uPushCount;        // 入队次数
    uint32_t uPushSuccessCount; // 入队成功次数
    uint32_t uPushFailCount;    // 入队失败次数
    uint32_t uPopCount;         // 出队次数
    uint32_t uPopSuccessCount;  // 出队成功次数
    uint32_t uPopFailCount;     // 出队失败次数
};

enum EntryFlag : uint16_t
{
    kInvalid = 1 << 0,
};

struct Entry
{
    const uint16_t uMagic;
    const uint16_t uFlags;
    const uint32_t uSize;

    uint8_t *data() { return reinterpret_cast<uint8_t *>(this + 1); }

    static constexpr uint32_t fixedSize() { return sizeof(Entry); }
};

template<ChannelType eChannelType>
class IChannel
{
protected:
    virtual ~IChannel() noexcept = default;

public:
    /**
     * @brief 创建一个通道
     * @param stConfig 通道配置
     * @return 成功返回通道指针，失败返回nullptr
     * @note 多线程安全
     */
    static IChannel *Create(const ChannelConfig *pConfig) noexcept;

    /**
     * @brief 销毁一个通道
     * @param pChannel 通道指针
     * @note 多线程安全
     */
    static void Destroy(IChannel *pChannel) noexcept;

    /**
     * @brief 创建一个元素
     * @return 成功返回元素指针，失败返回nullptr
     * @note 多线程安全
     */
    Entry *NewEntry() noexcept;

    /**
     * @brief 创建一个元素
     * @param uElementSize 元素大小
     * @return 成功返回元素指针，失败返回nullptr
     * @note 多线程安全
     */
    Entry *NewEntry(uint32_t uElementSize) noexcept;

    /**
     * @brief 发布一个元素
     * @param pEntry 元素指针
     * @note 多线程安全
     */
    void PostEntry(Entry *pEntry) noexcept;

    /**
     * @brief 获取一个元素
     * @return 成功返回元素指针，失败返回nullptr
     * @note 多线程安全
     */
    Entry *GetEntry() noexcept;

    /**
     * @brief 释放一个元素
     * @param pEntry 元素指针
     * @note 多线程安全
     */
    void DeleteEntry(Entry *pEntry) noexcept;

    /**
     * @brief 判断通道是否为空
     * @return 为空返回true，否则返回false
     * @note 多线程安全
     */
    bool IsEmpty() const noexcept;

    /**
     * @brief 获取通道当前元素个数
     * @return 通道当前元素个数
     * @note 多线程安全
     */
    uint32_t GetSize() const noexcept;

    /**
     * @brief 获取通道统计信息
     * @param pStats 统计信息对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    int32_t GetStats(QueueStats *pStats) const noexcept;
};

extern template class EXPORT IChannel<ChannelType::kSPSC>;
extern template class EXPORT IChannel<ChannelType::kSPMC>;
extern template class EXPORT IChannel<ChannelType::kMPSC>;
extern template class EXPORT IChannel<ChannelType::kMPMC>;

} // namespace channel
} // namespace base
} // namespace cppx

#endif // __CPPX_CHANNEL_H__
