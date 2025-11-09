#include "json_impl.h"
#include <cstdint>
#include <fstream>
#include <cctype>
#include <utility>
#include <utilities/common.h>
#include <utilities/error_code.h>
#include <memory/allocator_ex.h>

namespace cppx
{
namespace base
{

static IJson::JsonType GetIJsonType(Json::ValueType jsonType)
{
    static IJson::JsonType s_jsonTypeMap[] = {
        IJson::JsonType::kInvalid,
        IJson::JsonType::kInt64,
        IJson::JsonType::kUint64,
        IJson::JsonType::kDouble,
        IJson::JsonType::kString,
        IJson::JsonType::kBool,
        IJson::JsonType::kArray,
        IJson::JsonType::kObject,
    };
    return s_jsonTypeMap[jsonType];
}

static Json::ValueType GetJsonValueType(IJson::JsonType jsonType)
{
    static Json::ValueType s_jsonValueTypeMap[] = {
        Json::ValueType::nullValue,
        Json::ValueType::booleanValue,
        Json::ValueType::intValue,
        Json::ValueType::intValue,
        Json::ValueType::uintValue,
        Json::ValueType::uintValue,
        Json::ValueType::realValue,
        Json::ValueType::stringValue,
        Json::ValueType::objectValue,
        Json::ValueType::arrayValue,
    };
    return s_jsonValueTypeMap[(uint8_t)jsonType];
}


IJson *IJson::Create(JsonType jsonType) noexcept
{
    return IAllocatorEx::GetInstance()->New<CJsonImpl>(jsonType);
}

void IJson::Destroy(IJson *pJson) noexcept
{
    if (likely(pJson != nullptr))
    {
        delete pJson;
    }
}

CJsonImpl::CJsonImpl(JsonType jsonType)
    : m_jsonValueData(GetJsonValueType(jsonType))
    , m_jsonValue(m_jsonValueData)
    , m_bRoot(true)
{
}

/**
 * 构造根对象时，拷贝jsonValue到m_jsonValueData，并指向m_jsonValueData
 * 构造子对象时，不拷贝jsonValue到m_jsonValueData，而是指向jsonValue
 */
CJsonImpl::CJsonImpl(Json::Value &jsonValue, bool bRoot)
    : m_jsonValueData(bRoot ? jsonValue : Json::objectValue)
    , m_jsonValue(bRoot ? m_jsonValueData : jsonValue)
    , m_bRoot(bRoot)
{
}

CJsonImpl::CJsonImpl(const Json::Value &&jsonValue) 
    : m_jsonValueData(jsonValue)
    , m_jsonValue(m_jsonValueData)
    , m_bRoot(true)
{
}

CJsonImpl::~CJsonImpl() noexcept
{
    for (auto pJsonValue : m_vecJsonValues)
    {
        delete pJsonValue;
    }
    m_vecJsonValues.clear();
}

int32_t CJsonImpl::Parse(const char *pJsonStr) noexcept
{
    if (unlikely(pJsonStr == nullptr))
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    try
    {
        Json::Value jsonValue;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errs;
        if (unlikely(!reader->parse(pJsonStr, pJsonStr + strlen(pJsonStr), &jsonValue, &errs)))
        {
            SetLastError(ErrorCode::kInvalidParam);
            return ErrorCode::kInvalidParam;
        }
        m_jsonValue = std::move(jsonValue);
    }
    catch (std::exception &e)
    {
        SetLastError(ErrorCode::kThrowException);
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::ParseFile(const char *pJsonFile) noexcept
{
    if (unlikely(pJsonFile == nullptr))
    {
        SetLastError(ErrorCode::kInvalidParam);
        return ErrorCode::kInvalidParam;
    }

    try
    {
        std::ifstream ifs(pJsonFile);
        if (unlikely(!ifs.is_open()))
        {
            SetLastError(ErrorCode::kInvalidParam);
            return ErrorCode::kInvalidParam;
        }
        Json::CharReaderBuilder builder;
        std::string errs;
        Json::Value jsonValue;
        if (unlikely(!Json::parseFromStream(builder, ifs, &jsonValue, &errs)))
        {
            SetLastError(ErrorCode::kInvalidParam);
            return ErrorCode::kInvalidParam;
        }
        m_jsonValue = std::move(jsonValue);
    }
    catch (std::exception &e)
    {
        SetLastError(ErrorCode::kThrowException);
        return ErrorCode::kThrowException;
    }

    return 0;
}

template<typename T>
T CJsonImpl::GetValue(const char *pKey, T defaultValue) const noexcept
{
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey)))
    {
        try
        {
            return m_jsonValue[pKey].as<T>();
        }
        catch (std::exception &e)
        {
        }
    }

