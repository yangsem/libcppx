# Linux 时区设置函数源码分析

## 问题背景

系统环境：

-  `aarch64-none-linux-gnu` 
- ubuntu 版本：
- glibc 版本：
- Linux kernel 版本：

问题代码：

A进程调用接口设置时区：

```c
int32_t setTimezone(int second)
{
		struct timeval tv;
		struct timezone tz;
		// 以下调用忽略错误处理
		setenv("TZ", getTimezoneStr());
		gettimeofday(&tv, tz);
		tz.tz_minuteswest = -(second / 60);
		settimeofday(&tv, &tz);
		return 0;
}
```

然后再B进程通过两中方式获取去本地时间：

```c++
// 方式1：
struct tm getLocatime()
{
		struct timeval tv;
		struct timezone tz;
		// 以下调用忽略错误处理
		gettimeofday(&tv, &tz);
		struct tm now;
		time_t second = tv.tv_sec + (-tz.tz_minuteswest * 60);
		gmtime_r(&second, &now);
		return now;
}

// 方式2：
struct tm getLocaltime()
{
		time_t second;
		struct tm now;
		time(&second);
		localtime_r(&second, &now);
		return now;
}
```

假设：

- 修改前timezone为UTC+4
- 修改后的timezone为UTC+8
- 当前UTC时间为 2025.09.22 10:00:00

进程B两者方式的获取结果：

- 方式1 获取的本地时间为：2025.09.22 18:00:00
- 方式2 获取的本地时间为：2025.09.22 14:00:00

**这里可以发现问题：**

- **使用settimeofday修改时区对localtime_r的方式不生效**

## 问题分析

### 1. 查看 man page 接口描述

- gettimeofday 和 settimeofday 用于获取和设置 UTC 时间和时区信息。

  - 但是这两个接口目前被标记为废弃，有替代接口clock_gettime和clock_settime
  - 不应该再使用gettimeofday 和 settimeofday 获取和设置时区
  - 对于gettimeofday的timezone参数，在不同的平台表现会不一样，可以返回0值，可能返回有效值
  - 对于settimeofday的timezone参数，在不同的平台表现也也不一样，可能返回失败，也可能设置成功

- localtime_r 用于将UTC秒数转换成包含timezone的本地时间

  ```
  localtime() 函数把日历时间 timep 转换为分解时间表示，采用用户指定的时区。  
  它的行为就像调用了 tzset(3)，并设置外部变量 tzname（当前时区的信息）、timezone（UTC 与本地标准时间的差值，单位为秒），  以及 daylight（如果一年中的某些时间适用夏令时，则设为非零）
  ```

- 进一步询问 AI 修改时区相关的问题，修改时区需要结合 tzset() 重新加载 `"TZ"` 环境变量或者时区文件`/etc/localtime`

根据这里的描述以及问题的现象，可以发现问题应该是出在timezone的维护上：

- 如果是通过修改环境变量，这个变化是无法跨越非亲缘关系的进程，那么方式1理论上是不能生效的，这里矛盾
- 如果是通过时区文件调整，那么B进程的方式1如何感知时区文件变化的？方式2该如何修改？

这里就需要进一步分析glibc的源码实现

### 2. glibc 源码分析

- 通过 `ldd --version` 查看 glibc的版本

