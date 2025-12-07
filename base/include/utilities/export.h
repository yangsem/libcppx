#ifndef __CPPX_EXPORT_H__
#define __CPPX_EXPORT_H__

#if defined(__WIN32) || defined(__WIN64)
#define OS_WIN
#elif defined(__APPLE__)
#define OS_MAC
#elif defined(__linux__)
#define OS_LINUX
#else
#error "Unsupported OS"
#endif


#ifndef OS_WIN
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT __declspec(dllexport)
#endif

#endif //__CPPX_EXPORT_H__
