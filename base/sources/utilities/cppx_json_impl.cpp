#include "cppx_json_impl.h"

#include <fstream>
#include <iostream>
#include <cctype>

#include <utilities/cppx_common.h>
#include <utilities/cppx_last_error.h>
#include <utilities/cppx_error_code.h>

namespace cppx
{
namespace base
{

static IJson::JsonType GetIJsonType(Json::ValueType jsonType)
{
    static IJson::JsonType s_jsonTypeMap[] = {
        IJson::JsonType::kJsonTypeNull,
        IJson::JsonType::kJsonTypeNumber,
        IJson::JsonType::kJsonTypeNumber,
        IJson::JsonType::kJsonTypeNull,
        IJson::JsonType::kJsonTypeString,
        IJson::JsonType::kJsonTypeBoolean,
        IJson::JsonType::kJsonTypeArray,
        IJson::JsonType::kJsonTypeObject,
    };
    return s_jsonTypeMap[jsonType];
}

static Json::ValueType GetJsonValueType(IJson::JsonType jsonType)
{
    static Json::ValueType s_jsonValueTypeMap[] = {
        Json::ValueType::objectValue,
        Json::ValueType::arrayValue,
        Json::ValueType::stringValue,
        Json::ValueType::intValue,
        Json::ValueType::booleanValue,
        Json::ValueType::nullValue,
    };
    return s_jsonValueTypeMap[(uint8_t)jsonType];
}


IJson *IJson::Create(JsonType jsonType) noexcept
{
    return NEW CJsonImpl(jsonType);
}

void IJson::Destroy(IJson *pJson) noexcept
{
    if (likely(pJson != nullptr))
    {
        delete pJson;
    }
}

IJson::JsonGuard IJson::CreateWithGuard(JsonType jsonType) noexcept
{
    return JsonGuard(IJson::Create(jsonType));
}

CJsonImpl::CJsonImpl(JsonType jsonType) noexcept : m_jsonValue(GetJsonValueType(jsonType))
{
}

CJsonImpl::CJsonImpl(const Json::Value &&jsonValue) : m_jsonValue(std::move(jsonValue))
{
}

CJsonImpl::~CJsonImpl() noexcept
{
}

int32_t CJsonImpl::Parse(const char *pJsonStr) noexcept
{
    if (unlikely(pJsonStr == nullptr))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Invalid JSON");
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
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Invalid JSON: %s", errs.c_str());
            return ErrorCode::kInvalidParam;
        }
        m_jsonValue = std::move(jsonValue);
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Parse JSON failed: %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::ParseFile(const char *pJsonFile) noexcept
{
    if (unlikely(pJsonFile == nullptr))
    {
        SET_LAST_ERROR(ErrorCode::kInvalidParam, "Invalid JSON");
        return ErrorCode::kInvalidParam;
    }

    try
    {
        std::ifstream ifs(pJsonFile);
        if (unlikely(!ifs.is_open()))
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Open JSON file %s failed: %s", pJsonFile, strerror(errno));
            return ErrorCode::kInvalidParam;
        }
        Json::CharReaderBuilder builder;
        std::string errs;
        Json::Value jsonValue;
        if (unlikely(!Json::parseFromStream(builder, ifs, &jsonValue, &errs)))
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Invalid JSON: %s", errs.c_str());
            return ErrorCode::kInvalidParam;
        }
        m_jsonValue = std::move(jsonValue);
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Parse JSON file %s failed: %s", pJsonFile, e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

IJson::JsonGuard CJsonImpl::GetObject(const char *pKey) const noexcept
{
    try
    {
        if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isObject()))
        {
            return NEW CJsonImpl(std::move(m_jsonValue[pKey]));
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Get Json Object %s failed: Not found", pKey);
            return nullptr;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Get Json Object %s failed: %s", pKey, e.what());
        return nullptr;
    }

