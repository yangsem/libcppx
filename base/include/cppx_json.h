#ifndef __CPPX_JSON_H__
#define __CPPX_JSON_H__

#include <stdint.h>
#include <cppx_export.h>

namespace cppx
{
namespace base
{

class EXPORT IJson
{
public:
    enum class JsonType : uint8_t
    {
        kJsonTypeObject = 0,
        kJsonTypeArray,
        kJsonTypeString,
        kJsonTypeNumber,
        kJsonTypeBoolean,
    };

    class JsonGuard
    {
    public:
        JsonGuard(IJson *pJson) : m_pJson(pJson) {}

        JsonGuard(const JsonGuard &) = delete;
        JsonGuard &operator=(const JsonGuard &) = delete;

        JsonGuard(JsonGuard &&jsonGuard) noexcept
        {
            m_pJson = jsonGuard.m_pJson;
            jsonGuard.m_pJson = nullptr;
        }

        JsonGuard &operator=(JsonGuard &&jsonGuard) noexcept
        {
            m_pJson = jsonGuard.m_pJson;
            jsonGuard.m_pJson = nullptr;
            return *this;
        }

        ~JsonGuard() noexcept
        {
            if (m_pJson != nullptr)
            {
                IJson::Destroy(m_pJson);
            }
        }

        IJson *operator->() noexcept
        {
            return m_pJson;
        }

        IJson &operator*() noexcept
        {
            return *m_pJson;
        }

    private:
        IJson *m_pJson {nullptr};
    };

protected:
    virtual ~IJson() noexcept = default;

public:
    /**
     * @brief 创建一个IJson对象
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程安全
     */
    static IJson *Create() noexcept;

    /**
     * @brief 销毁一个IJson对象
     * @param pJson IJson对象指针
     * @note 多线程安全
     */
    static void Destroy(IJson *pJson) noexcept;

    /**
     * @brief 创建一个IJson对象，并返回一个JsonGuard对象
     * @return 成功返回JsonGuard对象，失败返回nullptr
     * @note 多线程安全
     */
    static JsonGuard CreateWithGuard() noexcept;

    /**
     * @brief 解析一个JSON字符串
     * @param pJsonStr JSON字符串
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t Parse(const char *pJsonStr) noexcept = 0;
    /**
     * @brief 解析一个JSON文件
     * @param pJsonFile JSON文件路径
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t ParseFile(const char *pJsonFile) noexcept = 0;

    /**
     * @brief 获取一个对象
     * @param pKey 对象键
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全
     */
    virtual IJson *GetObject(const char *pKey) noexcept = 0;

    /**
     * @brief 获取一个数组
     * @param pKey 数组键
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全
     */
    virtual IJson *GetArray(const char *pKey) noexcept = 0;

    /**
     * @brief 获取一个字符串
     * @param pKey 字符串键
     * @param pDefault 默认值
     * @return 成功返回字符串指针，失败返回nullptr
     * @note 多线程不安全
     */
    virtual const char *GetString(const char *pKey, const char *pDefault = nullptr) noexcept = 0;

    /**
     * @brief 获取一个整数
     * @param pKey 整数键
     * @param iDefault 默认值
     * @return 成功返回整数，失败返回默认值
     * @note 多线程不安全
     */
    virtual int32_t GetInt(const char *pKey, int32_t iDefault = 0) noexcept = 0;

    /**
     * @brief 获取一个布尔值
     * @param pKey 布尔值键
     * @param bDefault 默认值
     * @return 成功返回布尔值，失败返回默认值
     * @note 多线程不安全
     */
    virtual bool GetBool(const char *pKey, bool bDefault = false) noexcept = 0;

    /**
     * @brief 设置一个对象
     * @param pKey 对象键
     * @param pJson IJson对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetObject(const char *pKey, IJson *pJson) noexcept = 0;

    /**
     * @brief 设置一个数组
     * @param pKey 数组键
     * @param pJson IJson对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetArray(const char *pKey, IJson *pJson) noexcept = 0;

    /**
     * @brief 设置一个字符串
     * @param pKey 字符串键
     * @param pValue 字符串值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetString(const char *pKey, const char *pValue) noexcept = 0;

    /**
     * @brief 设置一个整数
     * @param pKey 整数键
     * @param iValue 整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetInt(const char *pKey, int32_t iValue) noexcept = 0;

    /**
     * @brief 设置一个布尔值
     * @param pKey 布尔值键
     * @param bValue 布尔值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetBool(const char *pKey, bool bValue) noexcept = 0;

    /**
     * @brief 将IJson对象转换为JSON字符串
     * @param bPretty 是否美化格式
     * @return 成功返回JSON字符串指针，失败返回nullptr
     * @note 多线程不安全
     * @note 返回的指针不需要手动释放，再次调用同一个对象的ToString接口后上次的指针会失效
     */
    virtual const char *ToString(bool bPretty = false) noexcept = 0;

    /**
     * @brief 获取一个对象的类型
     * @param pKey 对象键，为空时返回当前对象的类型
     * @return 成功返回对象类型，失败返回kJsonTypeObject
     * @note 多线程不安全
     */
    virtual JsonType GetType(const char *pKey = nullptr) noexcept = 0;
};

}
}

#endif // __CPPX_JSON_H__