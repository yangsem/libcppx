#ifndef __CPPX_JSON_IMPL_H__
#define __CPPX_JSON_IMPL_H__

#include "cppx_json.h"
#include <json/json.h>

namespace cppx
{
namespace base
{

class CJsonImpl final : public IJson
{
public:
    CJsonImpl() = default;
    ~CJsonImpl() noexcept override;

    CJsonImpl(const CJsonImpl &) = delete;
    CJsonImpl &operator=(const CJsonImpl &) = delete;
    CJsonImpl(CJsonImpl &&) = delete;
    CJsonImpl &operator=(CJsonImpl &&) = delete;

    int32_t Parse(const char *pJsonStr) noexcept override;
    int32_t ParseFile(const char *pJsonFile) noexcept override;
    IJson *GetObject(const char *pKey) noexcept override;
    IJson *GetArray(const char *pKey) noexcept override;
    const char *GetString(const char *pKey, const char *pDefault = nullptr) noexcept override;
    int32_t GetInt(const char *pKey, int32_t iDefault = 0) noexcept override;
    bool GetBool(const char *pKey, bool bDefault = false) noexcept override;
    int32_t SetObject(const char *pKey, IJson *pJson) noexcept override;
    int32_t SetArray(const char *pKey, IJson *pJson) noexcept override;
    int32_t SetString(const char *pKey, const char *pValue) noexcept override;
    int32_t SetInt(const char *pKey, int32_t iValue) noexcept override;
    int32_t SetBool(const char *pKey, bool bValue) noexcept override;
    const char *ToString(bool bPretty = false) noexcept override;
    JsonType GetType(const char *pKey = nullptr) noexcept override;

private:
    Json::Value m_jsonValue;
};

}
}

#endif // __CPPX_JSON_IMPL_H__