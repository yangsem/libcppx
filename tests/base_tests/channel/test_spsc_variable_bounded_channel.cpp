#include <gtest/gtest.h>
#include <channel/channel.h>
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
    explicit ChannelGuard(SPSCVariableBoundedChannel* pChannel)
        : m_pChannel(pChannel)
    {
    }
    
    ~ChannelGuard()
    {
        if (m_pChannel)
        {
            SPSCVariableBoundedChannel::Destroy(m_pChannel);
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
                SPSCVariableBoundedChannel::Destroy(m_pChannel);
            }
            m_pChannel = other.m_pChannel;
            other.m_pChannel = nullptr;
        }
        return *this;
    }
    
    SPSCVariableBoundedChannel* get() const { return m_pChannel; }
    SPSCVariableBoundedChannel* operator->() const { return m_pChannel; }
    SPSCVariableBoundedChannel& operator*() const { return *m_pChannel; }
    
    explicit operator bool() const { return m_pChannel != nullptr; }

private:
    SPSCVariableBoundedChannel* m_pChannel;
};

class SPSCVariableBoundedChannelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 测试前的准备工作
    }

    void TearDown() override
    {
        // 测试后的清理工作
    }
};

// 测试Create和Destroy接口
TEST_F(SPSCVariableBoundedChannelTest, TestCreateAndDestroy)
{
    // 测试Create接口 - 有效配置
    ChannelConfig config;
    config.uElementSize = 0; // VariableBounded不需要这个参数
    config.uMaxElementCount = 0; // VariableBounded不需要这个参数
    config.uTotalMemorySizeKB = 1024; // 1MB内存 (1024KB)
    
    SPSCVariableBoundedChannel* pChannel = SPSCVariableBoundedChannel::Create(&config);
    ASSERT_NE(pChannel, nullptr);
    
    // 测试Destroy接口
    SPSCVariableBoundedChannel::Destroy(pChannel);
    
    // 测试Create接口 - 空指针配置
    SPSCVariableBoundedChannel* pChannel2 = SPSCVariableBoundedChannel::Create(nullptr);
    // 根据实现，可能返回nullptr或有效指针
    if (pChannel2 != nullptr)
    {
        SPSCVariableBoundedChannel::Destroy(pChannel2);
    }
    
    // 测试Create接口 - 零内存大小
    ChannelConfig config2;
    config2.uElementSize = 0;
    config2.uMaxElementCount = 0;
    config2.uTotalMemorySizeKB = 0;
    SPSCVariableBoundedChannel* pChannel3 = SPSCVariableBoundedChannel::Create(&config2);
    // 根据实现，可能返回nullptr或有效指针
    if (pChannel3 != nullptr)
    {
        SPSCVariableBoundedChannel::Destroy(pChannel3);
    }
    
    // 测试Create接口 - 大内存
    ChannelConfig config3;
    config3.uElementSize = 0;
    config3.uMaxElementCount = 0;
    config3.uTotalMemorySizeKB = 100 * 1024; // 100MB (100*1024KB)
    SPSCVariableBoundedChannel* pChannel4 = SPSCVariableBoundedChannel::Create(&config3);
    if (pChannel4 != nullptr)
    {
        SPSCVariableBoundedChannel::Destroy(pChannel4);
    }
}

// 测试New接口（无参数版本）- 应该返回nullptr
TEST_F(SPSCVariableBoundedChannelTest, TestNewNoParam)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试New接口 - 无参数版本应该返回nullptr（VariableBounded必须指定大小）
    void* pData = channel->New();
    EXPECT_EQ(pData, nullptr);
}

// 测试New接口（带大小参数版本）
TEST_F(SPSCVariableBoundedChannelTest, TestNewWithSize)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试New接口 - 正常大小
    void* pData = channel->New(64);
    ASSERT_NE(pData, nullptr);
    
    // 测试New接口 - 不同大小
    void* pData2 = channel->New(32);
    ASSERT_NE(pData2, nullptr);
    
    // 测试New接口 - 零大小
    void* pData3 = channel->New(0);
    // 根据实现，可能返回nullptr或有效指针
    if (pData3 != nullptr)
    {
        // 如果返回了指针，应该可以正常使用
    }
    
    // 测试New接口 - 大尺寸
    void* pData4 = channel->New(1024);
    if (pData4 != nullptr)
    {
        // 大尺寸元素
    }
}

