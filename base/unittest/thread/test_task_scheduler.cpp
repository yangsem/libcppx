#include <gtest/gtest.h>
#include <thread>
#include <thread/task_scheduler.h>
#include <utilities/cppx_common.h>
#include <chrono>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

using namespace cppx::base;

class TaskSchedulerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 测试前的准备工作
        taskExecutionCount = 0;
        taskExecutionOrder.clear();
        taskExecutionMutex.lock();
        taskExecutionOrder.clear();
        taskExecutionMutex.unlock();
    }

    void TearDown() override
    {
        // 测试后的清理工作
    }

    // 测试用的任务函数
    static void TestTaskFunc(void* pCtx)
    {
        TaskSchedulerTest* pTest = static_cast<TaskSchedulerTest*>(pCtx);
        if (pTest)
        {
            pTest->taskExecutionCount++;
            std::lock_guard<std::mutex> lock(pTest->taskExecutionMutex);
            pTest->taskExecutionOrder.push_back("TestTask");
        }
    }

    static void DelayedTaskFunc(void* pCtx)
    {
        TaskSchedulerTest* pTest = static_cast<TaskSchedulerTest*>(pCtx);
        if (pTest)
        {
            pTest->taskExecutionCount++;
            std::lock_guard<std::mutex> lock(pTest->taskExecutionMutex);
            pTest->taskExecutionOrder.push_back("DelayedTask");
        }
    }

    static void PeriodicTaskFunc(void* pCtx)
    {
        TaskSchedulerTest* pTest = static_cast<TaskSchedulerTest*>(pCtx);
        if (pTest)
        {
            pTest->taskExecutionCount++;
            std::lock_guard<std::mutex> lock(pTest->taskExecutionMutex);
            pTest->taskExecutionOrder.push_back("PeriodicTask");
        }
    }

    static void LongRunningTaskFunc(void* pCtx)
    {
        TaskSchedulerTest* pTest = static_cast<TaskSchedulerTest*>(pCtx);
        if (pTest)
        {
            pTest->taskExecutionCount++;
            std::lock_guard<std::mutex> lock(pTest->taskExecutionMutex);
            pTest->taskExecutionOrder.push_back("LongRunningTask");
        }
        // 模拟长时间运行的任务
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    static void CounterTaskFunc(void* pCtx)
    {
        std::atomic<int>* pCounter = static_cast<std::atomic<int>*>(pCtx);
        if (pCounter)
        {
            (*pCounter)++;
        }
    }

    std::atomic<int> taskExecutionCount{0};
    std::vector<std::string> taskExecutionOrder;
    std::mutex taskExecutionMutex;
};

// 测试Create和Destroy接口
TEST_F(TaskSchedulerTest, TestCreateAndDestroy)
{
    // 测试Create接口
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    // 测试Create接口 - 默认参数
    ITaskScheduler* pScheduler2 = ITaskScheduler::Create("TestScheduler2");
    ASSERT_NE(pScheduler2, nullptr);
    
    // 测试Create接口 - 空名称
    ITaskScheduler* pScheduler3 = ITaskScheduler::Create(nullptr);
    ASSERT_NE(pScheduler3, nullptr);
    
    // 测试Destroy接口
    ITaskScheduler::Destroy(pScheduler);
    ITaskScheduler::Destroy(pScheduler2);
}

// 测试Start和Stop接口
TEST_F(TaskSchedulerTest, TestStartAndStop)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    // 测试Start接口
    int32_t result = pScheduler->Start();
    EXPECT_EQ(result, 0);
    
    // 测试Stop接口
    pScheduler->Stop();
    
    // 测试重复Start
    result = pScheduler->Start();
    EXPECT_EQ(result, 0);
    
    // 测试重复Stop
    pScheduler->Stop();
    
    ITaskScheduler::Destroy(pScheduler);
}

// 测试PostTask接口
TEST_F(TaskSchedulerTest, TestPostTask)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 测试PostTask接口 - 有效任务
    ITaskScheduler::Task task;
    task.pTaskName = "TestTask";
    task.pTaskFunc = TestTaskFunc;
    task.pTaskCtx = this;
    task.eTaskType = ITaskScheduler::TaskType::kRunFixedCount;
    task.eVersion = ITaskScheduler::TaskVersion::kVersion;
    task.uFlags = 0;
    task.uTaskExecTimes = 1;
    task.uDelayUs = 0;
    task.uIntervalUs = 0;
    
    int64_t taskID = pScheduler->PostTask(&task);
    EXPECT_NE(taskID, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), 1);
    
    // 测试PostTask接口 - 空任务
    int64_t invalidTaskID = pScheduler->PostTask(nullptr);
    EXPECT_EQ(invalidTaskID, ITaskScheduler::kInvalidTaskID);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试PostOnceTask接口
