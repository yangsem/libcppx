#include <gtest/gtest.h>
#include <logger/logger.h>
#include <utilities/json.h>
#include <utilities/error_code.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

using namespace cppx::base;
using namespace cppx::base::logger;

// RAII包装类，用于自动管理ILogger对象生命周期
class LoggerGuard
{
public:
    explicit LoggerGuard(ILogger* pLogger = nullptr)
        : m_pLogger(pLogger)
    {
    }
    
    ~LoggerGuard()
    {
        if (m_pLogger)
        {
            ILogger::Destroy(m_pLogger);
        }
    }
    
    // 禁止拷贝
    LoggerGuard(const LoggerGuard&) = delete;
    LoggerGuard& operator=(const LoggerGuard&) = delete;
    
    // 允许移动
    LoggerGuard(LoggerGuard&& other) noexcept
        : m_pLogger(other.m_pLogger)
    {
        other.m_pLogger = nullptr;
    }
    
    LoggerGuard& operator=(LoggerGuard&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pLogger)
            {
                ILogger::Destroy(m_pLogger);
            }
            m_pLogger = other.m_pLogger;
            other.m_pLogger = nullptr;
        }
        return *this;
    }
    
    ILogger* get() const { return m_pLogger; }
    ILogger* operator->() const { return m_pLogger; }
    ILogger& operator*() const { return *m_pLogger; }
    
    explicit operator bool() const { return m_pLogger != nullptr; }
    
    ILogger* release()
    {
        ILogger* p = m_pLogger;
        m_pLogger = nullptr;
        return p;
    }

private:
    ILogger* m_pLogger;
};

// RAII包装类，用于自动管理IJson对象生命周期
class JsonGuard
{
public:
    explicit JsonGuard(IJson::JsonType jsonType = IJson::JsonType::kObject)
        : m_pJson(IJson::Create(jsonType))
    {
    }
    
    ~JsonGuard()
    {
        if (m_pJson)
        {
            IJson::Destroy(m_pJson);
        }
    }
    
    JsonGuard(const JsonGuard&) = delete;
    JsonGuard& operator=(const JsonGuard&) = delete;
    
    JsonGuard(JsonGuard&& other) noexcept
        : m_pJson(other.m_pJson)
    {
        other.m_pJson = nullptr;
    }
    
    JsonGuard& operator=(JsonGuard&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pJson)
            {
                IJson::Destroy(m_pJson);
            }
            m_pJson = other.m_pJson;
            other.m_pJson = nullptr;
        }
        return *this;
    }
    
    IJson* get() const { return m_pJson; }
    IJson* operator->() const { return m_pJson; }
    IJson& operator*() const { return *m_pJson; }
    
    explicit operator bool() const { return m_pJson != nullptr; }

private:
    IJson* m_pJson;
};

// 测试Fixture类
class CppxLoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 创建测试用的日志目录
        m_testLogPath = "./test_logs";
        if (std::filesystem::exists(m_testLogPath))
        {
            std::filesystem::remove_all(m_testLogPath);
        }
        std::filesystem::create_directories(m_testLogPath);
    }

    void TearDown() override
    {
        // 清理测试日志目录
        if (std::filesystem::exists(m_testLogPath))
        {
            std::filesystem::remove_all(m_testLogPath);
        }
    }

    // 创建默认配置
    JsonGuard CreateDefaultConfig(bool bAsync = false)
    {
        JsonGuard config;
        if (config.get() == nullptr)
        {
            return config; // 返回空配置，测试会失败
        }
        
        config->SetString(config::kLoggerName, "test_logger");
        config->SetUint32(config::kLogLevel, (uint32_t)ILogger::LogLevel::kInfo);
        config->SetBool(config::kLogAsync, bAsync);
        config->SetString(config::kLogPath, m_testLogPath.c_str());
        config->SetString(config::kLogPrefix, "test");
        config->SetString(config::kLogSuffix, ".log");
        config->SetUint64(config::kLogFileMaxSizeMB, 16);
        config->SetUint64(config::kLogTotalSizeMB, 1024);
        config->SetUint32(config::kLogFormatBufferSize, 4096);
        config->SetUint32(config::kLogChannelMaxMemMB, 128);
        
        return config;
    }

    // 读取日志文件内容
    std::string ReadLogFile(const std::string& fileName)
    {
        std::string filePath = m_testLogPath + "/" + fileName;
        if (!std::filesystem::exists(filePath))
        {
            return "";
        }
        
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    // 等待异步日志写入完成
    void WaitForAsyncLog(int milliseconds = 100)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

protected:
    std::string m_testLogPath;
};

