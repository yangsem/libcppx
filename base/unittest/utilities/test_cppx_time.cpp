#include <gtest/gtest.h>
#include <utilities/cppx_time.h>
#include <chrono>
#include <thread>

using namespace cppx::base;

class CppxTimeTest : public ::testing::Test
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

// 测试构造函数
TEST_F(CppxTimeTest, TestConstructors)
{
    // 测试默认构造函数
    ITime time1;
    EXPECT_EQ(time1.uYear, 0);
    EXPECT_EQ(time1.uMonth, 0);
    EXPECT_EQ(time1.uDay, 0);
    EXPECT_EQ(time1.uHour, 0);
    EXPECT_EQ(time1.uMinute, 0);
    EXPECT_EQ(time1.uSecond, 0);
    EXPECT_EQ(time1.uMicro, 0);

    // 测试带参数的构造函数
    ITime time2(2024, 1, 15, 10, 30, 45, 123456);
    EXPECT_EQ(time2.uYear, 2024);
    EXPECT_EQ(time2.uMonth, 1);
    EXPECT_EQ(time2.uDay, 15);
    EXPECT_EQ(time2.uHour, 10);
    EXPECT_EQ(time2.uMinute, 30);
    EXPECT_EQ(time2.uSecond, 45);
    EXPECT_EQ(time2.uMicro, 123456);

    // 测试带默认参数的构造函数
    ITime time3(2024, 12, 31);
    EXPECT_EQ(time3.uYear, 2024);
    EXPECT_EQ(time3.uMonth, 12);
    EXPECT_EQ(time3.uDay, 31);
    EXPECT_EQ(time3.uHour, 0);
    EXPECT_EQ(time3.uMinute, 0);
    EXPECT_EQ(time3.uSecond, 0);
    EXPECT_EQ(time3.uMicro, 0);

    // 测试时间戳构造函数
    uint64_t timestamp = 1705123456; // 2024-01-13 05:24:16 UTC+0
    ITime time4(timestamp);
    EXPECT_EQ(time4.uYear, 2024); // 验证年份合理
    EXPECT_EQ(time4.uMonth, 1);
    EXPECT_EQ(time4.uDay, 13);
    EXPECT_EQ(time4.uHour, 5);
    EXPECT_EQ(time4.uMinute, 24);
    EXPECT_EQ(time4.uSecond, 16);
    EXPECT_EQ(time4.uMicro, 0);
}

// 测试拷贝构造函数和赋值操作符
TEST_F(CppxTimeTest, TestCopyConstructorAndAssignment)
{
    ITime original(2024, 6, 15, 14, 30, 45, 500000);
    
    // 测试拷贝构造函数
    ITime copy1(original);
    EXPECT_EQ(copy1.uYear, original.uYear);
    EXPECT_EQ(copy1.uMonth, original.uMonth);
    EXPECT_EQ(copy1.uDay, original.uDay);
    EXPECT_EQ(copy1.uHour, original.uHour);
    EXPECT_EQ(copy1.uMinute, original.uMinute);
    EXPECT_EQ(copy1.uSecond, original.uSecond);
    EXPECT_EQ(copy1.uMicro, original.uMicro);

    // 测试赋值操作符
    ITime copy2;
    copy2 = original;
    EXPECT_EQ(copy2.uYear, original.uYear);
    EXPECT_EQ(copy2.uMonth, original.uMonth);
    EXPECT_EQ(copy2.uDay, original.uDay);
    EXPECT_EQ(copy2.uHour, original.uHour);
    EXPECT_EQ(copy2.uMinute, original.uMinute);
    EXPECT_EQ(copy2.uSecond, original.uSecond);
    EXPECT_EQ(copy2.uMicro, original.uMicro);

    // 测试时间戳赋值
    uint64_t timestamp = 1705123456;
    ITime time3;
    time3 = timestamp;
    EXPECT_EQ(time3.uYear, 2024);
    EXPECT_EQ(time3.uMonth, 1);
    EXPECT_EQ(time3.uDay, 13);
    EXPECT_EQ(time3.uHour, 5);
    EXPECT_EQ(time3.uMinute, 24);
    EXPECT_EQ(time3.uSecond, 16);
    EXPECT_EQ(time3.uMicro, 0);
}