TEST_F(TaskSchedulerTest, TestPostOnceTask)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 测试PostOnceTask接口 - 立即执行
    int64_t taskID1 = pScheduler->PostOnceTask("ImmediateTask", TestTaskFunc, this, 0);
    EXPECT_NE(taskID1, ITaskScheduler::kInvalidTaskID);
    
    // 测试PostOnceTask接口 - 延迟执行
    int64_t taskID2 = pScheduler->PostOnceTask("DelayedTask", DelayedTaskFunc, this, 50000); // 50ms延迟
    EXPECT_NE(taskID2, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), 2);
    
    // 测试PostOnceTask接口 - 无效参数
    int64_t invalidTaskID1 = pScheduler->PostOnceTask(nullptr, TestTaskFunc, this, 0);
    EXPECT_EQ(invalidTaskID1, ITaskScheduler::kInvalidTaskID);
    
    int64_t invalidTaskID2 = pScheduler->PostOnceTask("TestTask", nullptr, this, 0);
    EXPECT_EQ(invalidTaskID2, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), 2);

    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试PostPeriodicTask接口
TEST_F(TaskSchedulerTest, TestPostPeriodicTask)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 测试PostPeriodicTask接口
    int64_t taskID = pScheduler->PostPeriodicTask("PeriodicTask", PeriodicTaskFunc, this, 0, 100000); // 100ms间隔
    EXPECT_NE(taskID, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行多次
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证任务执行多次
    EXPECT_GT(taskExecutionCount.load(), 4);
    
    // 测试PostPeriodicTask接口 - 无效参数
    int64_t invalidTaskID1 = pScheduler->PostPeriodicTask(nullptr, PeriodicTaskFunc, this, 0, 100000);
    EXPECT_EQ(invalidTaskID1, ITaskScheduler::kInvalidTaskID);
    
    int64_t invalidTaskID2 = pScheduler->PostPeriodicTask("TestTask", nullptr, this, 0, 100000);
    EXPECT_EQ(invalidTaskID2, ITaskScheduler::kInvalidTaskID);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试CancleTask接口
TEST_F(TaskSchedulerTest, TestCancleTask)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 重置计数器
    taskExecutionCount = 0;
    
    // 投递一个延迟任务
    int64_t taskID = pScheduler->PostOnceTask("CancellableTask", TestTaskFunc, this, 100000); // 100ms延迟
    EXPECT_NE(taskID, ITaskScheduler::kInvalidTaskID);
    
    // 立即取消任务
    int32_t cancelResult = pScheduler->CancleTask(taskID);
    EXPECT_EQ(cancelResult, 0);
    
    // 等待足够时间，验证任务未被执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(taskExecutionCount.load(), 0);
    
    // 测试取消无效任务ID
    int32_t invalidCancelResult = pScheduler->CancleTask(ITaskScheduler::kInvalidTaskID);
    EXPECT_NE(invalidCancelResult, 0);
    
    // 测试取消不存在的任务ID
    int32_t nonExistentCancelResult = pScheduler->CancleTask(99999);
    EXPECT_NE(nonExistentCancelResult, 0);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试GetStats接口
TEST_F(TaskSchedulerTest, TestGetStats)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    // 测试GetStats接口 - 调度器未启动
    const char* stats1 = pScheduler->GetStats();
    EXPECT_EQ(stats1, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 测试GetStats接口 - 调度器已启动
    const char* stats2 = pScheduler->GetStats();
    EXPECT_EQ(stats2, nullptr);
    
    // 投递一些任务
    pScheduler->PostOnceTask("Task1", TestTaskFunc, this, 0);
    pScheduler->PostOnceTask("Task2", TestTaskFunc, this, 10000);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 再次获取统计信息
    const char* stats3 = pScheduler->GetStats();
    EXPECT_EQ(stats3, nullptr);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试任务执行顺序
TEST_F(TaskSchedulerTest, TestTaskExecutionOrder)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 投递多个任务，测试执行顺序
    pScheduler->PostOnceTask("Task1", TestTaskFunc, this, 0);
    pScheduler->PostOnceTask("Task2", TestTaskFunc, this, 0);
    pScheduler->PostOnceTask("Task3", TestTaskFunc, this, 0);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), 3);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试并发任务投递