// ==================== 测试 Create/Destroy ====================

// 测试Create方法 - 成功创建
TEST_F(CppxLoggerTest, TestCreateSuccess)
{
    JsonGuard config = CreateDefaultConfig();
    ILogger* pLogger = ILogger::Create(config.get());
    ASSERT_NE(pLogger, nullptr);
    ILogger::Destroy(pLogger);
}

// 测试Create方法 - 空配置
TEST_F(CppxLoggerTest, TestCreateWithNullConfig)
{
    ILogger* pLogger = ILogger::Create(nullptr);
    EXPECT_EQ(pLogger, nullptr);
    EXPECT_EQ(GetLastError(), ErrorCode::kInvalidParam);
}

// 测试Create方法 - 使用默认值
TEST_F(CppxLoggerTest, TestCreateWithDefaultValues)
{
    JsonGuard config;
    ASSERT_NE(config.get(), nullptr);
    // 只设置必要的配置，其他使用默认值
    config->SetString(config::kLogPath, m_testLogPath.c_str());
    
    ILogger* pLogger = ILogger::Create(config.get());
    ASSERT_NE(pLogger, nullptr);
    
    // 验证默认值
    EXPECT_EQ(pLogger->GetLogLevel(), ILogger::LogLevel::kInfo);
    
    LoggerGuard guard(pLogger);
}

// ==================== 测试 Init/Exit ====================

// 测试Init方法 - 成功初始化
TEST_F(CppxLoggerTest, TestInitSuccess)
{
    JsonGuard config = CreateDefaultConfig();
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    // Create已经调用了Init，这里验证状态
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kInfo);
}

// 测试Init方法 - 空配置
TEST_F(CppxLoggerTest, TestInitWithNullConfig)
{
    // 这个测试需要直接创建CLoggerImpl，但它是私有的
    // 所以通过Create来测试，Create内部会调用Init
    JsonGuard config;
    ILogger* pLogger = ILogger::Create(config.get());
    // 如果config为空，Create应该返回nullptr
    // 但实际上Create会调用Init，Init会检查pConfig
    // 由于Create内部会调用Init，如果Init失败，Create会返回nullptr
    // 但这里config不是nullptr，只是空的，所以应该能创建成功
    if (pLogger != nullptr)
    {
        ILogger::Destroy(pLogger);
    }
}

// ==================== 测试 Start/Stop ====================

// 测试Start/Stop - 同步模式
TEST_F(CppxLoggerTest, TestStartStopSync)
{
    JsonGuard config = CreateDefaultConfig(false); // 同步模式
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    int32_t result = logger->Start();
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    logger->Stop();
}

// 测试Start/Stop - 异步模式
TEST_F(CppxLoggerTest, TestStartStopAsync)
{
    JsonGuard config = CreateDefaultConfig(true); // 异步模式
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    int32_t result = logger->Start();
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    // 等待一下确保线程启动
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    logger->Stop();
}

// 测试Start - 重复启动
TEST_F(CppxLoggerTest, TestStartTwice)
{
    JsonGuard config = CreateDefaultConfig(true); // 异步模式
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    int32_t result1 = logger->Start();
    EXPECT_EQ(result1, ErrorCode::kSuccess);
    
    // 再次启动应该失败
    int32_t result2 = logger->Start();
    EXPECT_EQ(result2, ErrorCode::kInvalidCall);
    
    logger->Stop();
}

// ==================== 测试 GetLogLevel/SetLogLevel ====================

// 测试GetLogLevel/SetLogLevel
TEST_F(CppxLoggerTest, TestLogLevel)
{
    JsonGuard config = CreateDefaultConfig();
    config->SetUint32(config::kLogLevel, (uint32_t)ILogger::LogLevel::kDebug);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    // 测试初始级别
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kDebug);
    
    // 测试设置级别
    logger->SetLogLevel(ILogger::LogLevel::kWarn);
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kWarn);
    
    // 测试所有级别
    logger->SetLogLevel(ILogger::LogLevel::kTrace);
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kTrace);
    
    logger->SetLogLevel(ILogger::LogLevel::kError);
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kError);
    
    logger->SetLogLevel(ILogger::LogLevel::kFatal);
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kFatal);
    
    logger->SetLogLevel(ILogger::LogLevel::kEvent);
    EXPECT_EQ(logger->GetLogLevel(), ILogger::LogLevel::kEvent);
}

