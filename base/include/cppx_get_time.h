#ifndef __CPPX_GET_TIME_H__
#define __CPPX_GET_TIME_H__

#include <stdint.h>

namespace cppx
{
namespace base
{

class GetTimeTool
{
    GetTimeTool();
    ~GetTimeTool();

public:
    static inline uint64_t GetCurrentNano();

    static inline uint64_t GetCurrentMicro();

    static inline uint64_t GetCurrentMill();

    static inline uint64_t GetCurrentSecond();
};

}
}

#endif //__CPPX_GET_TIME_H__