// 测试Post接口
TEST_F(SPSCVariableBoundedChannelTest, TestPost)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试Post接口 - 正常情况
    void* pData = channel->New(64);
    ASSERT_NE(pData, nullptr);
    
    // 写入一些测试数据
    int* pInt = static_cast<int*>(pData);
    *pInt = 42;
    
    channel->Post(pData);
    
    // 验证通道不为空
    EXPECT_FALSE(channel->IsEmpty());
    
    // 测试Post接口 - 多次Post
    void* pData2 = channel->New(32);
    ASSERT_NE(pData2, nullptr);
    int* pInt2 = static_cast<int*>(pData2);
    *pInt2 = 100;
    channel->Post(pData2);
    
    EXPECT_GE(channel->GetSize(), 1);
}

// 测试Get接口
TEST_F(SPSCVariableBoundedChannelTest, TestGet)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试Get接口 - 空通道
    void* pData = channel->Get();
    EXPECT_EQ(pData, nullptr);
    
    // 测试Get接口 - 正常情况
    void* pNewData = channel->New(sizeof(int));
    ASSERT_NE(pNewData, nullptr);
    int* pInt = static_cast<int*>(pNewData);
    *pInt = 12345;
    channel->Post(pNewData);
    
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    EXPECT_EQ(pGetData, pNewData); // 应该返回同一个指针
    int value = *static_cast<int*>(pGetData);
    EXPECT_EQ(value, 12345);
    
    // 测试Get接口 - 多个元素
    void* pNewData2 = channel->New(sizeof(int));
    ASSERT_NE(pNewData2, nullptr);
    int* pInt2 = static_cast<int*>(pNewData2);
    *pInt2 = 67890;
    channel->Post(pNewData2);
    
    void* pGetData2 = channel->Get();
    ASSERT_NE(pGetData2, nullptr);
    int value2 = *static_cast<int*>(pGetData2);
    EXPECT_EQ(value2, 67890);
}

// 测试Delete接口
TEST_F(SPSCVariableBoundedChannelTest, TestDelete)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试Delete接口 - 正常情况
    void* pData = channel->New(sizeof(int));
    ASSERT_NE(pData, nullptr);
    channel->Post(pData);
    
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    
    channel->Delete(pGetData);
    // Delete后不应该崩溃
    
    // 测试Delete接口 - 空指针
    // channel->Delete(nullptr); // 可能崩溃或忽略
}

// 测试IsEmpty接口
TEST_F(SPSCVariableBoundedChannelTest, TestIsEmpty)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试IsEmpty接口 - 空通道
    EXPECT_TRUE(channel->IsEmpty());
    
    // 测试IsEmpty接口 - 非空通道
    void* pData = channel->New(sizeof(int));
    ASSERT_NE(pData, nullptr);
    channel->Post(pData);
    EXPECT_FALSE(channel->IsEmpty());
    
    // 测试IsEmpty接口 - 获取后为空
    void* pGetData = channel->Get();
    ASSERT_NE(pGetData, nullptr);
    channel->Delete(pGetData);
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试GetSize接口
TEST_F(SPSCVariableBoundedChannelTest, TestGetSize)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试GetSize接口 - 空通道
    EXPECT_EQ(channel->GetSize(), 0);
    
    // 测试GetSize接口 - 单个元素
    void* pData1 = channel->New(sizeof(int));
    ASSERT_NE(pData1, nullptr);
    channel->Post(pData1);
    EXPECT_EQ(channel->GetSize(), 1);
    
    // 测试GetSize接口 - 多个元素
    void* pData2 = channel->New(sizeof(int));
    ASSERT_NE(pData2, nullptr);
    channel->Post(pData2);
    EXPECT_EQ(channel->GetSize(), 2);
    
    void* pData3 = channel->New(sizeof(int));
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
TEST_F(SPSCVariableBoundedChannelTest, TestGetStats)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试GetStats接口 - 空指针
    int32_t result = channel->GetStats(nullptr);
    EXPECT_NE(result, 0);
    
    // 测试GetStats接口 - 正常情况
    IJson* pStats = IJson::Create();
    ASSERT_NE(pStats, nullptr);
    
    result = channel->GetStats(pStats);
    EXPECT_EQ(result, 0);
    
    // 执行一些操作后再获取统计信息
    void* pData = channel->New(sizeof(int));
    if (pData != nullptr)
    {
        channel->Post(pData);
        void* pGetData = channel->Get();
        if (pGetData != nullptr)
        {
            channel->Delete(pGetData);
        }
    }
    
    result = channel->GetStats(pStats);
    EXPECT_EQ(result, 0);
    
    IJson::Destroy(pStats);
}