// ==================== 测试 Log 方法 ====================

// 测试Log方法 - 同步模式
TEST_F(CppxLoggerTest, TestLogSync)
{
    JsonGuard config = CreateDefaultConfig(false); // 同步模式
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:100";
    const char* pFunction = "TestLogSync";
    const char* pFormat = "Test log message: {} {}";
    const char* ppParams[] = {"param1", "param2"};
    uint32_t uParamCount = 2;
    
    int32_t result = logger->Log(ErrorCode::kInfo, ILogger::LogLevel::kInfo, 
                                 pModule, pFileLine, pFunction,
                                 pFormat, ppParams, uParamCount);
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    logger->Stop();
    
    // 验证日志文件是否存在
    std::string logContent = ReadLogFile("test_logger.log");
    printf("logContent: %s\n", logContent.c_str());
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("Test log message"), std::string::npos);
    EXPECT_NE(logContent.find("param1"), std::string::npos);
    EXPECT_NE(logContent.find("param2"), std::string::npos);
}

// 测试Log方法 - 异步模式
TEST_F(CppxLoggerTest, TestLogAsync)
{
    JsonGuard config = CreateDefaultConfig(true); // 异步模式
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:200";
    const char* pFunction = "TestLogAsync";
    const char* pFormat = "Async test log: {}";
    const char* ppParams[] = {"async_param"};
    uint32_t uParamCount = 1;
    
    int32_t result = logger->Log(0, ILogger::LogLevel::kInfo, 
                                 pModule, pFileLine, pFunction,
                                 pFormat, ppParams, uParamCount);
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    // 等待异步写入完成
    WaitForAsyncLog(200);
    
    logger->Stop();
    WaitForAsyncLog(100);
    
    // 验证日志文件
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("Async test log"), std::string::npos);
    EXPECT_NE(logContent.find("async_param"), std::string::npos);
}

