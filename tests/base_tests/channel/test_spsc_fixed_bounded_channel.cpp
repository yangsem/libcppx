#include <gtest/gtest.h>
#include <channel/channel.h>
#include <channel/channel_ex.h>
#include <utilities/json.h>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <cstdio>
#include <chrono>

using namespace cppx::base::channel;
using namespace cppx::base;

// RAII包装类，用于自动管理Channel对象生命周期
class ChannelGuard
{
public:
    explicit ChannelGuard(SPSCFixedBoundedChannel* pChannel)
        : m_pChannel(pChannel)
    {
    }
    
    ~ChannelGuard()
    {
        if (m_pChannel)
        {
            SPSCFixedBoundedChannel::Destroy(m_pChannel);
        }
    }
    
    // 禁止拷贝
    ChannelGuard(const ChannelGuard&) = delete;
    ChannelGuard& operator=(const ChannelGuard&) = delete;
    
    // 允许移动
    ChannelGuard(ChannelGuard&& other) noexcept
        : m_pChannel(other.m_pChannel)
    {
        other.m_pChannel = nullptr;
    }
    
    ChannelGuard& operator=(ChannelGuard&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pChannel)
            {
                SPSCFixedBoundedChannel::Destroy(m_pChannel);
            }
            m_pChannel = other.m_pChannel;
            other.m_pChannel = nullptr;
        }
        return *this;
    }
    
    SPSCFixedBoundedChannel* get() const { return m_pChannel; }
    SPSCFixedBoundedChannel* operator->() const { return m_pChannel; }
    SPSCFixedBoundedChannel& operator*() const { return *m_pChannel; }
    
    explicit operator bool() const { return m_pChannel != nullptr; }

private:
    SPSCFixedBoundedChannel* m_pChannel;
};

class SPSCFixedBoundedChannelTest : public ::testing::Test
{
protected:
    char buffer[64];

    void SetUp() override
    {
        // 测试前的准备工作
        for (size_t i = 0; i < sizeof(buffer); ++i)
        {
            buffer[i] = 'a' + i % 26;
        }
        buffer[sizeof(buffer) - 1] = '\0';
    }

    void TearDown() override
    {
        // 测试后的清理工作
    }
};

// 测试Create和Destroy接口
TEST_F(SPSCFixedBoundedChannelTest, TestCreateAndDestroy)
{
    // 测试Create接口 - 有效配置
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0; // FixedBounded不需要这个参数
    
    SPSCFixedBoundedChannel* pChannel = SPSCFixedBoundedChannel::Create(&config);
    ASSERT_NE(pChannel, nullptr);
    // 测试Destroy接口
    SPSCFixedBoundedChannel::Destroy(pChannel);
    
    // 测试Create接口 - 空指针配置
    SPSCFixedBoundedChannel* pChannel2 = SPSCFixedBoundedChannel::Create(nullptr);
    ASSERT_EQ(pChannel2, nullptr);
    
    // 测试Create接口 - 零大小元素
    ChannelConfig config2;
    config2.uElementSize = 0;
    config2.uMaxElementCount = 1024;
    config2.uTotalMemorySizeKB = 0;
    SPSCFixedBoundedChannel* pChannel3 = SPSCFixedBoundedChannel::Create(&config2);
    ASSERT_EQ(pChannel3, nullptr);
    
    // 测试Create接口 - 零元素数量
    ChannelConfig config3;
    config3.uElementSize = 64;
    config3.uMaxElementCount = 0;
    config3.uTotalMemorySizeKB = 0;
    SPSCFixedBoundedChannel* pChannel4 = SPSCFixedBoundedChannel::Create(&config3);
    ASSERT_EQ(pChannel4, nullptr);
}

