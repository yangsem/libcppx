#include <utilities/error_code.h>

namespace cppx
{
namespace base
{

static thread_local ErrorCode tls_eErrorCode = ErrorCode::kSuccess;

ErrorCode GetLastError()
{
    return tls_eErrorCode;
}

void SetLastError(ErrorCode eErrorCode)
{
    tls_eErrorCode = eErrorCode;
}

}
}