    return nullptr;
}

IJson::JsonGuard CJsonImpl::GetArray(const char *pKey) const noexcept
{
    try
    {
        if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isArray()))
        {
            return NEW CJsonImpl(std::move(m_jsonValue[pKey]));
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Get Json Array %s failed: Not found", pKey);
            return nullptr;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Get Json Array %s failed: %s", pKey, e.what());
        return nullptr;
    }

    return nullptr;
}

const char *CJsonImpl::GetString(const char *pKey, const char *pDefault) const noexcept
{
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isString()))
    {
        return m_jsonValue[pKey].asCString();
    }

    return pDefault;
}

int32_t CJsonImpl::GetInt(const char *pKey, int32_t iDefault) const noexcept
{
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isInt()))
    {
        return m_jsonValue[pKey].asInt();
    }

    return iDefault;
}

bool CJsonImpl::GetBool(const char *pKey, bool bDefault) const noexcept
{
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey) && m_jsonValue[pKey].isBool()))
    {
        return m_jsonValue[pKey].asBool();
    }

    return bDefault;
}

int32_t CJsonImpl::SetObject(const char *pKey, IJson *pJson) noexcept
{
    try
    {
        auto pCJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pKey != nullptr && pCJsonImpl != nullptr && pCJsonImpl->m_jsonValue.isObject()))
        {
            m_jsonValue[pKey] = pCJsonImpl->m_jsonValue;
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Set Json Object %s failed: Invalid parameter", pKey);
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Set Json Object %s failed: %s", pKey, e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::SetArray(const char *pKey, IJson *pJson) noexcept
{
    try
    {
        auto pCJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pKey != nullptr && pCJsonImpl != nullptr && pCJsonImpl->m_jsonValue.isArray()))
        {
            m_jsonValue[pKey] = pCJsonImpl->m_jsonValue;
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Set Json Array %s failed: Invalid parameter", pKey);
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Set Json Array %s failed: %s", pKey, e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::SetString(const char *pKey, const char *pValue) noexcept
{
    try
    {
        if (likely(pKey != nullptr && pValue != nullptr))
        {
            m_jsonValue[pKey] = pValue;
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Set Json String %s failed: Invalid parameter", pKey);
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Set Json String %s failed: %s", pKey, e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::SetInt(const char *pKey, int32_t iValue) noexcept
{
    try
    {
        if (likely(pKey != nullptr))
        {
            m_jsonValue[pKey] = iValue;
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Set Json Int %s failed: Invalid parameter", pKey);
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Set Json Int %s failed: %s", pKey, e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::SetBool(const char *pKey, bool bValue) noexcept
{
    try
    {
        if (likely(pKey != nullptr))
        {
            m_jsonValue[pKey] = bValue;
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Set Json Bool %s failed: Invalid parameter", pKey);
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Set Json Bool %s failed: %s", pKey, e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

IJson::JsonGuard CJsonImpl::GetObject(int32_t iIndex) const noexcept
{
    if (likely(m_jsonValue.isArray() 
              && iIndex >= 0 && iIndex < (int32_t)m_jsonValue.size() 
              && m_jsonValue[iIndex].isObject()))
    {
        return NEW CJsonImpl(std::move(m_jsonValue[iIndex]));
    }

    return nullptr;
}

IJson::JsonGuard CJsonImpl::GetArray(int32_t iIndex) const noexcept
{
    if (likely(m_jsonValue.isArray() 
              && iIndex >= 0 && iIndex < (int32_t)m_jsonValue.size() 
              && m_jsonValue[iIndex].isArray()))
    {
        return NEW CJsonImpl(std::move(m_jsonValue[iIndex]));
    }

    return nullptr; 
}

const char *CJsonImpl::GetString(int32_t iIndex, const char *pDefault) const noexcept

{
    if (likely(m_jsonValue.isArray() 
              && iIndex >= 0 && iIndex < (int32_t)m_jsonValue.size() 
              && m_jsonValue[iIndex].isString()))
    {
        return m_jsonValue[iIndex].asCString();
    }

    return pDefault;
}

int32_t CJsonImpl::GetInt(int32_t iIndex, int32_t iDefault) const noexcept
{
    if (likely(m_jsonValue.isArray() 
              && iIndex >= 0 && iIndex < (int32_t)m_jsonValue.size() 
              && m_jsonValue[iIndex].isInt()))
    {
        return m_jsonValue[iIndex].asInt();
    }

    return iDefault;
}

bool CJsonImpl::GetBool(int32_t iIndex, bool bDefault) const noexcept
{
    if (likely(m_jsonValue.isArray() 
              && iIndex >= 0 && iIndex < (int32_t)m_jsonValue.size() 
              && m_jsonValue[iIndex].isBool()))
    {
        return m_jsonValue[iIndex].asBool();
    }

    return bDefault;
}

int32_t CJsonImpl::AppendObject(IJson *pJson) noexcept
{
    try
    {
        auto pCJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pCJsonImpl != nullptr && m_jsonValue.isArray() && pCJsonImpl->m_jsonValue.isObject()))
        {
            m_jsonValue.append(pCJsonImpl->m_jsonValue);
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Append Object failed: Invalid parameter");
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Append Object failed: %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::AppendArray(IJson *pJson) noexcept
{
    try
    {
        auto pCJsonImpl = dynamic_cast<CJsonImpl *>(pJson);
        if (likely(pCJsonImpl != nullptr && m_jsonValue.isArray() && pCJsonImpl->m_jsonValue.isArray()))
        {
            m_jsonValue.append(pCJsonImpl->m_jsonValue);
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Append Array failed: Invalid parameter");
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Append Array failed: %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::AppendString(const char *pValue) noexcept
{
    try
    {
        if (likely(pValue != nullptr && m_jsonValue.isArray() && pValue != nullptr))
        {
            m_jsonValue.append(pValue);
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Append String failed: Invalid parameter");
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Append String failed: %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::AppendInt(int32_t iValue) noexcept
{
    try
    {
        if (likely(m_jsonValue.isArray()))
        {
            m_jsonValue.append(iValue);
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Append Int failed: Invalid parameter");
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Append Int failed: %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
}

int32_t CJsonImpl::AppendBool(bool bValue) noexcept
{
    try
    {
        if (likely(m_jsonValue.isArray()))
        {
            m_jsonValue.append(bValue);
        }
        else
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Append Bool failed: Invalid parameter");
            return ErrorCode::kInvalidParam;
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "Append Bool failed: %s", e.what());
        return ErrorCode::kThrowException;
    }

    return 0;
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
    m_jsonValue.clear();
}

IJson::JsonStrGuard CJsonImpl::ToString(bool bPretty) const noexcept
{
    try
    {
        auto styledString = m_jsonValue.toStyledString();
        auto pStringBuffer = NEW char[styledString.length() + 1];
        if (likely(pStringBuffer != nullptr))
        {
            if (bPretty)
            {
                memcpy(pStringBuffer, styledString.c_str(), styledString.length() + 1);
            }
            else
            {
                uint32_t i = 0;
                for (const auto &ch : styledString)
                {
                    if (likely(!isspace(ch)))
                    {
                        pStringBuffer[i++] = ch;
                    }
                }
                pStringBuffer[i] = '\0';
            }
            return JsonStrGuard(pStringBuffer);
        }
    }
    catch (std::exception &e)
    {
        SET_LAST_ERROR(ErrorCode::kThrowException, "ToString failed: %s", e.what());
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

IJson::JsonType CJsonImpl::GetType(int32_t iIndex) const noexcept
{
    if (likely(iIndex >= 0 && iIndex < (int32_t)m_jsonValue.size()) && m_jsonValue.isArray())
    {
        return GetIJsonType(m_jsonValue[iIndex].type());
    }

    return IJson::JsonType::kJsonTypeNull;
}

uint32_t CJsonImpl::GetSize() const noexcept
{
    return m_jsonValue.size();
}

}
}