- 询问 AI glibc的源码可以在哪里看，得到两份地址：

  - [glibc在线查看](https://elixir.bootlin.com/glibc)
  - [glibc源码ftp下载](https://ftp.gnu.org/gnu/glibc/)

#### 2.1 gettimeofday

这里通过在线源码直接搜索 `__gettimeofday`函数，可以发现在glibc/sysdeps下有多个实现版本

- 接着询问 AI glibc 如何选择 sysdeps下的函数实现，得出结果：通过 `make print-sysdir`可以得到函数实现的路径选择优先级：
  
- 接着我们查看对应路径下的源码实现，发现实际上是直接进行了vdso的函数调用
  ```c
  #include <errno.h>
  #include <sys/time.h>
  
  #undef __gettimeofday
  
  #include <bits/libc-vdso.h>
  
  /* Get the current time of day and timezone information,
     putting it into *tv and *tz.  If tz is null, *tz is not filled.
     Returns 0 on success, -1 on errors.  */
  int
  __gettimeofday (tv, tz)
       struct timeval *tv;
       struct timezone *tz;
  {
    return INLINE_VSYSCALL (gettimeofday, 2, tv, tz);
  }
  libc_hidden_def (__gettimeofday)
  weak_alias (__gettimeofday, gettimeofday)
  libc_hidden_weak (gettimeofday)
  ```

- 然后我们再询问 AI 这里最终会调用到哪里去？最终找打Linux内核源码的`__cvdso_gettimeofday_data`
  
  ```c
  static __maybe_unused int
  __cvdso_gettimeofday_data(const struct vdso_time_data *vd,
  			  struct __kernel_old_timeval *tv, struct timezone *tz)
  {
  	const struct vdso_clock *vc = vd->clock_data;
  
  	if (likely(tv != NULL)) {
  		struct __kernel_timespec ts;
  
  		if (!do_hres(vd, &vc[CS_HRES_COARSE], CLOCK_REALTIME, &ts))
  			return gettimeofday_fallback(tv, tz);
  
  		tv->tv_sec = ts.tv_sec;
  		tv->tv_usec = (u32)ts.tv_nsec / NSEC_PER_USEC;
  	}
  
  	if (unlikely(tz != NULL)) {
  		if (IS_ENABLED(CONFIG_TIME_NS) &&
  		    vc->clock_mode == VDSO_CLOCKMODE_TIMENS)
  			vd = __arch_get_vdso_u_timens_data(vd);
  
  		tz->tz_minuteswest = vd[CS_HRES_COARSE].tz_minuteswest;
  		tz->tz_dsttime = vd[CS_HRES_COARSE].tz_dsttime;
  	}
  
  	return 0;
  }
  
  static __maybe_unused int
  __cvdso_gettimeofday(struct __kernel_old_timeval *tv, struct timezone *tz)
  {
  	return __cvdso_gettimeofday_data(__arch_get_vdso_u_time_data(), tv, tz);
  }
  ```
  
  最终的结论是 gettimeofday 会通过vdso机制访问内核的 `vdso_u_time_data`得到timezone
  
  ```c
  // include/vdso/datapage.h
  
  extern struct vdso_time_data vdso_u_time_data __attribute__((visibility("hidden")));
  extern struct vdso_rng_data vdso_u_rng_data __attribute__((visibility("hidden")));
  extern struct vdso_arch_data vdso_u_arch_data __attribute__((visibility("hidden")));
  ```
  
#### 2.2 settimeofday

使用 `gettimeofday` 一样的方式进行代码查找，找到对应平台下的glibc实现

```
```

接着询问AI在Linux/kernel下的函数实现：

```c
/*
 * In case for some reason the CMOS clock has not already been running
 * in UTC, but in some local time: The first time we set the timezone,
 * we will warp the clock so that it is ticking UTC time instead of
 * local time. Presumably, if someone is setting the timezone then we
 * are running in an environment where the programs understand about
 * timezones. This should be done at boot time in the /etc/rc script,
 * as soon as possible, so that the clock can be set right. Otherwise,
 * various programs will get confused when the clock gets warped.
 */

int do_sys_settimeofday64(const struct timespec64 *tv, const struct timezone *tz)
{
	static int firsttime = 1;
	int error = 0;

	if (tv && !timespec64_valid_settod(tv))
		return -EINVAL;

	error = security_settime64(tv, tz);
	if (error)
		return error;

	if (tz) {
		/* Verify we're within the +-15 hrs range */
		if (tz->tz_minuteswest > 15*60 || tz->tz_minuteswest < -15*60)
			return -EINVAL;

		sys_tz = *tz;
		update_vsyscall_tz();
		if (firsttime) {
			firsttime = 0;
			if (!tv)
				timekeeping_warp_clock();
		}
	}
	if (tv)
		return do_settimeofday64(tv);
	return 0;
}

void update_vsyscall_tz(void)
{
	struct vdso_time_data *vdata = vdso_k_time_data;

	vdata->tz_minuteswest = sys_tz.tz_minuteswest;
	vdata->tz_dsttime = sys_tz.tz_dsttime;

	__arch_sync_vdso_time_data(vdata);
}
```

最终得出结论：settimeofday 会将时区信息写在Linux内核vdso的 `vdso_k_time_data`中

```c
// inclde/vdso/datapage.h

#ifdef CONFIG_GENERIC_VDSO_DATA_STORE
extern struct vdso_time_data vdso_u_time_data __attribute__((visibility("hidden")));
extern struct vdso_rng_data vdso_u_rng_data __attribute__((visibility("hidden")));
extern struct vdso_arch_data vdso_u_arch_data __attribute__((visibility("hidden")));

extern struct vdso_time_data *vdso_k_time_data;
extern struct vdso_rng_data *vdso_k_rng_data;
extern struct vdso_arch_data *vdso_k_arch_data;
```

到这里还是两个不同的变量，接着询问AI这两个变量的关系：

vdso_k_time_data 是内核中的一个数据存储，通过vdso机制，可以在用户进程中将vdso_u_time_data的虚拟地址映射为只读的物理地址，指向vdso_k_time_data。

到此完成settimeofday和gettimeofday的完整闭环。

#### 2.3 localtime_r

如法炮制，查找localtime_r的源码实现。其中最关键的流程是：

- 再进程首次调用时，会去读取一次 `TZ` 环境变量的值，然后解析出对应的时区信息保存在glibc的全局变量中
- 如果不存在环境变量则去读取时区文件，并解析加载时区信息保存到全局变量
- 接着将用户传入的UTC秒数加上时区偏移得到最终的本地时间

```c
struct tm *
__localtime_r (t, tp)
     const time_t *t;
     struct tm *tp;
{
  return __tz_convert (t, 1, tp);
}
weak_alias (__localtime_r, localtime_r)

/* Return the `struct tm' representation of *TIMER in the local timezone.
   Use local time if USE_LOCALTIME is nonzero, UTC otherwise.  */
struct tm *
__tz_convert (const time_t *timer, int use_localtime, struct tm *tp)
{
  long int leap_correction;
  int leap_extra_secs;

  if (timer == NULL)
    {
      __set_errno (EINVAL);
      return NULL;
    }

  __libc_lock_lock (tzset_lock);

  /* Update internal database according to current TZ setting.
     POSIX.1 8.3.7.2 says that localtime_r is not required to set tzname.
     This is a good idea since this allows at least a bit more parallelism.  */
  tzset_internal (tp == &_tmbuf && use_localtime, 1);

  if (__use_tzfile)
    __tzfile_compute (*timer, use_localtime, &leap_correction,
		      &leap_extra_secs, tp);
  else
    {
      if (! __offtime (timer, 0, tp))
	tp = NULL;
      else
	__tz_compute (*timer, tp, use_localtime);
      leap_correction = 0L;
      leap_extra_secs = 0;
    }

  if (tp)
    {
      if (! use_localtime)
	{
	  tp->tm_isdst = 0;
	  tp->tm_zone = "GMT";
	  tp->tm_gmtoff = 0L;
	}

      if (__offtime (timer, tp->tm_gmtoff - leap_correction, tp))
        tp->tm_sec += leap_extra_secs;
      else
	tp = NULL;
    }

  __libc_lock_unlock (tzset_lock);

  return tp;
}

/* Interpret the TZ envariable.  */
static void
internal_function
tzset_internal (always, explicit)
     int always;
     int explicit;
{
  static int is_initialized;
  const char *tz;

  if (is_initialized && !always)
    return;
  is_initialized = 1;

  /* Examine the TZ environment variable.  */
  tz = getenv ("TZ");
  if (tz == NULL && !explicit)
    /* Use the site-wide default.  This is a file name which means we
       would not see changes to the file if we compare only the file
       name for change.  We want to notice file changes if tzset() has
       been called explicitly.  Leave TZ as NULL in this case.  */
    tz = TZDEFAULT;
  if (tz && *tz == '\0')
    /* User specified the empty string; use UTC explicitly.  */
    tz = "Universal";

  /* A leading colon means "implementation defined syntax".
     We ignore the colon and always use the same algorithm:
     try a data file, and if none exists parse the 1003.1 syntax.  */
  if (tz && *tz == ':')
    ++tz;

  /* Check whether the value changed since the last run.  */
  if (old_tz != NULL && tz != NULL && strcmp (tz, old_tz) == 0)
    /* No change, simply return.  */
    return;

  if (tz == NULL)
    /* No user specification; use the site-wide default.  */
    tz = TZDEFAULT;

  tz_rules[0].name = NULL;
  tz_rules[1].name = NULL;

  /* Save the value of `tz'.  */
  free (old_tz);
  old_tz = tz ? __strdup (tz) : NULL;

  /* Try to read a data file.  */
  __tzfile_read (tz, 0, NULL);
  if (__use_tzfile)
    return;

  /* No data file found.  Default to UTC if nothing specified.  */

  if (tz == NULL || *tz == '\0'
      || (TZDEFAULT != NULL && strcmp (tz, TZDEFAULT) == 0))
    {
      memset (tz_rules, '\0', sizeof tz_rules);
      tz_rules[0].name = tz_rules[1].name = "UTC";
      if (J0 != 0)
	tz_rules[0].type = tz_rules[1].type = J0;
      tz_rules[0].change = tz_rules[1].change = (time_t) -1;
      update_vars ();
      return;
    }

  __tzset_parse_tz (tz);
}
```

## 问题解决

### 1. 源码分析总结

- 内核和glibc分别维护了一套时区的信息，因此混用会导致功能异常
- 根据相关信息表明，未来内核不在维护时区信息，只维护UTC时间
- 根据glibc的描述，gettimeofday和settimeofday接口应该废弃，使用替代方案：getclock和setclock
- 时区信息由glibc维护，那么修改时区后就需要重启进程才能生效，或者通过某种机制通知到各个进程，例如信号机制

### 2. 当前项目的解决方案

当前项目现状：

- 组件A 通过gettimeofday和settimeofday管理时间时区等的获取和设置
- 组件B可能存在大量使用localtime_r类似的glibc接口，替换为组件A的接口可能相对困难
- 当前项目采用多进程的方式，且除了组件B存在历史代码，其他服务进程都是新代码实现可以直接采用组件A提供的接口

**因此，可以考虑在组件B调用组件A的设置时区接口将时区写入内核，然后再将时区信息通过tzset的方式写入glibc。那么所有的服务进程都可以得到相同的时间和时区信息。**