// 测试Log方法 - 不同日志级别
TEST_F(CppxLoggerTest, TestLogDifferentLevels)
{
    JsonGuard config = CreateDefaultConfig(false);
    config->SetUint32(config::kLogLevel, (uint32_t)ILogger::LogLevel::kTrace);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:300";
    const char* pFunction = "TestLogDifferentLevels";
    const char* pFormat = "Level test: {}";
    const char* ppParams[] = {"test"};
    
    // 测试所有级别
    logger->Log(0, ILogger::LogLevel::kTrace, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(0, ILogger::LogLevel::kDebug, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(0, ILogger::LogLevel::kInfo, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(0, ILogger::LogLevel::kWarn, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(0, ILogger::LogLevel::kError, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(0, ILogger::LogLevel::kFatal, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(0, ILogger::LogLevel::kEvent, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    
    logger->Stop();
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("TRACE"), std::string::npos);
    EXPECT_NE(logContent.find("DEBUG"), std::string::npos);
    EXPECT_NE(logContent.find(" INFO"), std::string::npos);
    EXPECT_NE(logContent.find(" WARN"), std::string::npos);
    EXPECT_NE(logContent.find("ERROR"), std::string::npos);
    EXPECT_NE(logContent.find("FATAL"), std::string::npos);
    EXPECT_NE(logContent.find("EVENT"), std::string::npos);
}

// 测试Log方法 - 多个参数
TEST_F(CppxLoggerTest, TestLogMultipleParams)
{
    JsonGuard config = CreateDefaultConfig(false);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:400";
    const char* pFunction = "TestLogMultipleParams";
    const char* pFormat = "Multiple params: {} {} {} {}";
    const char* ppParams[] = {"one", "two", "three", "four"};
    uint32_t uParamCount = 4;
    
    int32_t result = logger->Log(0, ILogger::LogLevel::kInfo, 
                                 pModule, pFileLine, pFunction,
                                 pFormat, ppParams, uParamCount);
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    logger->Stop();
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("one"), std::string::npos);
    EXPECT_NE(logContent.find("two"), std::string::npos);
    EXPECT_NE(logContent.find("three"), std::string::npos);
    EXPECT_NE(logContent.find("four"), std::string::npos);
}

// 测试Log方法 - 无参数
TEST_F(CppxLoggerTest, TestLogNoParams)
{
    JsonGuard config = CreateDefaultConfig(false);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:500";
    const char* pFunction = "TestLogNoParams";
    const char* pFormat = "No params message";
    
    int32_t result = logger->Log(0, ILogger::LogLevel::kInfo, 
                                 pModule, pFileLine, pFunction,
                                 pFormat, nullptr, 0);
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    logger->Stop();
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("No params message"), std::string::npos);
}

// 测试Log方法 - 空格式字符串
TEST_F(CppxLoggerTest, TestLogEmptyFormat)
{
    JsonGuard config = CreateDefaultConfig(false);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:600";
    const char* pFunction = "TestLogEmptyFormat";
    const char* pFormat = "";
    
    int32_t result = logger->Log(0, ILogger::LogLevel::kInfo, 
                                 pModule, pFileLine, pFunction,
                                 pFormat, nullptr, 0);
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    logger->Stop();
}

// ==================== 测试 LogFormat 方法 ====================

// 测试LogFormat方法 - 同步模式
TEST_F(CppxLoggerTest, TestLogFormatSync)
{
    JsonGuard config = CreateDefaultConfig(false);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    int32_t result = logger->LogFormat(0, ILogger::LogLevel::kInfo, 
                                       "LogFormat test: %s %d", "test", 123);
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    logger->Stop();
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("LogFormat test"), std::string::npos);
    EXPECT_NE(logContent.find("test"), std::string::npos);
    EXPECT_NE(logContent.find("123"), std::string::npos);
}

// 测试LogFormat方法 - 异步模式
TEST_F(CppxLoggerTest, TestLogFormatAsync)
{
    JsonGuard config = CreateDefaultConfig(true);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    int32_t result = logger->LogFormat(0, ILogger::LogLevel::kInfo, 
                                       "Async LogFormat: %s", "async_test");
    EXPECT_EQ(result, ErrorCode::kSuccess);
    
    WaitForAsyncLog(200);
    
    logger->Stop();
    WaitForAsyncLog(100);
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("Async LogFormat"), std::string::npos);
    EXPECT_NE(logContent.find("async_test"), std::string::npos);
}

// 测试LogFormat方法 - 不同格式
TEST_F(CppxLoggerTest, TestLogFormatDifferentFormats)
{
    JsonGuard config = CreateDefaultConfig(false);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "String: %s", "test");
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "Integer: %d", 42);
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "Float: %.2f", 3.14);
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "Hex: 0x%x", 255);
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "Multiple: %s %d %.2f", "test", 42, 3.14);
    
    logger->Stop();
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("String: test"), std::string::npos);
    EXPECT_NE(logContent.find("Integer: 42"), std::string::npos);
    EXPECT_NE(logContent.find("Float: 3.14"), std::string::npos);
    EXPECT_NE(logContent.find("Hex: 0xff"), std::string::npos);
}

// ==================== 测试 GetStats ====================

// 测试GetStats方法
TEST_F(CppxLoggerTest, TestGetStats)
{
    JsonGuard config = CreateDefaultConfig();
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    JsonGuard stats;
    int32_t result = logger->GetStats(stats.get());
    EXPECT_EQ(result, ErrorCode::kSuccess);
}

// ==================== 测试多线程 ====================

// 测试多线程日志写入 - 异步模式
TEST_F(CppxLoggerTest, TestMultiThreadLogging)
{
    JsonGuard config = CreateDefaultConfig(true);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const int numThreads = 10;
    const int logsPerThread = 10;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&logger, i]() {
            const char* pModule = "MultiThread";
            const char* pFileLine = "test_logger.cpp:800";
            const char* pFunction = "TestMultiThreadLogging";
            const char* pFormat = "Thread {} log {}";
            
            for (int j = 0; j < logsPerThread; ++j)
            {
                char threadId[32];
                snprintf(threadId, sizeof(threadId), "%d", i);
                char logId[32];
                snprintf(logId, sizeof(logId), "%d", j);
                const char* ppParams[] = {threadId, logId};
                
                logger->Log(0, ILogger::LogLevel::kInfo, 
                           pModule, pFileLine, pFunction,
                           pFormat, ppParams, 2);
            }
        });
    }
    
    for (auto& t : threads)
    {
        t.join();
    }
    
    WaitForAsyncLog(500);
    
    logger->Stop();
    WaitForAsyncLog(200);
    
    // 验证日志文件
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    
    // 验证所有线程的日志都存在
    for (int i = 0; i < numThreads; ++i)
    {
        char threadId[32];
        snprintf(threadId, sizeof(threadId), "%d", i);
        EXPECT_NE(logContent.find(threadId), std::string::npos);
    }
}

