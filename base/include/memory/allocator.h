#ifndef __CPPX_ALLOCATOR_H__
#define __CPPX_ALLOCATOR_H__

#include <utilities/export.h>
#include <utilities/json.h>

namespace cppx
{
namespace base
{

class EXPORT IAllocator
{
public:
    virtual ~IAllocator() noexcept = default;

public:
    /**
     * @brief 获取全局单例对象
     * @return 成功返回IAllocator对象指针，一定成功
     */
    static IAllocator *GetInstance() noexcept;

    /**
     * @brief 创建一个IAllocator对象
     * @return 成功返回IAllocator对象指针，失败返回nullptr
     * @note 多线程安全
     */
    static IAllocator *Create() noexcept;

    /**
     * @brief 销毁一个IAllocator对象
     * @param pAllocator IAllocator对象指针
     * @note 多线程安全
     */
    static void Destroy(IAllocator *pAllocator) noexcept;

    /**
     * @brief 初始化IAllocator对象
     * @param pConfig 配置对象
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t Init(IJson *pConfig) noexcept = 0;
    
    /**
     * @brief 清理IAllocator对象资源
     * @note 多线程安全
     */
    virtual void Exit() noexcept = 0;

    /**
     * @brief 分配内存
     * @param uSize 内存大小(字节)
     * @return 成功返回内存指针，失败返回nullptr
     * @note 多线程安全
     */
    virtual void *New(uint64_t uSize) noexcept = 0;
    
    /**
     * @brief 释放内存
     * @param pMem 内存指针
     * @note 多线程安全
     */
    virtual void Delete(void *pMem) noexcept = 0;

    /**
     * @brief 获取统计信息
     * @param pJson 统计信息对象
     * @return 成功返回0，失败返回错误码
     * @note 多线程安全
     */
    virtual int32_t GetStats(IJson *pJson) const noexcept = 0;
};

namespace config
{
constexpr const char *kAllocatorName = "allocator_name"; // 分配器名称，类型: string
constexpr const char *kAllocatorMaxMemoryMB = "allocator_max_memory_mb"; // 最大内存大小(MB)，类型: uint64_t
}

namespace default_value
{
constexpr const char *kAllocatorName = ""; // 分配器名称，默认: ""
constexpr const uint64_t kAllocatorMaxMemoryMB = 0; // 最大内存大小(MB)，默认: 不限制
}

}
}

#endif // __CPPX_ALLOCATOR_H__