// 测试New接口（无参数版本）
TEST_F(SPSCFixedBoundedChannelTest, TestNew)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 4;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试New接口 - 正常情况
    void* pData = channel->New();
    EXPECT_NE(pData, nullptr);
    memset(pData, 0, 64);
    channel->Post(pData);

    // 测试New接口 - 多次调用
    void* pData2 = channel->New();
    EXPECT_NE(pData2, nullptr);
    memset(pData2, 0, 64);
    EXPECT_NE(pData, pData2); // 应该返回不同的指针
    channel->Post(pData2);
    
    // 测试New接口 - 在通道满时
    void* pData3 = channel->New();
    EXPECT_NE(pData3, nullptr);
    memset(pData3, 0, 64);
    EXPECT_NE(pData2, pData3); // 应该返回同一个指针
    channel->Post(pData3);

    void* pData4 = channel->New();
    EXPECT_NE(pData4, nullptr);
    memset(pData4, 0, 64);
    EXPECT_NE(pData3, pData4); // 应该返回同一个指针
    channel->Post(pData4);

    void* pData5 = channel->New();
    EXPECT_EQ(pData5, nullptr);
}

// 测试New接口（带大小参数版本）
TEST_F(SPSCFixedBoundedChannelTest, TestNewWithSize)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试New接口 - 正常大小
    void* pData = channel->New(64);
    EXPECT_EQ(pData, nullptr);
    
    // 测试New接口 - 不同大小
    void* pData2 = channel->New(32);
    EXPECT_EQ(pData2, nullptr);
    
    // 测试New接口 - 零大小
    void* pData3 = channel->New(0);
    // 根据实现，可能返回nullptr或有效指针
    EXPECT_EQ(pData3, nullptr);
}

// 测试Post接口
TEST_F(SPSCFixedBoundedChannelTest, TestPost)
{
    ChannelConfig config;
    config.uElementSize = sizeof(int);
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);

    channel->Post(nullptr);
    EXPECT_TRUE(channel->IsEmpty());
    EXPECT_EQ(channel->GetSize(), 0);
    
    // 测试Post接口 - 正常情况
    void* pData = channel->New();
    EXPECT_NE(pData, nullptr);
    // 写入一些测试数据
    int* pInt = static_cast<int*>(pData);
    *pInt = 42;
    channel->Post(pData);
    // 验证通道不为空
    EXPECT_FALSE(channel->IsEmpty());
    EXPECT_EQ(channel->GetSize(), 1);
    
    for (int i = 0; i < 10; ++i)
    {
        void* pData = channel->New();
        EXPECT_NE(pData, nullptr);
        int* pInt = static_cast<int*>(pData);
        *pInt = i;
        channel->Post(pData);
    }
    EXPECT_EQ(channel->GetSize(), 11);
}

// 测试Get接口
TEST_F(SPSCFixedBoundedChannelTest, TestGet)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试Get接口 - 空通道
    void* pData = channel->Get();
    EXPECT_EQ(pData, nullptr);
    
    // 测试Get接口 - 正常情况
    void* pNewData = channel->New();
    ASSERT_NE(pNewData, nullptr);
    memcpy(pNewData, buffer, sizeof(buffer));
    channel->Post(pNewData);
    
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    EXPECT_STREQ(buffer, static_cast<char*>(pGetData));
    channel->Delete(pGetData);
    
    // 测试Get接口 - 多个元素
    void* pNewData2 = channel->New();
    ASSERT_NE(pNewData2, nullptr);
    memcpy(pNewData2, buffer, sizeof(buffer));
    channel->Post(pNewData2);
    
    void* pGetData2 = channel->Get();
    ASSERT_NE(pGetData2, nullptr);
    EXPECT_STREQ(buffer, static_cast<char*>(pGetData2));

    void* pGetData3 = channel->Get();
    ASSERT_NE(pGetData3, nullptr);
    EXPECT_EQ(pGetData2, pGetData3);
    EXPECT_STREQ(buffer, static_cast<char*>(pGetData3));
    channel->Delete(pGetData3);
}

