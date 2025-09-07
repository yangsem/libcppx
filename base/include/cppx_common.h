#ifndef __CPPX_COMMON_H__
#define __CPPX_COMMON_H__

#if defined(__WIN32) || defined(__WIN64)
#define OS_WIN
#else
#define OS_LINUX
#endif

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#ifndef OS_WIN
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define UNSED(x) ((void)(x))
#define ACCESS_ONCE(x) (*(volatile decltype(x) *)&(x))


#ifdef __cplusplus
#include <new>
#define NEW new (std::nothrow)
#endif

#ifndef OS_WIN
#include <sys/syscall.h>
#define getpid() getpid()
#define gettid() syscall(SYS_gettid)
#define set_thread_name(name) pthread_setname_np(pthread_self(), name)
#define thread_bind_cpu(cpu_no)                                             \
    do                                                                      \
    {                                                                       \
        cpu_set_t cpuset;                                                   \
        CPU_ZERO(&cpuset);                                                  \
        CPU_SET(cpu_no, &cpuset);                                           \
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset); \
    } while (0)
#else
#include <windows.h>
#define getpid() GetCurrentProcessId()
#define gettid() GetCurrentThreadId()
#define set_thread_name(name) // not supported in windows
#define thread_bind_cpu(cpu_no) // not supported in windows
#endif

#ifndef OS_WIN
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT __declspec(dllexport)
#endif

#define CACHE_LINE 64
#define ALIGN_AS_CACHELINE __attribute__((aligned(CACHE_LINE)))

#define ALIGNN(num, base) (((num) + (base)) & ~(base))
#define ALIGN8(num) ALIGNN(num, 7)
#define ALIGN64(num) ALIGNN(num, 63)

#define MAX_NAME_LEN 128
#define MAX_FILE_LEN 256
#define MAX_PATH_LEN 1024

#ifdef OS_LINUX
#define RESET "\033[0m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define GREEN "\033[0;32m"
#else
#define RESET
#define RED
#define GREEN
#define YELLOW
#define BLUE
#endif

#define TO_STR1(x) #x
#define TO_STR2(x) TO_STR1(x)
#define __POSITION__ __FUNCTION__, __FILE__ ":" TO_STR2(__LINE__)

#define PRINT_BASE(channel, format, ...)              \
    do                                                \
    {                                                 \
        fprintf(channel, format "\n", ##__VA_ARGS__); \
    } while (0)

#define PRINT_INFO(format, ...) PRINT_BASE(stdout, format "(%s,%s)", ##__VA_ARGS__, __POSITION__)
#define PRINT_WARN(format, ...) PRINT_BASE(stdout, GREEN format "(%s,%s)" RESET, ##__VA_ARGS__, __POSITION__)
#define PRINT_ERROR(format, ...) PRINT_BASE(stderr, RED format "(%s,%s)" RESET, ##__VA_ARGS__, __POSITION__)
#define PRINT_FAIL(format, ...) PRINT_BASE(stderr, RED format "(%s,%s)" RESET, ##__VA_ARGS__, __POSITION__)

#endif // __CPPX_COMMON_H__