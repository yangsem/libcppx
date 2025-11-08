#ifndef __CPPX_JSON_H__
#define __CPPX_JSON_H__

#include <cstdint>
#include <utilities/export.h>

namespace cppx
{
namespace base
{

class EXPORT IJson
{
public:
    enum class JsonType : uint8_t
    {
        kInvalid = 0,
        kBool,
        kInt32,
        kInt64,
        kUint32,
        kUint64,
        kDouble,
        kString,
        kObject,
        kArray,
    };

protected:
    virtual ~IJson() noexcept = default;

public:
    /**
     * @brief 创建一个IJson对象
     * @param jsonType 对象类型
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程安全
     */
    static IJson *Create(JsonType jsonType = JsonType::kObject) noexcept;

    /**
     * @brief 销毁一个IJson对象
     * @param pJson IJson对象指针
     * @note 多线程安全
     */
    static void Destroy(IJson *pJson) noexcept;

    /**
     * @brief 解析一个JSON字符串
     * @param pJsonStr JSON字符串
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全，解析失败后，对象状态不变
     */
    virtual int32_t Parse(const char *pJsonStr) noexcept = 0;
    /**
     * @brief 解析一个JSON文件
     * @param pJsonFile JSON文件路径
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全，解析失败后，对象状态不变
     */
    virtual int32_t ParseFile(const char *pJsonFile) noexcept = 0;

    /**
     * @brief 获取一个布尔值
     * @param pKey 布尔值键
     * @param bDefault 默认值
     * @return 成功返回布尔值，失败返回默认值
     * @note 多线程不安全
     */
    virtual bool GetBool(const char *pKey, bool bDefault = false) const noexcept = 0;