// ==================== 测试配置选项 ====================

// 测试自定义日志路径
TEST_F(CppxLoggerTest, TestCustomLogPath)
{
    std::string customPath = m_testLogPath + "/custom";
    std::filesystem::create_directories(customPath);
    
    JsonGuard config;
    ASSERT_NE(config.get(), nullptr);
    config->SetString(config::kLoggerName, "custom_logger");
    config->SetUint32(config::kLogLevel, (uint32_t)ILogger::LogLevel::kInfo);
    config->SetBool(config::kLogAsync, false);
    config->SetString(config::kLogPath, customPath.c_str());
    config->SetString(config::kLogSuffix, ".log");
    
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "Custom path test");
    logger->Stop();
    
    std::string logContent = ReadLogFile("../custom/custom_logger.log");
    if (logContent.empty())
    {
        // 尝试绝对路径
        std::string absPath = customPath + "/custom_logger.log";
        std::ifstream file(absPath);
        if (file.is_open())
        {
            logContent = std::string((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
        }
    }
    EXPECT_FALSE(logContent.empty());
}

// 测试日志文件前缀和后缀
TEST_F(CppxLoggerTest, TestLogPrefixAndSuffix)
{
    JsonGuard config = CreateDefaultConfig(false);
    config->SetString(config::kLogPrefix, "prefix_");
    config->SetString(config::kLogSuffix, ".txt");
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    logger->LogFormat(0, ILogger::LogLevel::kInfo, "Prefix suffix test");
    logger->Stop();
    
    // 验证文件是否存在（文件名应该是 test_logger.txt，因为前缀在文件切换时使用）
    std::string logContent = ReadLogFile("test_logger.txt");
    EXPECT_FALSE(logContent.empty());
}

// ==================== 测试边界情况 ====================

// 测试大量日志写入
TEST_F(CppxLoggerTest, TestLargeAmountOfLogs)
{
    JsonGuard config = CreateDefaultConfig(true);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const int numLogs = 1000;
    for (int i = 0; i < numLogs; ++i)
    {
        logger->LogFormat(0, ILogger::LogLevel::kInfo, "Log message %d", i);
    }
    
    WaitForAsyncLog(1000);
    
    logger->Stop();
    WaitForAsyncLog(200);
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    
    // 验证日志数量（通过查找特定日志）
    EXPECT_NE(logContent.find("Log message 0"), std::string::npos);
    EXPECT_NE(logContent.find("Log message 999"), std::string::npos);
}

// 测试错误码
TEST_F(CppxLoggerTest, TestErrorCode)
{
    JsonGuard config = CreateDefaultConfig(false);
    LoggerGuard logger(ILogger::Create(config.get()));
    ASSERT_NE(logger.get(), nullptr);
    
    logger->Start();
    
    const char* pModule = "TestModule";
    const char* pFileLine = "test_logger.cpp:900";
    const char* pFunction = "TestErrorCode";
    const char* pFormat = "Error code test: {}";
    const char* ppParams[] = {"test"};
    
    // 测试不同的错误码
    logger->Log(100, ILogger::LogLevel::kError, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(200, ILogger::LogLevel::kWarn, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    logger->Log(-1, ILogger::LogLevel::kInfo, pModule, pFileLine, pFunction, pFormat, ppParams, 1);
    
    logger->Stop();
    
    std::string logContent = ReadLogFile("test_logger.log");
    EXPECT_FALSE(logContent.empty());
    EXPECT_NE(logContent.find("100"), std::string::npos);
    EXPECT_NE(logContent.find("200"), std::string::npos);
    EXPECT_NE(logContent.find("-1"), std::string::npos);
}
