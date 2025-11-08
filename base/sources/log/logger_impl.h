#ifndef __CPPX_LOGGER_IMPL_H__
#define __CPPX_LOGGER_IMPL_H__

#include <cstdint>
#include <logger/logger.h>
#include <thread/thread_manager.h>
#include <channel/channel.h>
#include <cstdio>
#include <condition_variable>
#include <string>

namespace cppx
{
namespace base
{

class CLoggerImpl final : public ILogger
{
public:
    enum class LogItemType : uint8_t
    {
        kLog,
        kLogFormat,
    };

    struct LogItemHeader
    {
        LogItemType eType;
    };

    struct LogForamtItem
    {
        LogItemHeader header;
        uint32_t uWriteLen;
        const char *pLogBuffer;
    };

    struct LogItem
    {
        LogItemHeader header;
        int32_t iErrorNo;
        LogLevel eLevel;
        uint32_t uTid;
        uint32_t uParamCount;
        uint64_t uTimestampNs;
        const char *pModule;
        const char *pFileLine;
        const char *pFunction;
        const char *pFormat;
        const char **ppParams;

        bool CopyParams(const char **ppParams, uint32_t uParamCount) noexcept;
    };

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
    static bool RunWrapper(void *pArg);
    void Run();

    int32_t WriteLog(LogItem &logItem);
    int32_t WriteLog(const char *pLogBuffer, uint32_t uWriteLen);

    void CheckFileSwitch();
    int32_t OpenLogFile();

private:
    bool m_bAsync {false};
    uint32_t m_uPid {UINT32_MAX};
    static thread_local uint32_t m_uTid;
    uint32_t m_uLogFormatBufferSize {default_value::kLogFormatBufferSize};

    std::mutex m_lock;
    std::condition_variable m_condition;
    channel::IChannel *m_pChannel {nullptr};
    uint32_t m_uLogChannelMaxCount {default_value::kLogChannelMaxCount};

    bool m_bRunning {false};
    IThreadManager *m_pThreadManager {nullptr};
    IThread *m_pThread {nullptr};
    uint32_t m_uBindCpuNo {UINT32_MAX};


    FILE *m_pLogFile {nullptr};
    char m_szLogFileName[256];
    uint64_t m_uLogFileSize {0};
    uint64_t m_uLogFileMaxSizeMB {default_value::kLogFileMaxSizeMB};
    uint64_t m_uLogTotalSizeMB {default_value::kLogTotalSizeMB};

    uint64_t m_uLastCheckTimeNs {0};

    std::string m_strLoggerName;
    std::string m_strLogPath;
    std::string m_strLogPrefix;
    std::string m_strLogSuffix;

};

}
}
#endif // __CPPX_LOGGER_IMPL_H__