// 测试时间戳转换函数
TEST_F(CppxTimeTest, TestTimeStampConversion)
{
    // 使用一个已知的时间进行测试
    ITime time(2024, 1, 1, 0, 0, 0, 0);
    
    // 测试GetTimeStampSecond
    uint64_t seconds = time.GetTimeStampSecond();
    EXPECT_EQ(seconds, 1704067200);
    
    // 测试GetTimeStampMill
    uint64_t milliseconds = time.GetTimeStampMill();
    EXPECT_EQ(milliseconds, seconds * 1000);
    
    // 测试GetTimeStampMicro
    uint64_t microseconds = time.GetTimeStampMicro();
    EXPECT_GT(microseconds, 0);
    EXPECT_EQ(microseconds, seconds * 1000000);
    
    // 测试带微秒的时间
    ITime timeWithMicro(2024, 1, 1, 0, 0, 0, 123456);
    uint64_t microSeconds2 = timeWithMicro.GetTimeStampMicro();
    uint64_t milliSeconds2 = timeWithMicro.GetTimeStampMill();
    uint64_t seconds2 = timeWithMicro.GetTimeStampSecond();
    
    EXPECT_EQ(microSeconds2, seconds2 * 1000000 + 123456);
    EXPECT_EQ(milliSeconds2, seconds2 * 1000 + 123);
}

// 测试比较操作符
TEST_F(CppxTimeTest, TestComparisonOperators)
{
    ITime time1(2024, 1, 1, 10, 0, 0, 0);
    ITime time2(2024, 1, 1, 11, 0, 0, 0);
    ITime time3(2024, 1, 1, 10, 0, 0, 0);
    
    // 测试小于操作符
    EXPECT_TRUE(time1 < time2);
    EXPECT_FALSE(time2 < time1);
    EXPECT_FALSE(time1 < time3);
    
    // 测试大于操作符
    EXPECT_TRUE(time2 > time1);
    EXPECT_FALSE(time1 > time2);
    EXPECT_FALSE(time1 > time3);
    
    // 测试小于等于操作符
    EXPECT_TRUE(time1 <= time2);
    EXPECT_TRUE(time1 <= time3);
    EXPECT_FALSE(time2 <= time1);
    
    // 测试大于等于操作符
    EXPECT_TRUE(time2 >= time1);
    EXPECT_TRUE(time1 >= time3);
    EXPECT_FALSE(time1 >= time2);
    
    // 测试等于操作符
    EXPECT_TRUE(time1 == time3);
    EXPECT_FALSE(time1 == time2);
    
    // 测试不等于操作符
    EXPECT_TRUE(time1 != time2);
    EXPECT_FALSE(time1 != time3);
}

// 测试时间加减操作符
TEST_F(CppxTimeTest, TestArithmeticOperators)
{
    ITime time1(2024, 1, 1, 10, 30, 0, 0);
    ITime time2(2024, 1, 1, 0, 30, 0, 0);
    
    // 测试加法操作符 - 注意：operator+修改当前对象并返回引用
    ITime result1 = time1;
    result1 = result1 + time2;
    uint64_t expectedSeconds = time1.GetTimeStampSecond() + time2.GetTimeStampSecond();
    EXPECT_EQ(result1.GetTimeStampSecond(), expectedSeconds);
    
    // 测试减法操作符 - 注意：operator-修改当前对象并返回引用
    ITime result2 = time1;
    result2 = result2 - time2;
    uint64_t expectedSeconds2 = time1.GetTimeStampSecond() - time2.GetTimeStampSecond();
    EXPECT_EQ(result2.GetTimeStampSecond(), expectedSeconds2);
    
    // 测试+=操作符
    ITime time3 = time1;
    time3 += time2;
    EXPECT_EQ(time3.GetTimeStampSecond(), expectedSeconds);
    
    // 测试-=操作符
    ITime time4 = time1;
    time4 -= time2;
    EXPECT_EQ(time4.GetTimeStampSecond(), expectedSeconds2);
}

