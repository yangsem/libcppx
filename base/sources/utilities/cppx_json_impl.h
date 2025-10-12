#ifndef __CPPX_JSON_IMPL_H__
#define __CPPX_JSON_IMPL_H__

#include <utilities/cppx_json.h>
#include <json/json.h>

namespace cppx
{
namespace base
{

class CJsonImpl final : public IJson
{
public:
    CJsonImpl(JsonType jsonType = JsonType::kJsonTypeObject) noexcept;
    CJsonImpl(const Json::Value &&jsonValue);
    ~CJsonImpl() noexcept override;

    CJsonImpl(const CJsonImpl &) = delete;
    CJsonImpl &operator=(const CJsonImpl &) = delete;
    CJsonImpl &operator=(CJsonImpl &&) = delete;

    int32_t Parse(const char *pJsonStr) noexcept override;
    int32_t ParseFile(const char *pJsonFile) noexcept override;

    IJson::JsonGuard GetObject(const char *pKey) const noexcept override;
    IJson::JsonGuard GetArray(const char *pKey) const noexcept override;
    const char *GetString(const char *pKey, const char *pDefault = nullptr) const noexcept override;
    int32_t GetInt(const char *pKey, int32_t iDefault = 0) const noexcept override;
    bool GetBool(const char *pKey, bool bDefault = false) const noexcept override;

    int32_t SetObject(const char *pKey, IJson *pJson) noexcept override;
    int32_t SetArray(const char *pKey, IJson *pJson) noexcept override;
    int32_t SetString(const char *pKey, const char *pValue) noexcept override;
    int32_t SetInt(const char *pKey, int32_t iValue) noexcept override;
    int32_t SetBool(const char *pKey, bool bValue) noexcept override;

    IJson::JsonGuard GetObject(int32_t iIndex) const noexcept override;
    IJson::JsonGuard GetArray(int32_t iIndex) const noexcept override;
    const char *GetString(int32_t iIndex, const char *pDefault = nullptr) const noexcept override;
    int32_t GetInt(int32_t iIndex, int32_t iDefault = 0) const noexcept override;
    bool GetBool(int32_t iIndex, bool bDefault = false) const noexcept override;

    int32_t AppendObject(IJson *pJson) noexcept override;
    int32_t AppendArray(IJson *pJson) noexcept override;
    int32_t AppendString(const char *pValue) noexcept override;
    int32_t AppendInt(int32_t iValue) noexcept override;
    int32_t AppendBool(bool bValue) noexcept override;

    void Delete(const char *pKey) noexcept override;
    void Clear() noexcept override;

    JsonStrGuard ToString(bool bPretty = false) const noexcept override;
    JsonType GetType(const char *pKey = nullptr) const noexcept override;
    JsonType GetType(int32_t iIndex) const noexcept override;
    uint32_t GetSize() const noexcept override;

private:
    Json::Value m_jsonValue;
};

}
}

#endif // __CPPX_JSON_IMPL_H__