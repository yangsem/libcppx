#include <utilities/error_code.h>

namespace cppx
{
namespace base
{

static thread_local ErrorCode tls_eErrorCode = ErrorCode::kSuccess;

ErrorCode GetLastError() noexcept
{
    return tls_eErrorCode;
}

void SetLastError(ErrorCode eErrorCode) noexcept
{
    tls_eErrorCode = eErrorCode;
}

}
}