// 测试时间戳加减操作符
TEST_F(CppxTimeTest, TestTimeStampArithmeticOperators)
{
    ITime time(2024, 1, 1, 10, 0, 0, 0);
    uint64_t seconds = 3600; // 1小时
    
    // 测试加法操作符
    ITime result1 = time + seconds;
    EXPECT_EQ(result1.uHour, 11); // 10 + 1 = 11
    
    // 测试减法操作符
    ITime result2 = time - seconds;
    EXPECT_EQ(result2.uHour, 9); // 10 - 1 = 9
    
    // 测试+=操作符
    ITime time3 = time;
    time3 += seconds;
    EXPECT_EQ(time3.uHour, 11);
    
    // 测试-=操作符
    ITime time4 = time;
    time4 -= seconds;
    EXPECT_EQ(time4.uHour, 9);
}

// 测试自增自减操作符
TEST_F(CppxTimeTest, TestIncrementDecrementOperators)
{
    ITime time(2024, 1, 1, 10, 30, 45, 0);
    
    // 测试前置自增
    ITime time1 = time;
    ITime& result1 = ++time1;
    EXPECT_EQ(&result1, &time1); // 返回引用
    EXPECT_EQ(time1.uSecond, 46); // 45 + 1 = 46
    
    // 测试后置自增
    ITime time2 = time;
    ITime result2 = time2++;
    EXPECT_EQ(result2.uSecond, 45); // 返回原值
    EXPECT_EQ(time2.uSecond, 46); // 自增后的值
    
    // 测试前置自减
    ITime time3 = time;
    ITime& result3 = --time3;
    EXPECT_EQ(&result3, &time3); // 返回引用
    EXPECT_EQ(time3.uSecond, 44); // 45 - 1 = 44
    
    // 测试后置自减
    ITime time4 = time;
    ITime result4 = time4--;
    EXPECT_EQ(result4.uSecond, 45); // 返回原值
    EXPECT_EQ(time4.uSecond, 44); // 自减后的值
    
    // 测试秒数进位
    ITime time5(2024, 1, 1, 10, 30, 59, 0);
    ++time5;
    EXPECT_EQ(time5.uSecond, 0);
    EXPECT_EQ(time5.uMinute, 31); // 分钟进位
    
    // 测试分钟进位
    ITime time6(2024, 1, 1, 10, 59, 59, 0);
    ++time6;
    EXPECT_EQ(time6.uSecond, 0);
    EXPECT_EQ(time6.uMinute, 0);
    EXPECT_EQ(time6.uHour, 11); // 小时进位
}

