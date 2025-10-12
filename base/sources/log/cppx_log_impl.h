#ifndef __CPPX_LOG_IMPL_H__
#define __CPPX_LOG_IMPL_H__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <tuple>

#include <log/cppx_log.h>

namespace cppx
{
namespace base
{

class CLogImpl final : public ILog
{
public:
    CLogImpl() noexcept = default;
    CLogImpl(const CLogImpl &) = delete;
    CLogImpl &operator=(const CLogImpl &) = delete;
    CLogImpl(CLogImpl &&) = delete;
    CLogImpl &operator=(CLogImpl &&) = delete;

    ~CLogImpl() noexcept override;

    int32_t Init(IJson *pConfig) noexcept override;
    int32_t Start() noexcept override;
    void Stop() noexcept override;
    int32_t Log(int32_t iErrorNo, LogLevel eLevel, const char *pFormat, ...) noexcept override;
    LogLevel GetLogLevel() const noexcept override;
    void SetLogLevel(LogLevel eLevel) noexcept override;
    int32_t GetStats(IJson *pJsonStats) const noexcept override;

private:
};

}
}
#endif // __CPPX_LOG_IMPL_H__