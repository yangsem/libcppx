#include <cppx_last_error.h>
#include <gtest/gtest.h>

TEST(LastError, TestSetAndGet)
{
    SET_LAST_ERROR(1001, "This is a test error message with code %d", 1001);
    EXPECT_EQ(cppx::base::LastError::GetLastError(), 1001);
    EXPECT_EQ(std::string(cppx::base::LastError::GetLastErrorStr()), "This is a test error message with code 1001");


    cppx::base::LastError::SetLastError(2002, nullptr);
    EXPECT_EQ(cppx::base::LastError::GetLastError(), 2002);
    EXPECT_EQ(std::string(cppx::base::LastError::GetLastErrorStr()), "unknown error 2002");

    SET_LAST_ERROR(-1, "Negative error code test");
    EXPECT_EQ(cppx::base::LastError::GetLastError(), -1);
    EXPECT_EQ(std::string(cppx::base::LastError::GetLastErrorStr()), "Negative error code test");

    SET_LAST_ERROR(0, "Zero error code test %d %s", 0, "OK");
    EXPECT_EQ(cppx::base::LastError::GetLastError(), 0);
    EXPECT_EQ(std::string(cppx::base::LastError::GetLastErrorStr()), "Zero error code test 0 OK");
}