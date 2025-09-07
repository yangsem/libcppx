#include "cppx_get_time.h"
#include <time.h>

namespace cppx
{
namespace base
{

constexpr uint64_t kMicor = 1000;
constexpr uint64_t kMill = kMicor * 1000;
constexpr uint64_t kSecond = kMill * 1000;

uint64_t GetTimeTool::GetCurrentNano()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * kSecond) + ts.tv_nsec;
}

uint64_t GetTimeTool::GetCurrentMicro()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * kMicor) + ts.tv_nsec / kMicor;
}

uint64_t GetTimeTool::GetCurrentMill()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * kMill) + ts.tv_nsec / kMill;
}

uint64_t GetTimeTool::GetCurrentSecond()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec;
}

}
}