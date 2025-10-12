#ifndef __CPPX_TIME_H__
#define __CPPX_TIME_H__

#include <stdint.h>
#include <utilities/cppx_export.h>

namespace cppx
{
namespace base
{

class EXPORT ITime
{
public:
    uint32_t uYear{0};      // 年，[0-至今]
    uint32_t uMonth{0};     // 月，[1-12]
    uint32_t uDay{0};       // 日，[1-31]
    uint32_t uHour{0};      // 时，[0-23]
    uint32_t uMinute{0};    // 分，[0-59]
    uint32_t uSecond{0};    // 秒, [0-59]
    uint32_t uMicro{0};     // 微秒，[0-999999]
    uint32_t uReserved;     // 无意义对齐字段

public:
    ITime() noexcept = default;
    ITime(const ITime &) noexcept;

    ITime(uint32_t uYear, uint32_t uMonth, uint32_t uDay,
          uint32_t uHour = 0, uint32_t uMinute = 0, uint32_t uSecond = 0,
          uint32_t uMicro = 0) noexcept;
    
    /**
     * @brief UTC时间戳构造函数
     * @param uTimeStampSecond 时间戳（秒）
     * 
     * @note 该方法不考虑时区、夏令时等影响, 表示UTC+0时区的时间戳
     */ 
    ITime(uint64_t uTimeStampSecond) noexcept;

    ITime &operator=(const ITime &) noexcept;

    /**
     * @brief UTC时间戳赋值函数
     * @param uTimeStampSecond 时间戳（秒）
     * 
     * @note 该方法不考虑时区、夏令时等影响, 表示UTC+0时区的时间戳
     */ 
    ITime &operator=(uint64_t uTimeStampSecond) noexcept;

    ~ITime() noexcept = default;

    /**
     * @brief 将当前时间转换成UTC时间戳（秒）
     * @return 时间戳（秒）
     * 
     * @note 该方法不考虑时区、夏令时等影响
     */ 
    uint64_t GetTimeStampSecond() const noexcept;

    /**
     * @brief 将当前时间转换成UTC时间戳（毫秒）
     * @return 时间戳（毫秒）
     * 
     * @note 该方法不考虑时区、夏令时等影响
     */ 
    uint64_t GetTimeStampMill() const noexcept;

    /**
     * @brief 将当前时间转换成UTC时间戳（微秒）
     * @return 时间戳（微秒）
     * 
     * @note 该方法不考虑时区、夏令时等影响
     */ 
    uint64_t GetTimeStampMicro() const noexcept;

    bool operator<(const ITime &) const noexcept;
    bool operator>(const ITime &) const noexcept;
    bool operator<=(const ITime &) const noexcept;
    bool operator>=(const ITime &) const noexcept;
    bool operator==(const ITime &) const noexcept;
    bool operator!=(const ITime &) const noexcept;

    ITime &operator+(const ITime &) noexcept;
    ITime &operator-(const ITime &) noexcept;
    ITime &operator+=(const ITime &) noexcept;
    ITime &operator-=(const ITime &) noexcept;

    ITime operator+(uint64_t uTimeStampSecond) noexcept;
    ITime operator-(uint64_t uTimeStampSecond) noexcept;
    ITime &operator+=(uint64_t uTimeStampSecond) noexcept;
    ITime &operator-=(uint64_t uTimeStampSecond) noexcept;

    /**
     * @brief 系列函数以秒为单位进行加减操作
     * @return 加1秒后的时间
     * 
     * @note 该方法不考虑时区、夏令时等影响
     */ 
    ITime &operator++() noexcept;
    ITime &operator--() noexcept;
    ITime operator++(int) noexcept;
    ITime operator--(int) noexcept;
    
    /**
     * @brief 获取本地时间
     * @return 本地时间
     * 
     * @note 该方法考虑时区、夏令时等影响
     */ 
    static ITime GetLocalTime() noexcept;

    /**
     * @brief 获取UTC时间
     * @return UTC时间
     * 
     * @note 该方法不考虑时区、夏令时等影响, 表示UTC+0时区的时间
     */ 
    static ITime GetUTCTime() noexcept;

    /**
     * @brief 获取UTC时间戳（秒）
     * @return UTC时间戳（秒）
     * 
     * @note 该方法不考虑时区、夏令时等影响, 表示UTC+0时区的时间戳
     */ 
    static uint64_t GetUTCTSecond() noexcept;

    /**
     * @brief 设置UTC时间
     * @param pTime 时间
     * @return 成功返回0，否则返回错误码
     */ 
    static int32_t SetUTCTime(ITime *pTime) noexcept;

    /**
     * @brief 设置UTC时间
     * @param uTimeStampSecond 时间戳（秒）
     * @return 成功返回0，否则返回错误码
     */ 
    static int32_t SetUTCTime(uint64_t uTimeStampSecond) noexcept;

    /**
     * @brief 获取时区
     * @return 时区
     */ 
    static int32_t GetTimeZone() noexcept;

    /**
     * @brief 设置时区
     * @param iTimeZone 时区
     * @return 成功返回0，否则返回错误码
     */ 
    static int32_t SetTimeZone(int32_t iTimeZone) noexcept;

    /**
     * @brief 判断是否是夏令时
     * @return 是否是夏令时，0表示不是夏令时，1表示是夏令时，-1表示错误
     */ 
    static int32_t IsDst() noexcept;
};

}
}

#endif // __CPPX_TIME_H__