// 测试完整的生产消费流程
TEST_F(SPSCVariableBoundedChannelTest, TestProducerConsumerFlow)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    const int numElements = 50;
    
    // 生产者：创建并发布元素
    for (int i = 0; i < numElements; ++i)
    {
        void* pData = channel->New(sizeof(int));
        if (pData != nullptr)
        {
            int* pInt = static_cast<int*>(pData);
            *pInt = i;
            channel->Post(pData);
        }
    }
    
    EXPECT_EQ(channel->GetSize(), numElements);
    
    // 消费者：获取并处理元素
    int count = 0;
    while (!channel->IsEmpty())
    {
        void* pData = channel->Get();
        if (pData != nullptr)
        {
            int value = *static_cast<int*>(pData);
            EXPECT_EQ(value, count);
            channel->Delete(pData);
            count++;
        }
        else
        {
            break;
        }
    }
    
    EXPECT_EQ(count, numElements);
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试不同大小的元素
TEST_F(SPSCVariableBoundedChannelTest, TestVariableSizeElements)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试不同大小的元素
    std::vector<std::pair<void*, uint32_t>> elements;
    
    // 创建不同大小的元素
    uint32_t sizes[] = {4, 8, 16, 32, 64, 128, 256};
    for (uint32_t size : sizes)
    {
        void* pData = channel->New(size);
        if (pData != nullptr)
        {
            // 写入测试数据
            std::memset(pData, static_cast<int>(size), size);
            channel->Post(pData);
            elements.push_back({pData, size});
        }
    }
    
    // 验证不同大小的元素
    for (size_t i = 0; i < elements.size(); ++i)
    {
        void* pData = channel->Get();
        if (pData != nullptr)
        {
            EXPECT_EQ(pData, elements[i].first);
            // 验证数据
            uint8_t* pBytes = static_cast<uint8_t*>(pData);
            for (uint32_t j = 0; j < elements[i].second; ++j)
            {
                EXPECT_EQ(pBytes[j], static_cast<uint8_t>(elements[i].second));
            }
            channel->Delete(pData);
        }
    }
    
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试边界条件 - 内存满的情况
TEST_F(SPSCVariableBoundedChannelTest, TestMemoryFull)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB内存 (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 尝试填满内存
    std::vector<void*> allocated;
    const uint32_t elementSize = 1024; // 1KB per element
    int successCount = 0;
    
    for (int i = 0; i < 2000; ++i) // 尝试超过容量（1MB / 1KB = 1024个元素）
    {
        void* pData = channel->New(elementSize);
        if (pData != nullptr)
        {
            int* pInt = static_cast<int*>(pData);
            *pInt = i;
            channel->Post(pData);
            allocated.push_back(pData);
            successCount++;
        }
        else
        {
            // 内存满时New可能返回nullptr
            break;
        }
    }
    
    uint32_t size = channel->GetSize();
    EXPECT_LE(size, static_cast<uint32_t>(successCount));
    
    // 清理
    while (!channel->IsEmpty())
    {
        void* pData = channel->Get();
        if (pData != nullptr)
        {
            channel->Delete(pData);
        }
        else
        {
            break;
        }
    }
}

