#ifndef __CPPX_LOGGER_IMPL_H__
#define __CPPX_LOGGER_IMPL_H__

#include <logger/logger.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <tuple>

namespace cppx
{
namespace base
{

class CLoggerImpl final : public ILogger
{
public:
    CLoggerImpl() noexcept = default;
    CLoggerImpl(const CLoggerImpl &) = delete;
    CLoggerImpl &operator=(const CLoggerImpl &) = delete;
    CLoggerImpl(CLoggerImpl &&) = delete;
    CLoggerImpl &operator=(CLoggerImpl &&) = delete;

    ~CLoggerImpl() noexcept override;

    int32_t Init(IJson *pConfig) noexcept override;
    void Exit() noexcept override;

    int32_t Start() noexcept override;
    void Stop() noexcept override;

    int32_t Log(int32_t iErrorNo, LogLevel eLevel, const char *pModule,
                const char *pFileLine, const char *pFunction,
                const char *pFormat, const char **ppParams, uint32_t uParamCount) noexcept override;

    int32_t LogFormat(int32_t iErrorNo, LogLevel eLevel, const char *pFormat, ...) noexcept override;

    int32_t GetStats(IJson *pJson) const noexcept override;

private:
};

}
}
#endif // __CPPX_LOGGER_IMPL_H__