#include <gtest/gtest.h>
#include <thread>
#include <task_scheduler.h>
#include <cppx_common.h>

TEST(TaskScheduler, ImmdeteTask)
{
    auto pTaskScheduler = cppx::base::ITaskScheduler::Create("unittest");
    ASSERT_NE(pTaskScheduler, nullptr);
    ASSERT_EQ(pTaskScheduler->Start(), 0);

    class TestTask
    {
    public:
        bool is_exec = false;
        uint64_t begin;
        uint64_t end;

        static void Task(void* ctx)
        {
            auto pThis = (TestTask *)ctx;
            pThis->is_exec = true;
            clock_get_time_nano(pThis->end);
        }
    };
    TestTask task;
    clock_get_time_nano(task.begin);
    EXPECT_EQ(pTaskScheduler->PostOnceTask("once_task", &TestTask::Task, &task, 0), 0);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    EXPECT_TRUE(task.is_exec);
    printf("delay exec %lu ns\n", task.end - task.begin);

    pTaskScheduler->Stop();
    cppx::base::ITaskScheduler::Destroy(pTaskScheduler);
}

