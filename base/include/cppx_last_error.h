#ifndef __CPPX_LAST_ERROR_H__
#define __CPPX_LAST_ERROR_H__

#include <stdint.h>
#include <cppx_export.h>

#ifndef LAST_ERROR_BUFFER_SIZE
#define LAST_ERROR_BUFFER_SIZE 256
#endif
#define SET_LAST_ERROR(iErrorNo, szFormat, ...)                      \
    {                                                                \
        char szError[LAST_ERROR_BUFFER_SIZE];                        \
        snprintf(szError, sizeof(szError), szFormat, ##__VA_ARGS__); \
        cppx::base::LastError::SetLastError(iErrorNo, szError);      \
    }

namespace cppx
{
namespace base
{

class EXPORT LastError
{
    LastError();
    ~LastError();

public:
    /**
     * @brief Set the last error code and optional error message
     * @param iErrorNo Error code to set
     * @param pErrorStr Optional error message string
     * @return 0 on success, negative value on failure
     * 
     * @note This function is thread-safe and sets the error code for the calling thread
    */
    static int32_t SetLastError(int32_t iErrorNo, const char *pErrorStr = nullptr);

    /**
     * @brief Get the last error code
     * @return The last error code
     * 
     * @note This function is thread-safe and returns the error code for the calling thread
    */
    static int32_t GetLastError();

    /**
     * @brief Get the last error message string
     * @param iErrorNo Error code for which to get the message
     * @return Reference to a string containing the error message
     * 
     * @note This function is thread-safe and returns the error message for the calling thread
    */
    static const char *GetLastErrorStr();
};

}
}

#endif // __CPPX_LAST_ERROR_H__