// 测试Delete接口
TEST_F(SPSCFixedBoundedChannelTest, TestDelete)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试Delete接口 - 正常情况
    void* pData = channel->New();
    ASSERT_NE(pData, nullptr);
    memcpy(pData, buffer, sizeof(buffer));
    channel->Post(pData);
    
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    EXPECT_STREQ(buffer, static_cast<char*>(pGetData));
    channel->Delete(pGetData);

    EXPECT_EQ(channel->GetSize(), 0);
    channel->Delete(nullptr);
    EXPECT_EQ(channel->GetSize(), 0);
}

// 测试IsEmpty接口
TEST_F(SPSCFixedBoundedChannelTest, TestIsEmpty)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试IsEmpty接口 - 空通道
    EXPECT_TRUE(channel->IsEmpty());
    
    // 测试IsEmpty接口 - 非空通道
    void* pData = channel->New();
    ASSERT_NE(pData, nullptr);
    memcpy(pData, buffer, sizeof(buffer));
    channel->Post(pData);
    EXPECT_FALSE(channel->IsEmpty());
    
    // 测试IsEmpty接口 - 获取后为空
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    channel->Delete(pGetData);
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试GetSize接口
TEST_F(SPSCFixedBoundedChannelTest, TestGetSize)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试GetSize接口 - 空通道
    EXPECT_EQ(channel->GetSize(), 0);
    
    // 测试GetSize接口 - 单个元素
    void* pData1 = channel->New();
    ASSERT_NE(pData1, nullptr);
    channel->Post(pData1);
    EXPECT_EQ(channel->GetSize(), 1);
    
    // 测试GetSize接口 - 多个元素
    void* pData2 = channel->New();
    ASSERT_NE(pData2, nullptr);
    channel->Post(pData2);
    EXPECT_EQ(channel->GetSize(), 2);
    
    void* pData3 = channel->New();
    ASSERT_NE(pData3, nullptr);
    channel->Post(pData3);
    EXPECT_EQ(channel->GetSize(), 3);
    
    // 测试GetSize接口 - 获取后减少
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    channel->Delete(pGetData);
    EXPECT_EQ(channel->GetSize(), 2);
}

// 测试GetStats接口
TEST_F(SPSCFixedBoundedChannelTest, TestGetStats)
{
    ChannelConfig config;
    config.uElementSize = 64;
    config.uMaxElementCount = 4;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试GetStats接口 - 空指针
    int32_t result = channel->GetStats(nullptr);
    EXPECT_NE(result, 0);

    channel->Post(nullptr);
    auto pData = channel->Get();
    EXPECT_EQ(pData, nullptr);
    channel->Delete(nullptr);

    // 测试GetStats接口 - 正常情况
    for (int i = 0; i < 4; ++i)
    {
        auto pData2 = channel->New();
        ASSERT_NE(pData2, nullptr);
        channel->Post(pData2);
    }
    auto pData3 = channel->New();
    ASSERT_EQ(pData3, nullptr);

    for (int i = 0; i < 4; ++i)
    {
        auto pData4 = channel->Get();
        ASSERT_NE(pData4, nullptr);
        channel->Delete(pData4);
    }

    IJson* pStats = IJson::Create();
    ASSERT_NE(pStats, nullptr);
    result = channel->GetStats(pStats);
    EXPECT_EQ(result, 0);
    // printf("pStats: %s\n", pStats->ToString());
    auto pProducerStats = pStats->GetObject("producer");
    EXPECT_EQ(pProducerStats->GetUint32("New"), 4);
    EXPECT_EQ(pProducerStats->GetUint32("NewFailed"), 1);
    EXPECT_EQ(pProducerStats->GetUint32("Post"), 4);
    EXPECT_EQ(pProducerStats->GetUint32("PostFailed"), 1);
    auto pConsumerStats = pStats->GetObject("consumer");
    EXPECT_EQ(pConsumerStats->GetUint32("Get"), 4);
    EXPECT_EQ(pConsumerStats->GetUint32("GetFailed"), 1);
    EXPECT_EQ(pConsumerStats->GetUint32("Delete"), 4);
    EXPECT_EQ(pConsumerStats->GetUint32("DeleteFailed"), 1);

    IJson::Destroy(pStats);
}