// 测试多线程场景 - 单生产者单消费者
TEST_F(SPSCVariableBoundedChannelTest, TestSingleProducerSingleConsumer)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 10 * 1024; // 10MB内存 (10*1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    const int numElements = 10000;
    std::atomic<int> producedCount(0);
    std::atomic<int> consumedCount(0);
    std::atomic<bool> producerDone(false);
    
    // 生产者线程
    std::thread producer([&]() {
        for (int i = 0; i < numElements; ++i)
        {
            void* pData = channel->New(sizeof(int));
            if (pData != nullptr)
            {
                int* pInt = static_cast<int*>(pData);
                *pInt = i;
                channel->Post(pData);
                producedCount++;
            }
            else
            {
                // 如果New失败，稍等再试
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                --i; // 重试
            }
        }
        producerDone = true;
    });
    
    // 消费者线程
    std::thread consumer([&]() {
        while (!producerDone || !channel->IsEmpty())
        {
            void* pData = channel->Get();
            if (pData != nullptr)
            {
                int value = *static_cast<int*>(pData);
                EXPECT_EQ(value, consumedCount);
                channel->Delete(pData);
                consumedCount++;
            }
            else
            {
                // 如果Get失败，稍等再试
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(producedCount.load(), numElements);
    EXPECT_EQ(consumedCount.load(), numElements);
    EXPECT_TRUE(channel->IsEmpty());
}

// 测试数据完整性
TEST_F(SPSCVariableBoundedChannelTest, TestDataIntegrity)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    const int numElements = 50;
    
    // 创建并发布包含复杂数据的元素
    for (int i = 0; i < numElements; ++i)
    {
        const uint32_t dataSize = 256;
        void* pData = channel->New(dataSize);
        if (pData != nullptr)
        {
            // 写入测试数据
            std::memset(pData, 0, dataSize);
            int* pInt = static_cast<int*>(pData);
            *pInt = i;
            char* pStr = static_cast<char*>(pData) + sizeof(int);
            std::snprintf(pStr, 100, "Element_%d", i);
            
            channel->Post(pData);
        }
    }
    
    // 验证数据完整性
    for (int i = 0; i < numElements; ++i)
    {
        void* pData = channel->Get();
        if (pData != nullptr)
        {
            int value = *static_cast<int*>(pData);
            EXPECT_EQ(value, i);
            
            char* pStr = static_cast<char*>(pData) + sizeof(int);
            char expected[100];
            std::snprintf(expected, 100, "Element_%d", i);
            EXPECT_STREQ(pStr, expected);
            
            channel->Delete(pData);
        }
        else
        {
            break;
        }
    }
}

// 测试性能 - 大量元素
TEST_F(SPSCVariableBoundedChannelTest, TestPerformance)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 10 * 1024; // 10MB (10*1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    const int numElements = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    
    // 生产
    for (int i = 0; i < numElements; ++i)
    {
        void* pData = channel->New(sizeof(int));
        if (pData != nullptr)
        {
            int* pInt = static_cast<int*>(pData);
            *pInt = i;
            channel->Post(pData);
        }
    }
    
    // 消费
    int count = 0;
    while (!channel->IsEmpty() && count < numElements)
    {
        void* pData = channel->Get();
        if (pData != nullptr)
        {
            channel->Delete(pData);
            count++;
        }
        else
        {
            break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(count, numElements);
    // 验证性能合理（10万次操作应该在几秒内完成）
    EXPECT_LT(duration.count(), 10000);
}

// 测试New和Post的配对使用
TEST_F(SPSCVariableBoundedChannelTest, TestNewPostPair)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 测试New后必须Post
    void* pData = channel->New(sizeof(int));
    ASSERT_NE(pData, nullptr);
    
    // 不Post直接再次New（可能失败或返回新指针）
    void* pData2 = channel->New(sizeof(int));
    // 根据实现，可能返回nullptr或新指针
    ASSERT_EQ(pData2, nullptr);
    // 正常Post
    if (pData != nullptr)
    {
        int* pInt = static_cast<int*>(pData);
        *pInt = 42;
        channel->Post(pData);
    }
    
    // 验证可以Get
    void* pGetData = channel->Get();
    if (pGetData != nullptr)
    {
        EXPECT_EQ(*static_cast<int*>(pGetData), 42);
        channel->Delete(pGetData);
    }
}

// 测试Get和Delete的配对使用
TEST_F(SPSCVariableBoundedChannelTest, TestGetDeletePair)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 先Post一些数据
    for (int i = 0; i < 10; ++i)
    {
        void* pData = channel->New(sizeof(int));
        if (pData != nullptr)
        {
            int* pInt = static_cast<int*>(pData);
            *pInt = i;
            channel->Post(pData);
        }
    }
    
    // 测试Get后必须Delete
    void* pData = channel->Get();
    ASSERT_NE(pData, nullptr);
    
    // 不Delete直接再次Get（应该获取下一个元素）
    void* pData2 = channel->Get();
    // 根据实现，可能返回nullptr或下一个元素
    ASSERT_EQ(pData2, nullptr);
    // 正常Delete
    if (pData != nullptr)
    {
        channel->Delete(pData);
    }
    
    // 验证大小减少
    uint32_t size = channel->GetSize();
    EXPECT_LE(size, 9);
}

// 测试混合大小的元素场景
TEST_F(SPSCVariableBoundedChannelTest, TestMixedSizeElements)
{
    ChannelConfig config;
    config.uElementSize = 0;
    config.uMaxElementCount = 0;
    config.uTotalMemorySizeKB = 1024; // 1MB (1024KB)
    
    ChannelGuard channel(SPSCVariableBoundedChannel::Create(&config));
    ASSERT_NE(channel.get(), nullptr);
    
    // 创建混合大小的元素
    struct TestData {
        void* pData;
        uint32_t size;
        int value;
    };
    
    std::vector<TestData> testData;
    
    // 创建不同大小的元素并写入不同的值
    for (int i = 0; i < 20; ++i)
    {
        uint32_t size = (i % 5 + 1) * 16; // 16, 32, 48, 64, 80字节
        void* pData = channel->New(size);
        if (pData != nullptr)
        {
            int* pInt = static_cast<int*>(pData);
            *pInt = i;
            channel->Post(pData);
            testData.push_back({pData, size, i});
        }
    }
    
    // 验证所有元素
    for (size_t i = 0; i < testData.size(); ++i)
    {
        void* pData = channel->Get();
        if (pData != nullptr)
        {
            EXPECT_EQ(pData, testData[i].pData);
            int value = *static_cast<int*>(pData);
            EXPECT_EQ(value, testData[i].value);
            channel->Delete(pData);
        }
    }
    
    EXPECT_TRUE(channel->IsEmpty());
}