TEST_F(TaskSchedulerTest, TestConcurrentTaskPosting)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 创建多个线程并发投递任务
    std::vector<std::thread> threads;
    const int numThreads = 5;
    const int tasksPerThread = 10;
    
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([pScheduler, this, i]() {
            for (int j = 0; j < tasksPerThread; ++j)
            {
                std::string taskName = "ConcurrentTask_" + std::to_string(i) + "_" + std::to_string(j);
                pScheduler->PostOnceTask(taskName.c_str(), TestTaskFunc, this, 0);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), numThreads * tasksPerThread);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试长时间运行的任务
TEST_F(TaskSchedulerTest, TestLongRunningTask)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 投递长时间运行的任务
    int64_t taskID = pScheduler->PostOnceTask("LongRunningTask", LongRunningTaskFunc, this, 0);
    EXPECT_NE(taskID, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), 1);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试周期性任务的取消
TEST_F(TaskSchedulerTest, TestPeriodicTaskCancellation)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 投递周期性任务
    int64_t taskID = pScheduler->PostPeriodicTask("PeriodicTask", PeriodicTaskFunc, this, 0, 50000); // 50ms间隔
    EXPECT_NE(taskID, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行几次
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    int initialCount = taskExecutionCount.load();
    EXPECT_GT(initialCount, 0);
    
    // 取消周期性任务
    int32_t cancelResult = pScheduler->CancleTask(taskID);
    EXPECT_EQ(cancelResult, 0);
    
    // 等待更长时间，验证任务不再执行
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    int finalCount = taskExecutionCount.load();
    // 任务计数应该不再增加（或增加很少）
    EXPECT_LE(finalCount - initialCount, 2); // 允许少量延迟
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试任务参数传递
TEST_F(TaskSchedulerTest, TestTaskParameterPassing)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 测试任务参数传递
    std::atomic<int> counter(0);
    int64_t taskID = pScheduler->PostOnceTask("CounterTask", CounterTaskFunc, &counter, 0);
    EXPECT_NE(taskID, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 验证参数传递正确
    EXPECT_EQ(counter.load(), 1);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试边界情况
TEST_F(TaskSchedulerTest, TestBoundaryConditions)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 1); // 最小精度
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    // 测试零延迟任务
    int64_t taskID1 = pScheduler->PostOnceTask("ZeroDelayTask", TestTaskFunc, this, 0);
    EXPECT_NE(taskID1, ITaskScheduler::kInvalidTaskID);
    
    // 测试零间隔周期性任务
    int64_t taskID2 = pScheduler->PostPeriodicTask("ZeroIntervalTask", TestTaskFunc, this, 0, 0);
    EXPECT_NE(taskID2, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 验证任务执行
    EXPECT_GT(taskExecutionCount.load(), 0);
    
    // 取消周期性任务
    int32_t cancelResult = pScheduler->CancleTask(taskID2);
    // 注意：取消操作可能返回错误码，我们只验证函数被调用
    EXPECT_TRUE(cancelResult == 0);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试调度器重启
TEST_F(TaskSchedulerTest, TestSchedulerRestart)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    // 第一次启动
    int32_t result1 = pScheduler->Start();
    EXPECT_EQ(result1, 0);
    
    // 投递任务
    int64_t taskID1 = pScheduler->PostOnceTask("Task1", TestTaskFunc, this, 0);
    EXPECT_NE(taskID1, ITaskScheduler::kInvalidTaskID);
    
    // 停止调度器
    pScheduler->Stop();
    
    // 重置计数器
    taskExecutionCount = 0;
    
    // 第二次启动
    int32_t result2 = pScheduler->Start();
    EXPECT_EQ(result2, 0);
    
    // 投递任务
    int64_t taskID2 = pScheduler->PostOnceTask("Task2", TestTaskFunc, this, 0);
    EXPECT_NE(taskID2, ITaskScheduler::kInvalidTaskID);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 验证任务执行
    EXPECT_GT(taskExecutionCount.load(), 0);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}

// 测试性能
TEST_F(TaskSchedulerTest, TestPerformance)
{
    ITaskScheduler* pScheduler = ITaskScheduler::Create("TestScheduler", 10);
    ASSERT_NE(pScheduler, nullptr);
    
    int32_t result = pScheduler->Start();
    ASSERT_EQ(result, 0);
    
    const int numTasks = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    // 投递大量任务
    for (int i = 0; i < numTasks; ++i)
    {
        std::string taskName = "PerfTask_" + std::to_string(i);
        pScheduler->PostOnceTask(taskName.c_str(), TestTaskFunc, this, 0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 验证性能合理（每次投递应该在10微秒以内）
    EXPECT_LT(duration.count(), numTasks * 10);
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证任务执行
    EXPECT_EQ(taskExecutionCount.load(), numTasks);
    
    pScheduler->Stop();
    ITaskScheduler::Destroy(pScheduler);
}