// 测试完整的生产消费流程
TEST_F(SPSCFixedBoundedChannelTest, TestProducerConsumerFlow)
{
    ChannelConfig config;
    config.uElementSize = sizeof(uint64_t);
    config.uMaxElementCount = 64;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 生产者：创建并发布元素
    for (uint32_t i = 0; i < config.uMaxElementCount; ++i)
    {
        void* pData = channel->New();
        ASSERT_NE(pData, nullptr);
        uint64_t* pInt = static_cast<uint64_t*>(pData);
        *pInt = i;
        channel->Post(pData);
    }

    void* pData2 = channel->New();
    EXPECT_EQ(pData2, nullptr);
    EXPECT_EQ(channel->GetSize(), config.uMaxElementCount);
    
    // 消费者：获取并处理元素
    for (uint32_t i = 0; i < config.uMaxElementCount; ++i)
    {
        void* pData3 = channel->Get();
        ASSERT_NE(pData3, nullptr);
        uint64_t* pInt = static_cast<uint64_t*>(pData3);
        EXPECT_EQ(*pInt, static_cast<uint64_t>(i));
        channel->Delete(pData3);
    }
    void* pData4 = channel->Get();
    EXPECT_EQ(pData4, nullptr);

    EXPECT_EQ(channel->GetSize(), 0);
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试多线程场景 - 单生产者单消费者
TEST_F(SPSCFixedBoundedChannelTest, TestSingleProducerSingleConsumer)
{
    ChannelConfig config;
    config.uElementSize = sizeof(uint64_t);
    config.uMaxElementCount = 1024;
    config.uTotalMemorySizeKB = 0;
    
    ChannelGuard channel(SPSCFixedBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);

    // 生产者线程
    constexpr uint32_t numElements = 10240;
    std::thread producer([&]() {
        for (uint32_t i = 0; i < numElements; ++i)
        {
            void *pData = nullptr;
            while (pData == nullptr)
            {
                pData = channel->New();
                if (pData == nullptr)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
            uint64_t* pInt = static_cast<uint64_t*>(pData);
            *pInt = static_cast<uint64_t>(i);
            channel->Post(pData);
        }
    });

    // 消费者线程
    std::thread consumer([&]() {
        for (uint32_t i = 0; i < numElements; ++i)
        {
            void *pData = nullptr;
            while (pData == nullptr)
            {
                pData = channel->Get();
                if (pData == nullptr)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
            uint64_t* pInt = static_cast<uint64_t*>(pData);
            EXPECT_EQ(*pInt, static_cast<uint64_t>(i));
            channel->Delete(pData);
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(channel->GetSize(), 0);
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试IChannelEx的Push接口
TEST_F(SPSCFixedBoundedChannelTest, TestChannelExPush)
{
    ChannelConfig config;
    config.uElementSize = sizeof(int);
    config.uMaxElementCount = 100;
    config.uTotalMemorySizeKB = 0;
    
    using IntChannel = IChannelEx<int, ChannelType::kSPSC, LengthType::kBounded>;
    IntChannel* pChannel = IntChannel::Create(&config);
    ASSERT_NE(pChannel, nullptr);
    
    // 测试Push接口 - 正常情况
    int value = 42;
    int32_t result = pChannel->Push(std::move(value));
    EXPECT_EQ(result, 0);
    
    // 测试Push接口 - 多个元素
    for (int i = 0; i < 10; ++i)
    {
        int val = i;
        result = pChannel->Push(std::move(val));
        EXPECT_EQ(result, 0);
    }
    
    EXPECT_FALSE(pChannel->IsEmpty());
    EXPECT_GE(pChannel->GetSize(), 1);
    
    IntChannel::Destroy(pChannel);
}

// 测试IChannelEx的Pop接口
TEST_F(SPSCFixedBoundedChannelTest, TestChannelExPop)
{
    ChannelConfig config;
    config.uElementSize = sizeof(int);
    config.uMaxElementCount = 100;
    config.uTotalMemorySizeKB = 0;
    
    using IntChannel = IChannelEx<int, ChannelType::kSPSC, LengthType::kBounded>;
    IntChannel* pChannel = IntChannel::Create(&config);
    ASSERT_NE(pChannel, nullptr);
    
    // 先Push一些数据
    for (int i = 0; i < 10; ++i)
    {
        int val = i;
        pChannel->Push(std::move(val));
    }
    
    // 测试Pop接口 - 正常情况
    int value = 0;
    int32_t result = pChannel->Pop(value);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(value, 0);
    
    // 测试Pop接口 - 多个元素
    for (int i = 1; i < 10; ++i)
    {
        int val = -1;
        result = pChannel->Pop(val);
        EXPECT_EQ(result, 0);
        EXPECT_EQ(val, i);
    }
    
    EXPECT_TRUE(pChannel->IsEmpty());
    
    // 测试Pop接口 - 空通道
    int emptyVal = -1;
    result = pChannel->Pop(emptyVal);
    EXPECT_NE(result, 0);
    
    IntChannel::Destroy(pChannel);
}

// 测试IChannelEx的Push和Pop完整流程
TEST_F(SPSCFixedBoundedChannelTest, TestChannelExPushPopFlow)
{
    ChannelConfig config;
    config.uElementSize = sizeof(int);
    config.uMaxElementCount = 100;
    config.uTotalMemorySizeKB = 0;
    
    using IntChannel = IChannelEx<int, ChannelType::kSPSC, LengthType::kBounded>;
    IntChannel* pChannel = IntChannel::Create(&config);
    ASSERT_NE(pChannel, nullptr);
    
    const int numElements = 50;
    
    // Push元素
    for (int i = 0; i < numElements; ++i)
    {
        int val = i;
        int32_t result = pChannel->Push(std::move(val));
        EXPECT_EQ(result, 0);
    }
    
    EXPECT_EQ(pChannel->GetSize(), numElements);
    
    // Pop元素
    for (int i = 0; i < numElements; ++i)
    {
        int val = -1;
        int32_t result = pChannel->Pop(val);
        EXPECT_EQ(result, 0);
        EXPECT_EQ(val, i);
    }
    
    EXPECT_TRUE(pChannel->IsEmpty());
    
    IntChannel::Destroy(pChannel);
}

// 测试IChannelEx的复杂数据类型
TEST_F(SPSCFixedBoundedChannelTest, TestChannelExComplexType)
{
    struct TestStruct {
        int id;
        char name[32];
        double value;
        
        TestStruct() : id(0), value(0.0) {
            std::memset(name, 0, sizeof(name));
        }
        
        TestStruct(int i, const char* n, double v) : id(i), value(v) {
            std::strncpy(name, n, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        }
        
        void SetInvalid() {
            id = -1;
        }
    };
    
    ChannelConfig config;
    config.uElementSize = sizeof(TestStruct);
    config.uMaxElementCount = 100;
    config.uTotalMemorySizeKB = 0;
    
    using StructChannel = IChannelEx<TestStruct, ChannelType::kSPSC, LengthType::kBounded>;
    StructChannel* pChannel = StructChannel::Create(&config);
    ASSERT_NE(pChannel, nullptr);
    
    // Push复杂结构
    TestStruct ts1(1, "Test1", 3.14);
    int32_t result = pChannel->Push(std::move(ts1));
    EXPECT_EQ(result, 0);
    
    TestStruct ts2(2, "Test2", 2.71);
    result = pChannel->Push(std::move(ts2));
    EXPECT_EQ(result, 0);
    
    // Pop并验证
    TestStruct ts_out;
    result = pChannel->Pop(ts_out);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ts_out.id, 1);
    EXPECT_STREQ(ts_out.name, "Test1");
    EXPECT_DOUBLE_EQ(ts_out.value, 3.14);
    
    result = pChannel->Pop(ts_out);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ts_out.id, 2);
    EXPECT_STREQ(ts_out.name, "Test2");
    EXPECT_DOUBLE_EQ(ts_out.value, 2.71);
    
    StructChannel::Destroy(pChannel);
}