    return defaultValue;
}

bool CJsonImpl::GetBool(const char *pKey, bool bDefault) const noexcept
{
    return GetValue(pKey, bDefault);
}

int32_t CJsonImpl::GetInt32(const char *pKey, int32_t iDefault) const noexcept
{
    return GetValue(pKey, iDefault);
}

int64_t CJsonImpl::GetInt64(const char *pKey, int64_t iDefault) const noexcept
{
    return GetValue(pKey, iDefault);
}

uint32_t CJsonImpl::GetUint32(const char *pKey, uint32_t uDefault) const noexcept
{
    return GetValue(pKey, uDefault);
}

uint64_t CJsonImpl::GetUint64(const char *pKey, uint64_t uDefault) const noexcept
{
    return GetValue(pKey, uDefault);
}

double CJsonImpl::GetDouble(const char *pKey, double dDefault) const noexcept
{
    return GetValue(pKey, dDefault);
}

const char *CJsonImpl::GetString(const char *pKey, const char *pDefault) const noexcept
{
    return GetValue(pKey, pDefault);
}

const IJson *CJsonImpl::GetObject(const char *pKey) const noexcept
{
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isObject()))
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto jsonValue = m_jsonValue[pKey];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(std::move(jsonValue));
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

int32_t CJsonImpl::GetObject(const char *pKey, IJson *pJson) const noexcept
{
    if (likely(pKey != nullptr && pJson != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isObject()))
    {
        auto pJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pJsonImpl != nullptr))
        {
            try
            {
                pJsonImpl->m_jsonValue = m_jsonValue[pKey];
                return 0;
            }
            catch (std::exception &e)
            {
                SetLastError(ErrorCode::kThrowException);
                return ErrorCode::kThrowException;
            }
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

const IJson *CJsonImpl::GetArray(const char *pKey) const noexcept
{
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isArray()))
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto subJsonValue = m_jsonValue[pKey];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(std::move(subJsonValue));
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

