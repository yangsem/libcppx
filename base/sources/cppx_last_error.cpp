#include "cppx_last_error.h"
#include "cppx_common.h"

namespace cppx
{
namespace base
{

static int32_t & GetLastErrorVar()
{
    static thread_local int32_t tls_iLastErrorNo = 0;
    return tls_iLastErrorNo;
}

static std::string & GetLastErrorStrVar()
{
    static thread_local std::string tls_strLastError;
    return tls_strLastError;
}

LastError::LastError()
{
}

LastError::~LastError()
{
}

int32_t LastError::SetLastError(int32_t iErrorNo, const char *pErrorStr)
{
    auto &iLastErrorNo = GetLastErrorVar();
    iLastErrorNo = iErrorNo;
    try
    {
        if (likely(pErrorStr != nullptr))
        {
            auto &strLastError = GetLastErrorStr();
            strLastError = pErrorStr;
        }
        else
        {
            auto &strLastError = GetLastErrorStr();
            strLastError = "unknown error " + std::to_string(iErrorNo);
        }
    }
    catch (...)
    {
        return -1;
    }
    return 0;
}

int32_t LastError::GetLastError()
{
    return GetLastErrorVar();
}

std::string & LastError::GetLastErrorStr()
{
    return GetLastErrorStrVar();
}

}
}