#ifndef __CPPX_ERROR_CODE_H__
#define __CPPX_ERROR_CODE_H__

#include <cstdint>
#include <utilities/export.h>

namespace cppx
{
namespace base
{

enum ErrorCode : int32_t {
    kSuccess = 0,          // 成功
    kEvent = 1,            // 事件
    kNotSupported = 100,   // 不支持
    kOutOfMemory = 101,    // 内存不足
    kInvalidParam = 102,   // 参数无效
    kThrowException = 103, // 抛出异常
    kInvalidCall = 104,    // 无效调用
    kSysCallFailed = 105,  // 系统调用失败
    kSystemError = 106,    // 系统错误
};

/**
 * @brief 获取当前线程的最后一次出错的错误码
 * @return 当前线程的最后一次出错的错误码
 */
EXPORT ErrorCode GetLastError() noexcept;

/**
 * @brief 设置当前线程的最后一次出错的错误码
 * @param eErrorCode 错误码
 */
EXPORT void SetLastError(ErrorCode eErrorCode) noexcept;

}
}

#endif // __CPPX_ERROR_CODE_H__
