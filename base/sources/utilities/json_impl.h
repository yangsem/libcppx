#ifndef __CPPX_JSON_IMPL_H__
#define __CPPX_JSON_IMPL_H__

#include <vector>
#include <json/json.h>
#include <utilities/json.h>

namespace cppx
{
namespace base
{

class CJsonImpl final : public IJson
{
public:
    CJsonImpl(JsonType jsonType = JsonType::kObject);
    CJsonImpl(Json::Value &jsonValue, bool bRoot);
    CJsonImpl(const Json::Value &&jsonValue);
    ~CJsonImpl() noexcept override;

    CJsonImpl(const CJsonImpl &) = delete;
    CJsonImpl &operator=(const CJsonImpl &) = delete;
    CJsonImpl &operator=(CJsonImpl &&) = delete;

    int32_t Parse(const char *pJsonStr) noexcept override;
    int32_t ParseFile(const char *pJsonFile) noexcept override;

    bool GetBool(const char *pKey, bool bDefault = false) const noexcept override;
    int32_t GetInt32(const char *pKey, int32_t iDefault = 0) const noexcept override;
    int64_t GetInt64(const char *pKey, int64_t iDefault = 0) const noexcept override;
    uint32_t GetUint32(const char *pKey, uint32_t uDefault = 0) const noexcept override;
    uint64_t GetUint64(const char *pKey, uint64_t uDefault = 0) const noexcept override;
    double GetDouble(const char *pKey, double dDefault = 0.0) const noexcept override;
    const char *GetString(const char *pKey, const char *pDefault = nullptr) const noexcept override;
    const IJson *GetObject(const char *pKey) const noexcept override;
    int32_t GetObject(const char *pKey, IJson *pJson) const noexcept override;
    const IJson *GetArray(const char *pKey) const noexcept override;
    int32_t GetArray(const char *pKey, IJson *pJson) const noexcept override;

    int32_t SetBool(const char *pKey, bool bValue) noexcept override;
    int32_t SetInt32(const char *pKey, int32_t iValue) noexcept override;
    int32_t SetInt64(const char *pKey, int64_t iValue) noexcept override;
    int32_t SetUint32(const char *pKey, uint32_t uValue) noexcept override;
    int32_t SetUint64(const char *pKey, uint64_t uValue) noexcept override;
    int32_t SetDouble(const char *pKey, double dValue) noexcept override;
    int32_t SetString(const char *pKey, const char *pValue) noexcept override;
    int32_t SetObject(const char *pKey, IJson *pJson) noexcept override;
    IJson *SetObject(const char *pKey) noexcept override;
    int32_t SetArray(const char *pKey, IJson *pJson) noexcept override;
    IJson *SetArray(const char *pKey) noexcept override;

    bool GetBool(uint32_t iIndex, bool bDefault = false) const noexcept override;
    int32_t GetInt32(uint32_t iIndex, int32_t iDefault = 0) const noexcept override;
    int64_t GetInt64(uint32_t iIndex, int64_t iDefault = 0) const noexcept override;
    uint32_t GetUint32(uint32_t iIndex, uint32_t uDefault = 0) const noexcept override;
    uint64_t GetUint64(uint32_t iIndex, uint64_t uDefault = 0) const noexcept override;
    double GetDouble(uint32_t iIndex, double dDefault = 0.0) const noexcept override;
    const char *GetString(uint32_t iIndex, const char *pDefault = nullptr) const noexcept override;
    const IJson *GetObject(uint32_t iIndex) const noexcept override;
    int32_t GetObject(uint32_t iIndex, IJson *pJson) const noexcept override;
    const IJson *GetArray(uint32_t iIndex) const noexcept override;
    int32_t GetArray(uint32_t iIndex, IJson *pJson) const noexcept override;

    int32_t AppendBool(bool bValue) noexcept override;
    int32_t AppendInt32(int32_t iValue) noexcept override;
    int32_t AppendInt64(int64_t iValue) noexcept override;
    int32_t AppendUint32(uint32_t uValue) noexcept override;
    int32_t AppendUint64(uint64_t uValue) noexcept override;
    int32_t AppendDouble(double dValue) noexcept override;
    int32_t AppendString(const char *pValue) noexcept override;
    int32_t AppendObject(IJson *pJson) noexcept override;
    IJson *AppendObject() noexcept override;
    int32_t AppendArray(IJson *pJson) noexcept override;
    IJson *AppendArray() noexcept override;

    void Delete(const char *pKey) noexcept override;
    void Clear() noexcept override;

    const char *ToString(bool bPretty = false) const noexcept override;
    uint32_t GetSize() const noexcept override;
    JsonType GetType(const char *pKey = nullptr) const noexcept override;
    JsonType GetType(uint32_t iIndex) const noexcept override;

private:
    template<typename T>
    T GetValue(const char *pKey, T defaultValue) const noexcept;
    template<typename T>
    int32_t SetValue(const char *pKey, T &value) noexcept;
    template<typename T>
    T GetValue(uint32_t iIndex, T defaultValue) const noexcept;
    template<typename T>
    int32_t AppendValue(T &value) noexcept;

private:
    Json::Value m_jsonValueData;  // 当前对象为根对象时有效
    Json::Value &m_jsonValue;     // 根对象指向m_jsonValueData，否则指向m_jsonValueData的子对象
    mutable bool m_bRoot{true};
    mutable std::vector<CJsonImpl *> m_vecJsonValues;
    mutable std::string m_strJsonString;
};

}
}

#endif // __CPPX_JSON_IMPL_H__