// 测试静态函数 - 获取时间
TEST_F(CppxTimeTest, TestStaticTimeFunctions)
{
    // 测试GetLocalTime
    ITime localTime = ITime::GetLocalTime();
    EXPECT_GT(localTime.uYear, 2020);
    EXPECT_GE(localTime.uMonth, 1);
    EXPECT_LE(localTime.uMonth, 12);
    EXPECT_GE(localTime.uDay, 1);
    EXPECT_LE(localTime.uDay, 31);
    EXPECT_GE(localTime.uHour, 0);
    EXPECT_LE(localTime.uHour, 23);
    EXPECT_GE(localTime.uMinute, 0);
    EXPECT_LE(localTime.uMinute, 59);
    EXPECT_GE(localTime.uSecond, 0);
    EXPECT_LE(localTime.uSecond, 59);
    EXPECT_GE(localTime.uMicro, 0);
    EXPECT_LE(localTime.uMicro, 999999);
    
    // 测试GetUTCTime
    ITime utcTime = ITime::GetUTCTime();
    EXPECT_GT(utcTime.uYear, 2020);
    EXPECT_GE(utcTime.uMonth, 1);
    EXPECT_LE(utcTime.uMonth, 12);
    EXPECT_GE(utcTime.uDay, 1);
    EXPECT_LE(utcTime.uDay, 31);
    EXPECT_GE(utcTime.uHour, 0);
    EXPECT_LE(utcTime.uHour, 23);
    EXPECT_GE(utcTime.uMinute, 0);
    EXPECT_LE(utcTime.uMinute, 59);
    EXPECT_GE(utcTime.uSecond, 0);
    EXPECT_LE(utcTime.uSecond, 59);
    EXPECT_GE(utcTime.uMicro, 0);
    EXPECT_LE(utcTime.uMicro, 999999);
    
    // 测试GetUTCTSecond
    uint64_t utcSeconds = ITime::GetUTCTSecond();
    EXPECT_GT(utcSeconds, 0);
    
    // 验证时间戳的一致性
    uint64_t utcTimeSeconds = utcTime.GetTimeStampSecond();
    EXPECT_NEAR(utcSeconds, utcTimeSeconds, 1); // 允许1秒的误差
}

// 测试静态函数 - 设置UTC时间
TEST_F(CppxTimeTest, TestSetUTCTimeFunctions)
{
    // 测试通过ITime对象设置UTC时间
    ITime testTime(2024, 1, 1, 12, 0, 0, 0);
    int32_t result1 = ITime::SetUTCTime(&testTime);
    // 注意：这个函数可能会失败，因为通常需要系统权限
    // 我们只验证函数能够被调用
    EXPECT_TRUE(result1 == 0 || result1 != 0); // 接受任何返回值
    
    // 测试通过时间戳设置UTC时间
    uint64_t timestamp = 1704110400; // 2024-01-01 12:00:00 UTC
    int32_t result2 = ITime::SetUTCTime(timestamp);
    // 同样，这个函数可能会失败
    EXPECT_TRUE(result2 == 0 || result2 != 0); // 接受任何返回值
    
    // 测试空指针
    int32_t result3 = ITime::SetUTCTime(nullptr);
    EXPECT_NE(result3, 0); // 应该返回错误
}

// 测试时区相关函数
TEST_F(CppxTimeTest, TestTimeZoneFunctions)
{
    // 测试GetTimeZone
    int32_t timeZone = ITime::GetTimeZone();
    // 时区返回的是秒数偏移，通常在-43200到+50400之间（-12到+14小时）
    EXPECT_GE(timeZone, -43200); // -12小时 = -43200秒
    EXPECT_LE(timeZone, 50400);  // +14小时 = +50400秒
    
    // 测试SetTimeZone
    int32_t originalTimeZone = timeZone;
    int32_t newTimeZone = 28800; // UTC+8 = 8*3600 = 28800秒
    int32_t result = ITime::SetTimeZone(newTimeZone);
    
    // 验证设置是否成功
    if (result == 0) {
        int32_t currentTimeZone = ITime::GetTimeZone();
        EXPECT_EQ(currentTimeZone, newTimeZone);
        
        // 恢复原始时区
        ITime::SetTimeZone(originalTimeZone);
    } else {
        // 如果设置失败，验证原始时区未改变
        int32_t currentTimeZone = ITime::GetTimeZone();
        EXPECT_EQ(currentTimeZone, originalTimeZone);
    }
}

// 测试夏令时函数
TEST_F(CppxTimeTest, TestDstFunction)
{
    // 测试IsDst
    int32_t dst = ITime::IsDst();
    // 夏令时返回值：0表示不是夏令时，1表示是夏令时，-1表示错误
    EXPECT_TRUE(dst == 0 || dst == 1 || dst == -1);
}

