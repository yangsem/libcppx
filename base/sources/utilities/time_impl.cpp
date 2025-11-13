#include <ctime>
#include <utilities/time.h>
#include <utilities/common.h>
#include <utilities/error_code.h>

namespace cppx
{
namespace base
{

ITime::ITime(const ITime &time)
{
    *this = time;
}

ITime::ITime(uint32_t uYear, uint32_t uMonth, uint32_t uDay, 
             uint32_t uHour, uint32_t uMinute, uint32_t uSecond, uint32_t uMicro)
{
    this->uYear = uYear;
    this->uMonth = uMonth;
    this->uDay = uDay;
    this->uHour = uHour;
    this->uMinute = uMinute;
    this->uSecond = uSecond;
    this->uMicro = uMicro;
}

ITime::ITime(uint64_t uTimeStampSecond)
{
    struct tm tm;
    time_t time_sec = static_cast<time_t>(uTimeStampSecond);
    
#ifndef OS_WIN
    gmtime_r(&time_sec, &tm);
#else
    gmtime_s(time_sec, tm);
#endif
    
    uYear = static_cast<uint32_t>(tm.tm_year + 1900);
    uMonth = static_cast<uint32_t>(tm.tm_mon + 1);
    uDay = static_cast<uint32_t>(tm.tm_mday);
    uHour = static_cast<uint32_t>(tm.tm_hour);
    uMinute = static_cast<uint32_t>(tm.tm_min);
    uSecond = static_cast<uint32_t>(tm.tm_sec);
    uMicro = 0;
}

ITime& ITime::operator=(uint64_t uTimeStampSecond)
{
    *this = ITime(uTimeStampSecond);
    return *this;
}

uint64_t ITime::GetTimeStampSecond() const
{
    struct tm tm;
    tm.tm_year = static_cast<int>(uYear) - 1900;
    tm.tm_mon = static_cast<int>(uMonth) - 1;
    tm.tm_mday = static_cast<int>(uDay);
    tm.tm_hour = static_cast<int>(uHour);
    tm.tm_min = static_cast<int>(uMinute);
    tm.tm_sec = static_cast<int>(uSecond);
    
    return static_cast<uint64_t>(timegm(&tm));
}

uint64_t ITime::GetTimeStampMill() const
{
    return GetTimeStampSecond() * 1000 + (uMicro / 1000);
}

uint64_t ITime::GetTimeStampMicro() const
{
    return GetTimeStampSecond() * 1000000 + uMicro;
}

bool ITime::operator<(const ITime &other) const
{
    return uYear < other.uYear || uMonth < other.uMonth || uDay < other.uDay
           || uHour < other.uHour || uMinute < other.uMinute || uSecond < other.uSecond
           || uMicro < other.uMicro;
}

bool ITime::operator>(const ITime &other) const
{
    return other < *this;
}

bool ITime::operator<=(const ITime &other) const
{
    return !(other < *this);
}

bool ITime::operator>=(const ITime &other) const
{
    return !(*this < other);
}

bool ITime::operator==(const ITime &other) const
{
    return uYear == other.uYear && uMonth == other.uMonth && uDay == other.uDay &&
           uHour == other.uHour && uMinute == other.uMinute && uSecond == other.uSecond &&
           uMicro == other.uMicro;
}

bool ITime::operator!=(const ITime &other) const
{
    return !(*this == other);
}

ITime &ITime::operator+(const ITime &other)
{
    auto uThisSecond = this->GetTimeStampSecond();
    auto uOhterSecond = other.GetTimeStampSecond();
    *this = ITime(uThisSecond + uOhterSecond);
    return *this;
}

ITime &ITime::operator-(const ITime &other)
{
    auto uThisSecond = this->GetTimeStampSecond();
    auto uOhterSecond = other.GetTimeStampSecond();
    if(likely(uThisSecond > uOhterSecond))
    {
        *this = ITime(uThisSecond - uOhterSecond);
    }
    else
    {
        *this = ITime(0);
    }
    return *this;
}

ITime &ITime::operator+=(const ITime &other)
{
    return *this + other;
}

ITime &ITime::operator-=(const ITime &other)
{
    return *this - other;
}

ITime ITime::operator+(uint64_t uTimeStampSecond)
{
    ITime result = *this;
    result += uTimeStampSecond;
    return result;
}

ITime ITime::operator-(uint64_t uTimeStampSecond)
{
    ITime result = *this;
    result -= uTimeStampSecond;
    return result;
}

ITime &ITime::operator+=(uint64_t uTimeStampSecond)
{
    ITime timeToAdd(uTimeStampSecond);
    return *this += timeToAdd;
}

ITime &ITime::operator-=(uint64_t uTimeStampSecond)
{
    ITime timeToSubtract(uTimeStampSecond);
    return *this -= timeToSubtract;
}

ITime &ITime::operator++()
{
    return (*this) += 1;
}

ITime &ITime::operator--()
{
    return (*this) -= 1;
}

ITime ITime::operator++(int)
{
    ITime temp = *this;
    ++(*this);
    return temp;
}

ITime ITime::operator--(int)
{
    ITime temp = *this;
    --(*this);
    return temp;
}

ITime ITime::GetLocalTime()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    
    return ITime(static_cast<uint32_t>(tm.tm_year + 1900),
                 static_cast<uint32_t>(tm.tm_mon + 1),
                 static_cast<uint32_t>(tm.tm_mday),
                 static_cast<uint32_t>(tm.tm_hour),
                 static_cast<uint32_t>(tm.tm_min),
                 static_cast<uint32_t>(tm.tm_sec),
                 static_cast<uint32_t>(ts.tv_nsec / 1000));
}

ITime ITime::GetUTCTime()
{
    return ITime(GetUTCTSecond());
}

uint64_t ITime::GetUTCTSecond()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec);
}

int32_t ITime::SetUTCTime(ITime *pTime)
{
    if (unlikely(pTime == nullptr)) 
    {
        return ErrorCode::kInvalidParam;
    }
    
    struct tm tm;
    tm.tm_year = static_cast<int>(pTime->uYear) - 1900;
    tm.tm_mon = static_cast<int>(pTime->uMonth) - 1;
    tm.tm_mday = static_cast<int>(pTime->uDay);
    tm.tm_hour = static_cast<int>(pTime->uHour);
    tm.tm_min = static_cast<int>(pTime->uMinute);
    tm.tm_sec = static_cast<int>(pTime->uSecond);
    tm.tm_isdst = -1;

    return SetUTCTime(timegm(&tm));
}

int32_t ITime::SetUTCTime(uint64_t uTimeStampSecond)
{
    timespec ts;
    ts.tv_sec = uTimeStampSecond;
    ts.tv_nsec = 0;
    if (likely(clock_settime(CLOCK_REALTIME, &ts) == 0))
    {
        return 0;
    }
    return ErrorCode::kSysCallFailed;
}

int32_t ITime::GetTimeZone()
{
    return timezone;
}

int32_t ITime::SetTimeZone(int32_t iTimeZone)
{
    (void)iTimeZone;
    return ErrorCode::kInvalidCall;
}

int32_t ITime::IsDst()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    return tm.tm_isdst;
}

}
}