int32_t CJsonImpl::GetArray(const char *pKey, IJson *pJson) const noexcept
{
    if (likely(pKey != nullptr && pJson != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isArray()))
    {
        auto pJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pJsonImpl != nullptr))
        {
            try
            {
                pJsonImpl->m_jsonValue = std::move(m_jsonValue[pKey]);
                return 0;
            }
            catch (std::exception &e)
            {
                SetLastError(ErrorCode::kThrowException);
                return ErrorCode::kThrowException;
            }
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

template<typename T>
int32_t CJsonImpl::SetValue(const char *pKey, T &value) noexcept
{
    if (likely(pKey != nullptr))
    {
        try
        {
            m_jsonValue[pKey] = value;
            return 0;
        }
        catch (std::exception &e)
        {
            SetLastError(ErrorCode::kThrowException);
            return ErrorCode::kThrowException;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

int32_t CJsonImpl::SetBool(const char *pKey, bool bValue) noexcept
{
    return SetValue(pKey, bValue);
}

int32_t CJsonImpl::SetInt32(const char *pKey, int32_t iValue) noexcept
{
    return SetValue(pKey, iValue);
}

int32_t CJsonImpl::SetInt64(const char *pKey, int64_t iValue) noexcept
{
    return SetValue(pKey, iValue);
}

int32_t CJsonImpl::SetUint32(const char *pKey, uint32_t uValue) noexcept
{
    return SetValue(pKey, uValue);
}

int32_t CJsonImpl::SetUint64(const char *pKey, uint64_t uValue) noexcept
{
    return SetValue(pKey, uValue);
}

int32_t CJsonImpl::SetDouble(const char *pKey, double dValue) noexcept
{
    return SetValue(pKey, dValue);
}

int32_t CJsonImpl::SetString(const char *pKey, const char *pValue) noexcept
{
    if (likely(pValue != nullptr))
    {
        return SetValue(pKey, pValue);
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

int32_t CJsonImpl::SetObject(const char *pKey, IJson *pJson) noexcept
{
    auto pJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
    if (likely(pJsonImpl != nullptr))
    {
        return SetValue(pKey, pJsonImpl->m_jsonValue);
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

IJson *CJsonImpl::SetObject(const char *pKey) noexcept
{
    Json::Value jsonValue(Json::objectValue);
    if (SetValue(pKey, jsonValue) == ErrorCode::kSuccess)
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto &subJsonValue = m_jsonValue[pKey];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(subJsonValue, false);
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

int32_t CJsonImpl::SetArray(const char *pKey, IJson *pJson) noexcept
{
    auto pJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
    if (likely(pJsonImpl != nullptr))
    {
        return SetValue(pKey, pJsonImpl->m_jsonValue);
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

IJson *CJsonImpl::SetArray(const char *pKey) noexcept
{
    Json::Value jsonValue(Json::arrayValue);
    if (SetValue(pKey, jsonValue) == ErrorCode::kSuccess)
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto &subJsonValue = m_jsonValue[pKey];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(subJsonValue, false);
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

template<typename T>
T CJsonImpl::GetValue(uint32_t iIndex, T defaultValue) const noexcept
{
    if (likely(m_jsonValue.isArray() && iIndex < m_jsonValue.size()))
    {
        try
        {
            return m_jsonValue[iIndex].as<T>();
        }
        catch (std::exception &e)
        {
        }
    }

    return defaultValue;
}

bool CJsonImpl::GetBool(uint32_t iIndex, bool bDefault) const noexcept
{
    return GetValue(iIndex, bDefault);
}

int32_t CJsonImpl::GetInt32(uint32_t iIndex, int32_t iDefault) const noexcept
{
    return GetValue(iIndex, iDefault);
}

int64_t CJsonImpl::GetInt64(uint32_t iIndex, int64_t iDefault) const noexcept
{
    return GetValue(iIndex, iDefault);
}

uint32_t CJsonImpl::GetUint32(uint32_t iIndex, uint32_t uDefault) const noexcept
{
    return GetValue(iIndex, uDefault);
}

uint64_t CJsonImpl::GetUint64(uint32_t iIndex, uint64_t uDefault) const noexcept
{
    return GetValue(iIndex, uDefault);
}

double CJsonImpl::GetDouble(uint32_t iIndex, double dDefault) const noexcept
{
    return GetValue(iIndex, dDefault);
}

const char *CJsonImpl::GetString(uint32_t iIndex, const char *pDefault) const noexcept
{
    return GetValue(iIndex, pDefault);
}

const IJson *CJsonImpl::GetObject(uint32_t iIndex) const noexcept
{
    if (likely(m_jsonValue.isArray() && iIndex < m_jsonValue.size()))
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto &subJsonValue = m_jsonValue[iIndex];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(subJsonValue, false);
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

int32_t CJsonImpl::GetObject(uint32_t iIndex, IJson *pJson) const noexcept
{
    if (likely(m_jsonValue.isArray() && iIndex < m_jsonValue.size()))
    {
        auto pJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pJsonImpl != nullptr))
        {
            try
            {
                pJsonImpl->m_jsonValue = m_jsonValue[iIndex];
                return 0;
            }
            catch (std::exception &e)
            {
                SetLastError(ErrorCode::kThrowException);
                return ErrorCode::kThrowException;
            }
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

const IJson *CJsonImpl::GetArray(uint32_t iIndex) const noexcept
{
    if (likely(m_jsonValue.isArray() && iIndex < m_jsonValue.size()))
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto &subJsonValue = m_jsonValue[iIndex];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(subJsonValue, false);
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

int32_t CJsonImpl::GetArray(uint32_t iIndex, IJson *pJson) const noexcept
{
    if (likely(m_jsonValue.isArray() && iIndex < m_jsonValue.size()))
    {
        auto pJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pJsonImpl != nullptr))
        {
            try
            {
                pJsonImpl->m_jsonValue = m_jsonValue[iIndex];
                return 0;
            }
            catch (std::exception &e)
            {
                SetLastError(ErrorCode::kThrowException);
                return ErrorCode::kThrowException;
            }
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

template<typename T>
int32_t CJsonImpl::AppendValue(T &value) noexcept
{
    if (likely(m_jsonValue.isArray()))
    {
        try
        {
            m_jsonValue.append(value);
            return 0;
        }
        catch (std::exception &e)
        {
            SetLastError(ErrorCode::kThrowException);
            return ErrorCode::kThrowException;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

int32_t CJsonImpl::AppendBool(bool bValue) noexcept
{
    return AppendValue(bValue);
}

int32_t CJsonImpl::AppendInt32(int32_t iValue) noexcept
{
    return AppendValue(iValue);
}

int32_t CJsonImpl::AppendInt64(int64_t iValue) noexcept
{
    return AppendValue(iValue);
}

int32_t CJsonImpl::AppendUint32(uint32_t uValue) noexcept
{
    return AppendValue(uValue);
}

int32_t CJsonImpl::AppendUint64(uint64_t uValue) noexcept
{
    return AppendValue(uValue);
}

int32_t CJsonImpl::AppendDouble(double dValue) noexcept
{
    return AppendValue(dValue);
}

int32_t CJsonImpl::AppendString(const char *pValue) noexcept
{
    if (likely(pValue != nullptr))
    {
        return AppendValue(pValue);
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

int32_t CJsonImpl::AppendObject(IJson *pJson) noexcept
{
    auto pCJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
    if (likely(pCJsonImpl != nullptr && pCJsonImpl->m_jsonValue.isObject()))
    {
        return AppendValue(pCJsonImpl->m_jsonValue);
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

IJson *CJsonImpl::AppendObject() noexcept
{
    Json::Value jsonValue(Json::objectValue);
    if (AppendValue(jsonValue) == ErrorCode::kSuccess)
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto &subJsonValue = m_jsonValue[m_jsonValue.size() - 1];
            m_vecJsonValues.push_back(nullptr);
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(subJsonValue, false);
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

int32_t CJsonImpl::AppendArray(IJson *pJson) noexcept
{
    auto pCJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
    if (likely(pCJsonImpl != nullptr && pCJsonImpl->m_jsonValue.isArray()))
    {
        return AppendValue(pCJsonImpl->m_jsonValue);
    }

    SetLastError(ErrorCode::kInvalidParam);
    return ErrorCode::kInvalidParam;
}

IJson *CJsonImpl::AppendArray() noexcept
{
    Json::Value jsonValue(Json::arrayValue);
    if (AppendValue(jsonValue) == ErrorCode::kSuccess)
    {
        CJsonImpl *pJsonImpl = nullptr;
        try
        {
            auto &subJsonValue = m_jsonValue[m_jsonValue.size() - 1];
            pJsonImpl = IAllocatorEx::GetInstance()->New<CJsonImpl>(subJsonValue, false);
            if (likely(pJsonImpl != nullptr))
            {
                m_vecJsonValues.push_back(pJsonImpl);
                return pJsonImpl;
            }
        }
        catch (std::exception &e)
        {
            IAllocatorEx::GetInstance()->Delete(pJsonImpl);
            SetLastError(ErrorCode::kThrowException);
            return nullptr;
        }
    }

    SetLastError(ErrorCode::kInvalidParam);
    return nullptr;
}

void CJsonImpl::Delete(const char *pKey) noexcept
{
    if (likely(pKey != nullptr))
    {
        m_jsonValue.removeMember(pKey);
    }
}

void CJsonImpl::Clear() noexcept
{
    for (auto pJsonValue : m_vecJsonValues)
    {
        delete pJsonValue;
    }
    m_vecJsonValues.clear();
    m_jsonValue.clear();
}

const char *CJsonImpl::ToString(bool bPretty) const noexcept
{
    try
    {
        if (bPretty)
        {
            m_strJsonString = m_jsonValue.toStyledString();
            return m_strJsonString.c_str();
        }
        else
        {
            auto styledString = m_jsonValue.toStyledString();
            m_strJsonString.clear();
            for (const auto &ch : styledString)
            {
                if (likely(!isspace(ch)))
                {
                    m_strJsonString.push_back(ch);
                }
            }
            return m_strJsonString.c_str();
        }
    }
    catch (std::exception &e)
    {
        SetLastError(ErrorCode::kThrowException);
        return nullptr;
    }

    return nullptr;
}

IJson::JsonType CJsonImpl::GetType(const char *pKey) const noexcept
{
    Json::ValueType jsonType = Json::ValueType::nullValue;
    if (pKey == nullptr)
    {
        jsonType = m_jsonValue.type();
    }
    else if (likely(pKey != nullptr && m_jsonValue.isMember(pKey)))
    {
        jsonType = m_jsonValue[pKey].type();
    }
    
    return GetIJsonType(jsonType);
}

IJson::JsonType CJsonImpl::GetType(uint32_t iIndex) const noexcept
{
    Json::ValueType jsonType = Json::ValueType::nullValue;
    if (likely(m_jsonValue.isArray() && iIndex < m_jsonValue.size()))
    {
        jsonType = m_jsonValue[iIndex].type();
    }

    return GetIJsonType(jsonType);
}

uint32_t CJsonImpl::GetSize() const noexcept
{
    return m_jsonValue.size();
}

}
}