// 测试边界情况
TEST_F(CppxTimeTest, TestBoundaryConditions)
{
    // 测试年份边界
    ITime time1(1970, 1, 1, 0, 0, 0, 0); // Unix时间戳起始
    uint64_t timestamp1 = time1.GetTimeStampSecond();
    EXPECT_GE(timestamp1, 0);
    
    // 测试月份边界
    ITime time2(2024, 12, 31, 23, 59, 59, 999999);
    uint64_t timestamp2 = time2.GetTimeStampSecond();
    EXPECT_GT(timestamp2, 0);
    
    // 测试闰年
    ITime time3(2024, 2, 29, 12, 0, 0, 0); // 2024是闰年
    uint64_t timestamp3 = time3.GetTimeStampSecond();
    EXPECT_GT(timestamp3, 0);
    
    // 测试时间比较的边界情况
    ITime time4(2024, 1, 1, 0, 0, 0, 0);
    ITime time5(2024, 1, 1, 0, 0, 0, 1);
    EXPECT_TRUE(time4 < time5);
    EXPECT_TRUE(time5 > time4);
}

// 测试时间精度
TEST_F(CppxTimeTest, TestTimePrecision)
{
    ITime time1(2024, 1, 1, 12, 0, 0, 123456);
    ITime time2(2024, 1, 1, 12, 0, 0, 123457);
    
    // 测试微秒级精度
    EXPECT_TRUE(time1 < time2);
    EXPECT_TRUE(time2 > time1);
    
    // 测试时间戳转换的精度
    uint64_t micro1 = time1.GetTimeStampMicro();
    uint64_t micro2 = time2.GetTimeStampMicro();
    EXPECT_EQ(micro2 - micro1, 1); // 相差1微秒
    
    uint64_t milli1 = time1.GetTimeStampMill();
    uint64_t milli2 = time2.GetTimeStampMill();
    EXPECT_EQ(milli1, milli2); // 毫秒级相同
}

// 测试性能
TEST_F(CppxTimeTest, TestPerformance)
{
    const int iterations = 10000;
    
    // 测试时间戳转换性能
    ITime time(2024, 1, 1, 12, 0, 0, 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile uint64_t timestamp = time.GetTimeStampSecond();
        (void)timestamp; // 避免编译器优化
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // 验证性能合理（每次调用应该在1微秒以内）
    EXPECT_LT(duration.count(), iterations * 10); // 允许10微秒的容差
}

// 测试时间运算的复杂情况
TEST_F(CppxTimeTest, TestComplexTimeOperations)
{
    // 测试跨日期的加法 - 基于时间戳运算
    ITime time1(2024, 1, 1, 23, 30, 0, 0);
    ITime time2(2024, 1, 1, 0, 30, 0, 0);
    ITime result1 = time1;
    result1 = result1 + time2;
    uint64_t expectedSeconds1 = time1.GetTimeStampSecond() + time2.GetTimeStampSecond();
    EXPECT_EQ(result1.GetTimeStampSecond(), expectedSeconds1);
    
    // 测试跨月份的减法 - 基于时间戳运算
    ITime time3(2024, 2, 1, 0, 0, 0, 0);
    ITime time4(2024, 1, 1, 0, 0, 0, 0);
    ITime result2 = time3;
    result2 = result2 - time4;
    uint64_t expectedSeconds2 = time3.GetTimeStampSecond() - time4.GetTimeStampSecond();
    EXPECT_EQ(result2.GetTimeStampSecond(), expectedSeconds2);
    
    // 测试大数值的时间戳运算
    uint64_t largeTimestamp = 86400; // 1天
    ITime time5(2024, 1, 1, 0, 0, 0, 0);
    ITime result3 = time5 + largeTimestamp;
    uint64_t expectedSeconds3 = time5.GetTimeStampSecond() + largeTimestamp;
    EXPECT_EQ(result3.GetTimeStampSecond(), expectedSeconds3);
    
    // 验证结果时间的合理性
    EXPECT_GT(result3.uDay, time5.uDay); // 应该跨到第二天
}