    /**
     * @brief 获取一个整数
     * @param pKey 整数键
     * @param iDefault 默认值
     * @return 成功返回整数，失败返回默认值
     * @note 多线程不安全
     */
    virtual int32_t GetInt32(const char *pKey, int32_t iDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个64位整数
     * @param pKey 64位整数键
     * @param iDefault 默认值
     * @return 成功返回64位整数，失败返回默认值
     * @note 多线程不安全
     */
    virtual int64_t GetInt64(const char *pKey, int64_t iDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个32位整数
     * @param pKey 32位整数键
     * @param iDefault 默认值
     * @return 成功返回32位整数，失败返回默认值
     * @note 多线程不安全
     */
    virtual uint32_t GetUint32(const char *pKey, uint32_t uDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个64位整数
     * @param pKey 64位整数键
     * @param iDefault 默认值
     * @return 成功返回64位整数，失败返回默认值
     * @note 多线程不安全
     */
    virtual uint64_t GetUint64(const char *pKey, uint64_t uDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个双精度浮点数
     * @param pKey 双精度浮点数键
     * @param dDefault 默认值
     * @return 成功返回双精度浮点数，失败返回默认值
     * @note 多线程不安全
     */
    virtual double GetDouble(const char *pKey, double dDefault = 0.0) const noexcept = 0;

    /**
     * @brief 获取一个字符串
     * @param pKey 字符串键
     * @param pDefault 默认值
     * @return 成功返回字符串指针，失败返回nullptr
     * @note 多线程不安全
     */
    virtual const char *GetString(const char *pKey, const char *pDefault = nullptr) const noexcept = 0;

    /**
     * @brief 获取一个对象
     * @param pKey 对象键
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全
     */
    virtual const IJson *GetObject(const char *pKey) const noexcept = 0;

    /**
     * @brief 获取并拷贝一个子对象
     * @param pKey 对象键
     * @param pJson 子对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t GetObject(const char *pKey, IJson *pJson) const noexcept = 0;

    /**
     * @brief 获取一个数组
     * @param pKey 数组键
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全
     */
    virtual const IJson *GetArray(const char *pKey) const noexcept = 0;

    /**
     * @brief 获取并拷贝一个子数组
     * @param pKey 数组键
     * @param pJson 子数组指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t GetArray(const char *pKey, IJson *pJson) const noexcept = 0;

    /**
     * @brief 设置一个布尔值
     * @param pKey 布尔值键
     * @param bValue 布尔值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetBool(const char *pKey, bool bValue) noexcept = 0;

    /**
     * @brief 设置一个整数
     * @param pKey 整数键
     * @param iValue 整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetInt32(const char *pKey, int32_t iValue) noexcept = 0;

    /**
     * @brief 设置一个64位整数
     * @param pKey 64位整数键
     * @param iValue 64位整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetInt64(const char *pKey, int64_t iValue) noexcept = 0;

    /**
     * @brief 设置一个32位整数
     * @param pKey 32位整数键
     * @param iValue 32位整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetUint32(const char *pKey, uint32_t uValue) noexcept = 0;

    /**
     * @brief 设置一个64位整数
     * @param pKey 64位整数键
     * @param iValue 64位整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetUint64(const char *pKey, uint64_t uValue) noexcept = 0;

    /**
     * @brief 设置一个双精度浮点数
     * @param pKey 双精度浮点数键
     * @param dValue 双精度浮点数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetDouble(const char *pKey, double dValue) noexcept = 0;

    /**
     * @brief 设置一个字符串
     * @param pKey 字符串键
     * @param pValue 字符串值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetString(const char *pKey, const char *pValue) noexcept = 0;

    /**
     * @brief 设置一个对象
     * @param pKey 对象键
     * @param pJson IJson对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetObject(const char *pKey, IJson *pJson) noexcept = 0;

    /**
     * @brief 零拷贝添加一个子对象
     * @param pKey 对象键
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全，子对象生命周期跟随当前对象
     */
    virtual IJson *SetObject(const char *pKey) noexcept = 0;

    /**
     * @brief 设置一个数组
     * @param pKey 数组键
     * @param pJson IJson对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t SetArray(const char *pKey, IJson *pJson) noexcept = 0;

    /**
     * @brief 零拷贝添加一个子数组
     * @param pKey 数组键
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全，子对象生命周期跟随当前对象
     */
    virtual IJson *SetArray(const char *pKey) noexcept = 0;

    /*===========================以下为数组操作===========================*/

    /**
     * @brief 获取一个布尔值
     * @param iIndex 索引
     * @param bDefault 默认值
     * @return 成功返回布尔值，失败返回默认值
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回bDefault
     */
    virtual bool GetBool(uint32_t iIndex, bool bDefault = false) const noexcept = 0;

    /**
     * @brief 获取一个整数
     * @param iIndex 索引
     * @param iDefault 默认值
     * @return 成功返回整数，失败返回默认值
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回iDefault
     */
    virtual int32_t GetInt32(uint32_t iIndex, int32_t iDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个64位整数
     * @param iIndex 索引
     * @param iDefault 默认值
     * @return 成功返回64位整数，失败返回默认值
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回iDefault
     */
    virtual int64_t GetInt64(uint32_t iIndex, int64_t iDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个32位整数
     * @param iIndex 索引
     * @param iDefault 默认值
     * @return 成功返回32位整数，失败返回默认值
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回iDefault
     */
    virtual uint32_t GetUint32(uint32_t iIndex, uint32_t uDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个64位整数
     * @param iIndex 索引
     * @param iDefault 默认值
     * @return 成功返回64位整数，失败返回默认值
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回iDefault
     */
    virtual uint64_t GetUint64(uint32_t iIndex, uint64_t uDefault = 0) const noexcept = 0;

    /**
     * @brief 获取一个双精度浮点数
     * @param iIndex 索引
     * @param dDefault 默认值
     * @return 成功返回双精度浮点数，失败返回默认值
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回dDefault
     */
    virtual double GetDouble(uint32_t iIndex, double dDefault = 0.0) const noexcept = 0;

    /**
     * @brief 获取一个字符串
     * @param iIndex 索引
     * @param pDefault 默认值
     * @return 成功返回字符串指针，失败返回nullptr
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回pDefault
     */
    virtual const char *GetString(uint32_t iIndex, const char *pDefault = nullptr) const noexcept = 0;

    /**
     * @brief 获取一个对象
     * @param iIndex 索引
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回nullptr
     */
    virtual const IJson *GetObject(uint32_t iIndex) const noexcept = 0;

    /**
     * @brief 获取并拷贝一个子对象
     * @param iIndex 索引
     * @param pJson 子对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t GetObject(uint32_t iIndex, IJson *pJson) const noexcept = 0;

    /**
     * @brief 获取一个数组
     * @param iIndex 索引
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全，仅在对象为数组时有效，数组越界返回nullptr
     */
    virtual const IJson *GetArray(uint32_t iIndex) const noexcept = 0;

    /**
     * @brief 获取并拷贝一个子数组
     * @param iIndex 索引
     * @param pJson 子数组指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t GetArray(uint32_t iIndex, IJson *pJson) const noexcept = 0;

    /**
     * @brief 往数组末尾添加一个布尔值
     * @param bValue 布尔值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendBool(bool bValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个整数
     * @param iValue 整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendInt32(int32_t iValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个64位整数
     * @param iValue 64位整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendInt64(int64_t iValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个32位整数
     * @param uValue 32位整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendUint32(uint32_t uValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个64位整数
     * @param uValue 64位整数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendUint64(uint64_t uValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个双精度浮点数
     * @param dValue 双精度浮点数值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendDouble(double dValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个字符串
     * @param pValue 字符串值
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendString(const char *pValue) noexcept = 0;

    /**
     * @brief 往数组末尾添加一个对象
     * @param pJson IJson对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendObject(IJson *pJson) noexcept = 0;

    /**
     * @brief 零拷贝添加一个子对象
     * @param pJson IJson对象指针
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全，子对象生命周期跟随当前对象
     */
    virtual IJson *AppendObject() noexcept = 0;

    /**
     * @brief 往数组末尾添加一个数组
     * @param pJson IJson对象指针
     * @return 成功返回0，失败返回错误码
     * @note 多线程不安全
     */
    virtual int32_t AppendArray(IJson *pJson) noexcept = 0;

    /**
     * @brief 零拷贝添加一个子数组
     * @return 成功返回IJson对象指针，失败返回nullptr
     * @note 多线程不安全，子对象生命周期跟随当前对象
     */
    virtual IJson *AppendArray() noexcept = 0;

    /**
     * @brief 删除一个对象
     * @param pKey 对象键
     * @note 多线程不安全
     */
    virtual void Delete(const char *pKey) noexcept = 0;

    /**
     * @brief 清空一个对象
     * @note 多线程不安全
     */
    virtual void Clear() noexcept = 0;

    /**
     * @brief 将IJson对象转换为JSON字符串
     * @param bPretty 是否美化格式
     * @return 成功返回JSON字符串，失败返回nullptr
     * @note 多线程不安全，返回的指针生命周期跟随当前对象
     */
    virtual const char *ToString(bool bPretty = false) const noexcept = 0;

    /**
     * @brief 获取一个对象的元素个数
     * @return 成功返回元素个数
     * @note 多线程不安全，仅在对象为数组或对象时有效，否则返回0
     */
    virtual uint32_t GetSize() const noexcept = 0;

    /**
     * @brief 获取一个对象的类型
     * @param pKey 对象键，为空时返回当前对象的类型
     * @return 返回对象类型
     * @note 多线程不安全
     */
    virtual JsonType GetType(const char *pKey = nullptr) const noexcept = 0;

    /**
     * @brief 获取一个对象的类型
     * @param iIndex 索引
     * @return 返回数组元素对象类型
     * @note 多线程不安全
     */
    virtual JsonType GetType(uint32_t iIndex) const noexcept = 0;
};

}
}

#endif // __CPPX_JSON_H__