#include "logger_impl.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <map>
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <memory/allocator_ex.h>
#include <utilities/time.h>

namespace cppx
{
namespace base
{

thread_local uint32_t CLoggerImpl::m_uTid = UINT32_MAX;

static const char *LogLevelToString(ILogger::LogLevel eLevel)
{
    static const char *szLogLevel[] = {
        "TRACE",
        "DEBUG",
        " INFO",
        " WARN",
        "ERROR",
        "FATAL",
        "EVENT",
        " NULL"
    };

    uint32_t uIndex = 0;
    if (eLevel >= ILogger::LogLevel::kTrace && eLevel < ILogger::LogLevel::kEvent)
    {
        uIndex = (uint32_t)eLevel;
    }
    else
    {
        uIndex = sizeof(szLogLevel) / sizeof(szLogLevel[0]) - 1;
    }

    return szLogLevel[uIndex];
}

ILogger *ILogger::Create(IJson *pConfig) noexcept
{
    auto pLogger = AllocatorEx::GetInstance()->New<CLoggerImpl>();
    if (pLogger == nullptr)
    {
        PRINT_ERROR("new logger failed, out of memory");
        SetLastError(ErrorCode::kOutOfMemory);
        return nullptr;
    }

    auto iErrorNo = pLogger->Init(pConfig);
    if (iErrorNo != ErrorCode::kSuccess)
    {
        PRINT_ERROR("init logger failed, error code: %d", iErrorNo);
        AllocatorEx::GetInstance()->Delete(pLogger);
        SetLastError((ErrorCode)iErrorNo);
        return nullptr;
    }

    return pLogger;
}

void ILogger::Destroy(ILogger *pLogger) noexcept
{
    if (pLogger != nullptr)
    {
        AllocatorEx::GetInstance()->Delete(pLogger);
    }
}

int32_t CLoggerImpl::Init(IJson *pConfig) noexcept
{
    if (pConfig == nullptr)
    {
        PRINT_ERROR("init logger failed, invalid param");
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    auto uLogLevel = pConfig->GetUint32(config::kLogLevel, default_value::kLogLevel);
    if (uLogLevel > (uint32_t)LogLevel::kEvent)
    {
        uLogLevel = default_value::kLogLevel;
    }
    m_eLogLevel = (LogLevel)uLogLevel;
    m_uPid = getpid();
    m_bAsync = pConfig->GetBool(config::kLogAsync, default_value::kLogAsync);
    m_uBindCpuNo = pConfig->GetUint32(config::kBindCpuNo, default_value::kBindCpuNo);
    m_uLogFileMaxSizeMB = pConfig->GetUint64(config::kLogFileMaxSizeMB, default_value::kLogFileMaxSizeMB);
    m_uLogTotalSizeMB = pConfig->GetUint64(config::kLogTotalSizeMB, default_value::kLogTotalSizeMB);
    m_uLogFormatBufferSize = pConfig->GetUint32(config::kLogFormatBufferSize, default_value::kLogFormatBufferSize);
    m_uLogChannelMaxCount = pConfig->GetUint32(config::kLogChannelMaxCount, default_value::kLogChannelMaxCount);

    try
    {
        m_strLoggerName = pConfig->GetString(config::kLoggerName, default_value::kLoggerName);
        m_strLogPath = pConfig->GetString(config::kLogPath, default_value::kLogPath);
        m_strLogPrefix = pConfig->GetString(config::kLogPrefix, default_value::kLogPrefix);
        m_strLogSuffix = pConfig->GetString(config::kLogSuffix, default_value::kLogSuffix);
    }
    catch(std::exception &e)
    {
        PRINT_ERROR("init logger failed, throw exception: %s", e.what());
        SetLastError(ErrorCode::kThrowException);
        return ErrorCode::kThrowException;
    }

    if (m_bAsync)
    {
        channel::ChannelConfig stConfig;
        stConfig.eChannelType = channel::ChannelType::kMPSC;
        stConfig.eElementType = channel::ElementType::kVariableSize;
        stConfig.eLengthType = channel::LengthType::kBounded;
        stConfig.uMaxElementCount = m_uLogChannelMaxCount;
        m_pChannel = channel::IChannel::Create(&stConfig);
        if (m_pChannel == nullptr)
        {
            PRINT_ERROR("init logger failed, create channel failed");
            SetLastError(ErrorCode::kSystemError);
            return ErrorCode::kSystemError;
        }

        m_pThreadManager = IThreadManager::GetInstance();
        if (m_pThreadManager == nullptr)
        {
            PRINT_ERROR("init logger failed, get thread manager failed");
            SetLastError(ErrorCode::kSystemError);
            return ErrorCode::kSystemError;
        }

        char szThreadName[16];
        snprintf(szThreadName, sizeof(szThreadName), "logger_%s", m_strLoggerName.c_str());
        m_pThread = m_pThreadManager->CreateThread();
        if (m_pThread == nullptr 
            || m_pThread->Bind(szThreadName, &CLoggerImpl::RunWrapper, this) != ErrorCode::kSuccess)
        {
            PRINT_ERROR("init logger failed, create thread failed");
            SetLastError(ErrorCode::kSystemError);
            return ErrorCode::kSystemError;
        }

        if (m_uBindCpuNo != UINT32_MAX)
        {
            if (m_pThread->BindCpu(m_uBindCpuNo) != ErrorCode::kSuccess)
            {
                PRINT_ERROR("init logger failed, bind cpu failed");
                SetLastError(ErrorCode::kSystemError);
                return ErrorCode::kSystemError;
            }
        }
    }

    return ErrorCode::kSuccess;
}

void CLoggerImpl::Exit() noexcept
{
    Stop();

    if (m_bAsync)
    {
        if (m_pThreadManager != nullptr && m_pThread != nullptr)
        {
            m_pThreadManager->DestroyThread(m_pThread);
            m_pThread = nullptr;
        }
        m_pThreadManager = nullptr;
    }
}

int32_t CLoggerImpl::Start() noexcept
{
    if (m_bAsync)
    {
        if (m_bRunning)
        {
            PRINT_ERROR("start logger failed, already running");
            SetLastError(ErrorCode::kInvalidCall);
            return ErrorCode::kInvalidCall;
        }

        if (m_pThread == nullptr || m_pThread->Start() != ErrorCode::kSuccess)
        {
            PRINT_ERROR("start logger failed, start thread failed");
            SetLastError(ErrorCode::kSystemError);
            return ErrorCode::kSystemError;
        }
        m_bRunning = true;
    }

    return ErrorCode::kSuccess;
}

void CLoggerImpl::Stop() noexcept
{
    if (m_bAsync)
    {
        if (m_bRunning && m_pThread != nullptr)
        {
            m_pThread->Stop();
            m_bRunning = false;
        }
    }
}

int32_t CLoggerImpl::Log(int32_t iErrorNo, LogLevel eLevel, const char *pModule,
                        const char *pFileLine, const char *pFunction,
                        const char *pFormat, const char **ppParams, uint32_t uParamCount) noexcept
{
    if (unlikely(m_uTid == UINT32_MAX))
    {
        m_uTid = gettid();
    }
    if (likely(m_bAsync))
    {
        auto pEntry = m_pChannel->NewEntry(sizeof(LogItem));
        if (unlikely(pEntry == nullptr))
        {
            SetLastError(ErrorCode::kOutOfMemory);
            return ErrorCode::kOutOfMemory;
        }
        auto pLogItem = reinterpret_cast<LogItem *>(pEntry->pData);
        pLogItem->header.eType = LogItemType::kLog;
        pLogItem->iErrorNo = iErrorNo;
        pLogItem->eLevel = eLevel;
        pLogItem->uTid = m_uTid;
        pLogItem->uParamCount = uParamCount;
        clock_get_time_nano(pLogItem->uTimestampNs);
        pLogItem->pModule = pModule;
        pLogItem->pFileLine = pFileLine;
        pLogItem->pFunction = pFunction;
        pLogItem->pFormat = pFormat;
        if (pLogItem->CopyParams(ppParams, uParamCount))
        {
            // todo
            pLogItem->ppParams = nullptr;
            pLogItem->uParamCount = 0;
        }
        
        m_pChannel->PostEntry(pEntry);
        if (m_pChannel->GetSize() == 1)
        {
            m_condition.notify_one();
        }
        return ErrorCode::kSuccess;
    }
    else
    {
        LogItem logItem;
        logItem.header.eType = LogItemType::kLog;
        logItem.iErrorNo = iErrorNo;
        logItem.eLevel = eLevel;
        logItem.uTid = m_uTid;
        logItem.uParamCount = uParamCount;
        clock_get_time_nano(logItem.uTimestampNs);
        logItem.pModule = pModule;
        logItem.pFileLine = pFileLine;
        logItem.pFunction = pFunction;
        logItem.pFormat = pFormat;
        logItem.ppParams = ppParams;
        return WriteLog(logItem);
    }
}

int32_t CLoggerImpl::LogFormat(int32_t iErrorNo, LogLevel eLevel, const char *pFormat, ...) noexcept
{
    if (unlikely(m_uTid == UINT32_MAX))
    {
        m_uTid = gettid();
    }

    auto pLogBuffer = (char *)IAllocator::GetInstance()->New(m_uLogFormatBufferSize);
    if (unlikely(pLogBuffer == nullptr))
    {
        SetLastError(ErrorCode::kOutOfMemory);
        return ErrorCode::kOutOfMemory;
    }

    auto time = ITime::GetLocalTime();
    // YYYYMMDD-HHMMSS.uuuuuu PID TID ERROR_CODE LEVEL 
    auto iLen = snprintf((char *)pLogBuffer, m_uLogFormatBufferSize, 
             "%04d%02d%02d-%02d:%02d:%02d.%06d %u %u %d %s ",
             time.uYear, time.uMonth, time.uDay, time.uHour, time.uMinute, time.uSecond, time.uMicro, 
             m_uPid, m_uTid, iErrorNo, LogLevelToString(eLevel));
    if (unlikely(iLen < 0))
    {
        IAllocator::GetInstance()->Delete(pLogBuffer);
        SetLastError(ErrorCode::kSystemError);
        return ErrorCode::kSystemError;
    }

    va_list args;
    va_start(args, pFormat);
    // FORMAT
    auto iLen2 = vsnprintf((char *)pLogBuffer + iLen, m_uLogFormatBufferSize - iLen, pFormat, args);
    va_end(args);
    if (unlikely(iLen2 < 0))
    {
        IAllocator::GetInstance()->Delete(pLogBuffer);
        SetLastError(ErrorCode::kSystemError);
        return ErrorCode::kSystemError;
    }

    uint32_t uWriteLen = std::min((uint32_t)iLen + iLen2, m_uLogFormatBufferSize - 1);
    pLogBuffer[uWriteLen - 1] = '\n';
    uWriteLen++;

    if (likely(m_bAsync))
    {
        auto pEntry = m_pChannel->NewEntry(sizeof(LogForamtItem));
        if (unlikely(pEntry == nullptr))
        {
            IAllocator::GetInstance()->Delete(pLogBuffer);
            SetLastError(ErrorCode::kOutOfMemory);
            return ErrorCode::kOutOfMemory;
        }
        auto pLogForamtItem = reinterpret_cast<LogForamtItem *>(pEntry->pData);
        pLogForamtItem->header.eType = LogItemType::kLogFormat;
        pLogForamtItem->uWriteLen = uWriteLen;
        pLogForamtItem->pLogBuffer = pLogBuffer;
        m_pChannel->PostEntry(pEntry);
        if (m_pChannel->GetSize() == 1)
        {
            m_condition.notify_one();
        }
        return ErrorCode::kSuccess;
    }
    else
    {
        auto iErrorNo = WriteLog((char *)pLogBuffer, uWriteLen);
        IAllocator::GetInstance()->Delete(pLogBuffer);
        return iErrorNo;
    }
}

int32_t CLoggerImpl::GetStats(IJson *pJson) const noexcept
{
    if (pJson != nullptr)
    {
        pJson->Clear();
    }
    return ErrorCode::kSuccess;
}

bool CLoggerImpl::RunWrapper(void *pArg)
{
    auto pLogger = reinterpret_cast<CLoggerImpl *>(pArg);
    pLogger->Run();
    return true;
}

void CLoggerImpl::Run()
{
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_condition.wait_for(lock, std::chrono::microseconds(10), 
                             [this] () { return m_pChannel->GetSize() > 0; });
    }

    auto pEntry = m_pChannel->GetEntry();
    if (unlikely(pEntry != nullptr))
    {
        auto pHeader = reinterpret_cast<LogItemHeader *>(pEntry->pData);
        if (pHeader->eType == LogItemType::kLog)
        {
            auto pLogItem = reinterpret_cast<LogItem *>(pEntry->pData);
            WriteLog(*pLogItem);
        }
        else if (pHeader->eType == LogItemType::kLogFormat)
        {
            auto pLogForamtItem = reinterpret_cast<LogForamtItem *>(pEntry->pData);
            WriteLog(pLogForamtItem->pLogBuffer, pLogForamtItem->uWriteLen);
        }
    }

    CheckFileSwitch();
}

int32_t CLoggerImpl::WriteLog(LogItem &logItem)
{
    uint64_t uLogFormatBufferSize = 64; //YYYYMMDD-HHMMSS.uuuuuu PID TID ERROR_CODE LEVEL 
    uLogFormatBufferSize += strlen(logItem.pModule);
    uLogFormatBufferSize += strlen(logItem.pFileLine);
    uLogFormatBufferSize += strlen(logItem.pFunction);
    uLogFormatBufferSize += strlen(logItem.pFormat);
    for (uint32_t i = 0; i < logItem.uParamCount; ++i)
    {
        uLogFormatBufferSize += strlen(logItem.ppParams[i]);
    }

    auto pLogBuffer = (char *)IAllocator::GetInstance()->New(uLogFormatBufferSize);
    if (unlikely(pLogBuffer == nullptr))
    {
        SetLastError(ErrorCode::kOutOfMemory);
        return ErrorCode::kOutOfMemory;
    }

    uint64_t uCurrentTimeNs = 0;
    clock_get_time_nano(uCurrentTimeNs);
    auto time = ITime::GetLocalTime();
    time -= (uCurrentTimeNs - logItem.uTimestampNs) / kSecond; // 计算时间差，转换成秒，会不会不准？影响不大
    auto iLen = snprintf(pLogBuffer, uLogFormatBufferSize, 
                         "%04d%02d%02d-%02d:%02d:%02d.%06d %u %u %d %s [%s] ",
                         time.uYear, time.uMonth, time.uDay, time.uHour, time.uMinute, time.uSecond, time.uMicro, 
                         m_uPid, m_uTid, logItem.iErrorNo, LogLevelToString(logItem.eLevel), logItem.pModule);
    if (unlikely(iLen < 0))
    {
        IAllocator::GetInstance()->Delete(pLogBuffer);
        SetLastError(ErrorCode::kSystemError);
        return ErrorCode::kSystemError;
    }

    uint32_t uLogBufferIndex = iLen;
    uint32_t uParamIndex = 0;
    for (uint32_t i = 0; logItem.pFormat[i] != '\0'; ++i)
    {
        if (logItem.pFormat[i] == '{' && logItem.pFormat[i+1] == '}')
        {
            auto iParamLen = snprintf(pLogBuffer + uLogBufferIndex, uLogFormatBufferSize - uLogBufferIndex, 
                                           "%s", logItem.ppParams[uParamIndex]);
            if (unlikely(iParamLen < 0))
            {
                break;
            }
            uLogBufferIndex += (uint32_t)iParamLen;
            uParamIndex++;
            ++i;
        }
        else
        {
            pLogBuffer[uLogBufferIndex] = logItem.pFormat[i];
            uLogBufferIndex++;
        }
    }

    for (; uParamIndex < logItem.uParamCount; ++uParamIndex)
    {
        auto iParamLen = snprintf(pLogBuffer + uLogBufferIndex, uLogFormatBufferSize - uLogBufferIndex, 
                                   " %s", logItem.ppParams[uParamIndex]);
        if (unlikely(iParamLen < 0))
        {
            break;
        }
        uLogBufferIndex += (uint32_t)iParamLen;
    }

    auto iFileLineLen = snprintf(pLogBuffer + uLogBufferIndex, uLogFormatBufferSize - uLogBufferIndex, "(%s:%s)", logItem.pFileLine, logItem.pFunction);
    if (likely(iFileLineLen > 0))
    {
        uLogBufferIndex += (uint32_t)iFileLineLen;
    }

    pLogBuffer[uLogBufferIndex] = '\n';
    uLogBufferIndex++;

    return WriteLog(pLogBuffer, uLogBufferIndex);
}

int32_t CLoggerImpl::WriteLog(const char *pLogBuffer, uint32_t uWriteLen)
{
    // 如果文件为空或者文件大小超过限制，则打开新文件
    if (unlikely(m_pLogFile == nullptr || m_uLogFileSize >= m_uLogFileMaxSizeMB * 1024 * 1024))
    {
        if (unlikely(OpenLogFile() != ErrorCode::kSuccess))
        {
            PRINT_ERROR("write log failed, open log file failed");
            return ErrorCode::kSystemError;
        }
    }

    auto iRet = fwrite(pLogBuffer, 1, uWriteLen, m_pLogFile);
    if (unlikely(iRet == -1))
    {
        PRINT_ERROR("write log failed, write log file failed");
        SetLastError(ErrorCode::kSystemError);
        return ErrorCode::kSystemError;
    }
    fflush(m_pLogFile);
    m_uLogFileSize += (uint64_t)iRet;
    return ErrorCode::kSuccess;
}

int32_t CLoggerImpl::OpenLogFile()
{
    if (m_pLogFile != nullptr)
    {
        fclose(m_pLogFile);
        m_pLogFile = nullptr;
        m_uLogFileSize = 0;

        // 重命名文件
        auto time = ITime::GetLocalTime();
        char szLogFileName[256];
        snprintf(szLogFileName, sizeof(szLogFileName), "%s/%s-%04d%02d%02d-%02d%02d%02d.%s", 
                 m_strLogPath.c_str(), m_strLoggerName.c_str(), 
                 time.uYear, time.uMonth, time.uDay, 
                 time.uHour, time.uMinute, time.uSecond, 
                 m_strLogSuffix.c_str());
        if (std::filesystem::exists(m_szLogFileName))
        {
            std::filesystem::rename(m_szLogFileName, szLogFileName);
        }
    }

    snprintf(m_szLogFileName, sizeof(m_szLogFileName), "%s/%s.%s", 
             m_strLogPath.c_str(), m_strLoggerName.c_str(), m_strLogSuffix.c_str());
    // 如果文件存在，则以追加方式打开，否则创建文件
    m_pLogFile = fopen(m_szLogFileName, "a");
    if (unlikely(m_pLogFile == nullptr))
    {
        PRINT_ERROR("open log file failed, open log file failed: %s", strerror(errno));
        SetLastError(ErrorCode::kSystemError);
        return ErrorCode::kSystemError;
    }

    return ErrorCode::kSuccess;
}

void CLoggerImpl::CheckFileSwitch()
{
    if (m_uLogFileSize >= m_uLogFileMaxSizeMB * 1024 * 1024)
    {
        fclose(m_pLogFile);
        m_pLogFile = nullptr;
        m_uLogFileSize = 0;
        if (unlikely(OpenLogFile() != ErrorCode::kSuccess))
        {
            PRINT_ERROR("check file switch failed, open log file failed");
            return;
        }
    }

    uint64_t uCurrentTimeNs = 0;
    clock_get_time_nano(uCurrentTimeNs);
    if (uCurrentTimeNs - m_uLastCheckTimeNs < kSecond * 60 * 10) // 10分钟检查一次
    {
        return;
    }
    m_uLastCheckTimeNs = uCurrentTimeNs;
    std::map<std::string, uint64_t> mapFileNames;
    uint64_t uTotalSize = 0;
    for (const auto &entry : std::filesystem::directory_iterator(m_strLogPath))
    {
        auto strFileName = entry.path().filename().string();
        if (strFileName == m_szLogFileName 
            || strFileName.find_first_of(m_strLogPrefix) != 0 
            || strFileName.find_last_of(m_strLogSuffix) != strFileName.size() - m_strLogSuffix.size())
        {
            continue;
        }

        auto uFileSize = std::filesystem::file_size(entry.path());
        uTotalSize += uFileSize;
        mapFileNames.emplace(std::move(strFileName), uFileSize);
    }

    // 如果总大小超过限制，则删除最早的文件
    while (uTotalSize > m_uLogTotalSizeMB * 1024 * 1024)
    {
        // map会将文件按名称排序，因此时间越早的文件在前面
        auto it = mapFileNames.begin();
        if (it == mapFileNames.end())
        {
            break;
        }
        auto &strFileName = it->first;
        auto &uFileSize = it->second;
        uTotalSize -= uFileSize;
        mapFileNames.erase(it);
        auto strFilePath = m_strLogPath + "/" + strFileName;
        std::filesystem::remove(strFilePath);
    }
}

}
}
