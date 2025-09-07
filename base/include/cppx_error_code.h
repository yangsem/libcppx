#ifndef __CPPX_ERROR_CODE_H__
#define __CPPX_ERROR_CODE_H__

namespace cppx
{
namespace base
{

enum ErrorCode
{
    kSuccess = 0,
    kNotSupported,
    kNoMemory,
    
    kInvalidParam = 100,
    kThrowException,
    kInvalidCall,
    kSysCallFailed,
};

}
}

#endif // __CPPX_ERROR_CODE_H__
