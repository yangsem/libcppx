#include "cppx_json_impl.h"

#include <fstream>
#include <iostream>

#include <cppx_common.h>
#include <cppx_last_error.h>
#include <cppx_error_code.h>

namespace cppx
{
namespace base
{

IJson *IJson::Create() noexcept
{
    return NEW CJsonImpl();
}

void IJson::Destroy(IJson *pJson) noexcept
{
    if (likely(pJson != nullptr))
    {
        delete pJson;
    }
}

IJson::JsonGuard IJson::CreateWithGuard() noexcept
{
    return JsonGuard(IJson::Create());
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
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errs;
        if (unlikely(!reader->parse(pJsonStr, pJsonStr + strlen(pJsonStr), &m_jsonValue, &errs)))
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Invalid JSON: %s", errs.c_str());
            return ErrorCode::kInvalidParam;
        }
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
        if (unlikely(!Json::parseFromStream(builder, ifs, &m_jsonValue, &errs)))
        {
            SET_LAST_ERROR(ErrorCode::kInvalidParam, "Invalid JSON: %s", errs.c_str());
            return ErrorCode::kInvalidParam;
        }
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
        if (likely(pKey != nullptr && pCJsonImpl != nullptr))
        {
            m_jsonValue[pKey] = std::move(pCJsonImpl->m_jsonValue);
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
        if (likely(pKey != nullptr && pCJsonImpl != nullptr))
        {
            m_jsonValue[pKey] = std::move(pCJsonImpl->m_jsonValue);
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
                for (auto ch : styledString)
                {
                    if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r')
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
    if (likely(pKey != nullptr && m_jsonValue.isMember(pKey)))
    {
        static IJson::JsonType s_jsonTypeMap[] = {
            IJson::JsonType::kJsonTypeNull,
            IJson::JsonType::kJsonTypeNumber,
            IJson::JsonType::kJsonTypeNull,
            IJson::JsonType::kJsonTypeString,
            IJson::JsonType::kJsonTypeBoolean,
            IJson::JsonType::kJsonTypeArray,
            IJson::JsonType::kJsonTypeObject,
        };
        
        auto jsonType = pKey != nullptr ? m_jsonValue[pKey].type() : m_jsonValue.type();
        if (likely(jsonType < sizeof(s_jsonTypeMap) / sizeof(s_jsonTypeMap[0])))
        {
            return s_jsonTypeMap[jsonType];
        }
    }

    return JsonType::kJsonTypeNull;
}

}
}
