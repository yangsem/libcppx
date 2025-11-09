#include <gtest/gtest.h>
#include <utilities/json.h>
#include <fstream>
#include <cstring>
#include <climits>
#include <cstdint>
#include <thread>
#include <vector>

using namespace cppx::base;

// RAII包装类，用于自动管理IJson对象生命周期
class JsonGuard
{
public:
    explicit JsonGuard(IJson::JsonType jsonType = IJson::JsonType::kObject)
        : m_pJson(IJson::Create(jsonType))
    {
    }
    
    ~JsonGuard()
    {
        if (m_pJson)
        {
            IJson::Destroy(m_pJson);
        }
    }
    
    // 禁止拷贝
    JsonGuard(const JsonGuard&) = delete;
    JsonGuard& operator=(const JsonGuard&) = delete;
    
    // 允许移动
    JsonGuard(JsonGuard&& other) noexcept
        : m_pJson(other.m_pJson)
    {
        other.m_pJson = nullptr;
    }
    
    JsonGuard& operator=(JsonGuard&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pJson)
            {
                IJson::Destroy(m_pJson);
            }
            m_pJson = other.m_pJson;
            other.m_pJson = nullptr;
        }
        return *this;
    }
    
    IJson* get() const { return m_pJson; }
    IJson* operator->() const { return m_pJson; }
    IJson& operator*() const { return *m_pJson; }
    
    explicit operator bool() const { return m_pJson != nullptr; }

private:
    IJson* m_pJson;
};

class CppxJsonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 创建测试用的JSON文件
        std::ofstream testFile("test_data.json");
        testFile << R"({
    "name": "测试用户",
    "age": 25,
    "isActive": true,
    "address": {
        "city": "北京",
        "zipCode": "100000"
    },
    "hobbies": ["读书", "游泳", "编程"],
    "scores": [95, 87, 92],
    "metadata": null
})";
        testFile.close();
    }

    void TearDown() override
    {
        // 清理测试文件
        std::remove("test_data.json");
    }
};

// 测试基本的JSON对象创建和销毁
TEST_F(CppxJsonTest, TestBasicCreation)
{
    // 测试Create方法
    IJson* pJson = IJson::Create();
    ASSERT_NE(pJson, nullptr);
    
    // 测试Create方法和Guard包装类
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试销毁
    IJson::Destroy(pJson);
}

// 测试JSON字符串解析
TEST_F(CppxJsonTest, TestParseString)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试有效JSON字符串
    const char* validJson = R"({"name": "测试", "age": 25, "active": true})";
    int32_t result = jsonGuard->Parse(validJson);
    EXPECT_EQ(result, 0);
    
    // 测试无效JSON字符串
    const char* invalidJson = R"({"name": "测试", "age": 25, "active": true)";  // 缺少右括号
    result = jsonGuard->Parse(invalidJson);
    EXPECT_NE(result, 0);
    
    // 测试空字符串
    result = jsonGuard->Parse("");
    EXPECT_NE(result, 0);
    
    // 测试null指针
    result = jsonGuard->Parse(nullptr);
    EXPECT_NE(result, 0);
}

// 测试JSON文件解析
TEST_F(CppxJsonTest, TestParseFile)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试解析存在的文件
    int32_t result = jsonGuard->ParseFile("test_data.json");
    EXPECT_EQ(result, 0);
    
    // 测试解析不存在的文件
    result = jsonGuard->ParseFile("nonexistent.json");
    EXPECT_NE(result, 0);
    
    // 测试null指针
    result = jsonGuard->ParseFile(nullptr);
    EXPECT_NE(result, 0);
}

// 测试Get操作
TEST_F(CppxJsonTest, TestGetOperations)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试GetString
    const char* name = jsonGuard->GetString("name");
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "测试用户");
    
    // 测试GetString with default
    const char* defaultStr = jsonGuard->GetString("nonexistent", "默认值");
    EXPECT_STREQ(defaultStr, "默认值");
    
    // 测试GetInt32
    int32_t age = jsonGuard->GetInt32("age");
    EXPECT_EQ(age, 25);
    
    // 测试GetInt32 with default
    int32_t defaultInt = jsonGuard->GetInt32("nonexistent", 999);
    EXPECT_EQ(defaultInt, 999);
    
    // 测试GetBool
    bool isActive = jsonGuard->GetBool("isActive");
    EXPECT_TRUE(isActive);
    
    // 测试GetBool with default
    bool defaultBool = jsonGuard->GetBool("nonexistent", false);
    EXPECT_FALSE(defaultBool);
    
    // 测试GetObject
    const IJson* addressGuard = jsonGuard->GetObject("address");
    ASSERT_NE(addressGuard, nullptr);
    const char* city = addressGuard->GetString("city");
    ASSERT_NE(city, nullptr);
    EXPECT_STREQ(city, "北京");
    
    // 测试GetArray
    const IJson* hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard, nullptr);
    
    // 测试获取不存在的对象
    const IJson* nonexistentGuard = jsonGuard->GetObject("nonexistent");
    EXPECT_EQ(nonexistentGuard, nullptr);
    
    // 测试获取不存在的数组
    const IJson* nonexistentArray = jsonGuard->GetArray("nonexistent");
    EXPECT_EQ(nonexistentArray, nullptr);
}

// 测试Set操作
TEST_F(CppxJsonTest, TestSetOperations)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试SetString
    int32_t result = jsonGuard->SetString("name", "新用户");
    EXPECT_EQ(result, 0);
    const char* name = jsonGuard->GetString("name");
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "新用户");
    
    // 测试SetInt32
    result = jsonGuard->SetInt32("age", 30);
    EXPECT_EQ(result, 0);
    int32_t age = jsonGuard->GetInt32("age");
    EXPECT_EQ(age, 30);
    
    // 测试SetBool
    result = jsonGuard->SetBool("isActive", false);
    EXPECT_EQ(result, 0);
    bool isActive = jsonGuard->GetBool("isActive");
    EXPECT_FALSE(isActive);
    
    // 测试SetObject
    JsonGuard subJsonGuard;
    ASSERT_NE(subJsonGuard.get(), nullptr);
    subJsonGuard->SetString("country", "中国");
    subJsonGuard->SetString("province", "北京");
    
    result = jsonGuard->SetObject("location", subJsonGuard.get());
    EXPECT_EQ(result, 0);
    
    const IJson* locationGuard = jsonGuard->GetObject("location");
    ASSERT_NE(locationGuard, nullptr);
    const char* country = locationGuard->GetString("country");
    ASSERT_NE(country, nullptr);
    EXPECT_STREQ(country, "中国");
    
    // 测试SetArray
    JsonGuard arrayJsonGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayJsonGuard.get(), nullptr);
    arrayJsonGuard->AppendBool(true);
    arrayJsonGuard->AppendInt32(1);
    arrayJsonGuard->AppendString("value");
    
    result = jsonGuard->SetArray("newArray", arrayJsonGuard.get());
    EXPECT_EQ(result, 0);
    
    // 测试无效参数
    result = jsonGuard->SetString(nullptr, "value");
    EXPECT_NE(result, 0);
    
    result = jsonGuard->SetString("key", nullptr);
    EXPECT_NE(result, 0);
    
    result = jsonGuard->SetObject(nullptr, nullptr);
    EXPECT_NE(result, 0);
}

// 测试Delete功能
TEST_F(CppxJsonTest, TestDelete)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试Delete
    jsonGuard->Delete("name");
    // 测试获取key为name的值
    const char* name = jsonGuard->GetString("name");
    EXPECT_EQ(name, nullptr);
}

// 测试Clear功能
TEST_F(CppxJsonTest, TestClear)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);

    // 设置一些数据
    jsonGuard->SetString("name", "测试");
    jsonGuard->SetInt32("age", 25);
    jsonGuard->SetBool("active", true);

    // 测试清空前，类型为object
    EXPECT_EQ(jsonGuard->GetType(), IJson::JsonType::kObject);
    
    // 测试Clear
    jsonGuard->Clear();
    // 测试获取key为name的值
    const char* name = jsonGuard->GetString("name");
    EXPECT_EQ(name, nullptr);
    // 测试获取key为age的值
    int32_t age = jsonGuard->GetInt32("age");
    EXPECT_EQ(age, 0);
    // 测试获取key为active的值
    bool active = jsonGuard->GetBool("active");
    EXPECT_EQ(active, false);

    EXPECT_EQ(jsonGuard->GetType(), IJson::JsonType::kObject);
}

// 测试ToString功能
TEST_F(CppxJsonTest, TestToString)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 设置一些数据
    jsonGuard->SetString("name", "测试");
    jsonGuard->SetInt32("age", 25);
    jsonGuard->SetBool("active", true);
    
    // 测试普通格式
    const char* str = jsonGuard->ToString(false);
    ASSERT_NE(str, nullptr);
    EXPECT_NE(strlen(str), 0);
    
    // 测试美化格式
    const char* prettyStr = jsonGuard->ToString(true);
    ASSERT_NE(prettyStr, nullptr);
    EXPECT_NE(strlen(prettyStr), 0);
    
    // 美化格式应该比普通格式长（包含换行和缩进）
    EXPECT_GT(strlen(prettyStr), strlen(str));
}

// 测试GetType功能
TEST_F(CppxJsonTest, TestGetType)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试各种类型的检测
    IJson::JsonType type = jsonGuard->GetType("name");
    EXPECT_EQ(type, IJson::JsonType::kString);
    
    type = jsonGuard->GetType("age");
    EXPECT_EQ(type, IJson::JsonType::kInt64);
    
    type = jsonGuard->GetType("isActive");
    EXPECT_EQ(type, IJson::JsonType::kBool);
    
    type = jsonGuard->GetType("address");
    EXPECT_EQ(type, IJson::JsonType::kObject);
    
    type = jsonGuard->GetType("hobbies");
    EXPECT_EQ(type, IJson::JsonType::kArray);
    
    type = jsonGuard->GetType("metadata");
    EXPECT_EQ(type, IJson::JsonType::kInvalid); // null值在头文件中没有对应类型，使用kInvalid
    
    // 测试获取不存在的键的类型
    type = jsonGuard->GetType("nonexistent");
    EXPECT_EQ(type, IJson::JsonType::kInvalid);
}

// 测试错误处理和边界情况
TEST_F(CppxJsonTest, TestErrorHandling)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试空JSON对象的各种操作
    const char* str = jsonGuard->GetString("nonexistent");
    EXPECT_EQ(str, nullptr);
    
    int32_t intVal = jsonGuard->GetInt32("nonexistent");
    EXPECT_EQ(intVal, 0);
    
    bool boolVal = jsonGuard->GetBool("nonexistent");
    EXPECT_FALSE(boolVal);
    
    const IJson* objGuard = jsonGuard->GetObject("nonexistent");
    EXPECT_EQ(objGuard, nullptr);
    
    const IJson* arrGuard = jsonGuard->GetArray("nonexistent");
    EXPECT_EQ(arrGuard, nullptr);
    
    // 测试类型不匹配的情况
    jsonGuard->SetString("testKey", "string value");
    
    // 尝试以错误类型获取
    int32_t wrongType = jsonGuard->GetInt32("testKey");
    EXPECT_EQ(wrongType, 0); // 应该返回默认值
    
    bool wrongBool = jsonGuard->GetBool("testKey");
    EXPECT_FALSE(wrongBool); // 应该返回默认值
}

// 测试复杂嵌套结构
TEST_F(CppxJsonTest, TestComplexNestedStructure)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建嵌套结构
    JsonGuard userGuard;
    userGuard->SetString("name", "张三");
    userGuard->SetInt32("age", 28);
    
    JsonGuard profileGuard;
    profileGuard->SetString("email", "zhangsan@example.com");
    profileGuard->SetString("phone", "13800138000");
    
    userGuard->SetObject("profile", profileGuard.get());
    
    JsonGuard skillsGuard;
    // 这里需要根据实际的数组操作方法来设置数组元素
    
    userGuard->SetArray("skills", skillsGuard.get());
    
    // 设置到主对象
    jsonGuard->SetObject("user", userGuard.get());
    
    // 验证嵌套结构
    const IJson* retrievedUser = jsonGuard->GetObject("user");
    ASSERT_NE(retrievedUser, nullptr);
    
    const char* userName = retrievedUser->GetString("name");
    ASSERT_NE(userName, nullptr);
    EXPECT_STREQ(userName, "张三");
    
    const IJson* retrievedProfile = retrievedUser->GetObject("profile");
    ASSERT_NE(retrievedProfile, nullptr);
    
    const char* email = retrievedProfile->GetString("email");
    ASSERT_NE(email, nullptr);
    EXPECT_STREQ(email, "zhangsan@example.com");
}

// 测试Guard类的移动语义
TEST_F(CppxJsonTest, TestGuardMoveSemantics)
{
    // 测试JsonGuard的移动构造
    JsonGuard guard1;
    ASSERT_NE(guard1.get(), nullptr);
    
    auto guard2 = std::move(guard1);
    EXPECT_EQ(guard1.get(), nullptr); // 移动后应该为空
    ASSERT_NE(guard2.get(), nullptr); // 新对象应该有效
    
    // 测试ToString返回的字符串指针
    guard2->SetString("test", "value");
    const char* str1 = guard2->ToString();
    ASSERT_NE(str1, nullptr);
    EXPECT_GT(strlen(str1), 0);
}

// 性能测试 - 大量数据操作
TEST_F(CppxJsonTest, TestPerformanceWithLargeData)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 设置大量数据
    const int numItems = 1000;
    for (int i = 0; i < numItems; ++i)
    {
        std::string key = "item_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        jsonGuard->SetString(key.c_str(), value.c_str());
    }
    
    // 验证数据
    for (int i = 0; i < numItems; ++i)
    {
        std::string key = "item_" + std::to_string(i);
        std::string expectedValue = "value_" + std::to_string(i);
        const char* actualValue = jsonGuard->GetString(key.c_str());
        ASSERT_NE(actualValue, nullptr);
        EXPECT_STREQ(actualValue, expectedValue.c_str());
    }
    
    // 测试ToString性能
    const char* str = jsonGuard->ToString();
    ASSERT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0);
}

// 测试数组操作（如果支持的话）
TEST_F(CppxJsonTest, TestArrayOperations)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析包含数组的测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 获取数组并验证
    const IJson* hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard, nullptr);
    
    // 验证数组类型
    IJson::JsonType arrayType = hobbiesGuard->GetType();
    EXPECT_EQ(arrayType, IJson::JsonType::kArray);
    
    // 获取分数数组
    const IJson* scoresGuard = jsonGuard->GetArray("scores");
    ASSERT_NE(scoresGuard, nullptr);
    
    // 验证数组类型
    IJson::JsonType scoresType = scoresGuard->GetType();
    EXPECT_EQ(scoresType, IJson::JsonType::kArray);
}

// 测试空值和null处理
TEST_F(CppxJsonTest, TestNullAndEmptyValues)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试null值（头文件中没有kNull类型，使用kInvalid表示）
    IJson::JsonType nullType = jsonGuard->GetType("metadata");
    EXPECT_EQ(nullType, IJson::JsonType::kInvalid);
    
    // 测试空字符串
    jsonGuard->SetString("emptyString", "");
    const char* emptyStr = jsonGuard->GetString("emptyString");
    ASSERT_NE(emptyStr, nullptr);
    EXPECT_STREQ(emptyStr, "");
    
    // 测试零值
    jsonGuard->SetInt32("zeroValue", 0);
    int32_t zero = jsonGuard->GetInt32("zeroValue");
    EXPECT_EQ(zero, 0);
    
    // 测试false值
    jsonGuard->SetBool("falseValue", false);
    bool falseVal = jsonGuard->GetBool("falseValue");
    EXPECT_FALSE(falseVal);
}

// 测试特殊字符和Unicode
TEST_F(CppxJsonTest, TestSpecialCharactersAndUnicode)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试包含特殊字符的字符串
    const char* specialStr = "测试字符串 with special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?";
    jsonGuard->SetString("special", specialStr);
    const char* retrieved = jsonGuard->GetString("special");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_STREQ(retrieved, specialStr);
    
    // 测试Unicode字符
    const char* unicodeStr = "中文测试 🚀 emoji测试";
    jsonGuard->SetString("unicode", unicodeStr);
    const char* unicodeRetrieved = jsonGuard->GetString("unicode");
    ASSERT_NE(unicodeRetrieved, nullptr);
    EXPECT_STREQ(unicodeRetrieved, unicodeStr);
    
    // 测试换行符和制表符
    const char* newlineStr = "line1\nline2\twith\ttab";
    jsonGuard->SetString("newlines", newlineStr);
    const char* newlineRetrieved = jsonGuard->GetString("newlines");
    ASSERT_NE(newlineRetrieved, nullptr);
    EXPECT_STREQ(newlineRetrieved, newlineStr);
}

// 测试边界数值
TEST_F(CppxJsonTest, TestBoundaryValues)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试最大和最小int32值
    jsonGuard->SetInt32("maxInt", INT32_MAX);
    int32_t maxInt = jsonGuard->GetInt32("maxInt");
    EXPECT_EQ(maxInt, INT32_MAX);
    
    jsonGuard->SetInt32("minInt", INT32_MIN);
    int32_t minInt = jsonGuard->GetInt32("minInt");
    EXPECT_EQ(minInt, INT32_MIN);
    
    // 测试零值
    jsonGuard->SetInt32("zero", 0);
    int32_t zero = jsonGuard->GetInt32("zero");
    EXPECT_EQ(zero, 0);
    
    // 测试负数
    jsonGuard->SetInt32("negative", -12345);
    int32_t negative = jsonGuard->GetInt32("negative");
    EXPECT_EQ(negative, -12345);
}

// 测试内存管理
TEST_F(CppxJsonTest, TestMemoryManagement)
{
    // 测试多次创建和销毁
    for (int i = 0; i < 100; ++i)
    {
        JsonGuard jsonGuard;
        ASSERT_NE(jsonGuard.get(), nullptr);
        
        jsonGuard->SetString("test", "value");
        jsonGuard->SetInt32("number", i);
        
        const char* str = jsonGuard->ToString();
        ASSERT_NE(str, nullptr);
        
        // Guard对象会在作用域结束时自动销毁
    }
    
    // 测试手动创建和销毁
    IJson* pJson = IJson::Create();
    ASSERT_NE(pJson, nullptr);
    pJson->SetString("manual", "test");
    IJson::Destroy(pJson);
}

// 测试并发安全性（基本测试）
TEST_F(CppxJsonTest, TestBasicConcurrency)
{
    // 注意：根据接口文档，Create和Destroy是线程安全的
    // 但其他操作不是线程安全的，这里只测试基本的创建销毁
    
    std::vector<std::thread> threads;
    const int numThreads = 10;
    const int operationsPerThread = 100;
    
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([operationsPerThread]() {
            for (int i = 0; i < operationsPerThread; ++i)
            {
                JsonGuard jsonGuard;
                ASSERT_NE(jsonGuard.get(), nullptr);
                
                jsonGuard->SetString("thread_test", "value");
                jsonGuard->SetInt32("thread_id", i);
                
                const char* str = jsonGuard->ToString();
                ASSERT_NE(str, nullptr);
            }
        });
    }
    
    for (auto& thread : threads)
    {
        thread.join();
    }
}

// 测试错误恢复
TEST_F(CppxJsonTest, TestErrorRecovery)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先设置一些有效数据
    jsonGuard->SetString("valid", "data");
    jsonGuard->SetInt32("number", 42);
    
    // 尝试解析无效JSON（应该失败但不影响现有数据）
    const char* invalidJson = R"({"invalid": json)";
    int32_t result = jsonGuard->Parse(invalidJson);
    EXPECT_NE(result, 0);
    
    // 验证原有数据仍然存在
    const char* validData = jsonGuard->GetString("valid");
    EXPECT_STREQ(validData, "data");
    
    int32_t number = jsonGuard->GetInt32("number");
    EXPECT_EQ(number, 42);
    
    // 现在解析有效JSON
    const char* validJson = R"({"new": "data", "value": 123})";
    result = jsonGuard->Parse(validJson);
    EXPECT_EQ(result, 0);
    
    // 验证新数据
    const char* newData = jsonGuard->GetString("new");
    EXPECT_STREQ(newData, "data");
    
    int32_t value = jsonGuard->GetInt32("value");
    EXPECT_EQ(value, 123);
}

// 测试数组索引访问接口
TEST_F(CppxJsonTest, TestArrayIndexAccess)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析包含数组的测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 获取hobbies数组
    const IJson* hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard, nullptr);
    
    // 测试GetString通过索引访问（需要明确指定uint32_t类型）
    const char* hobby1 = hobbiesGuard->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(hobby1, nullptr);
    EXPECT_STREQ(hobby1, "读书");
    
    const char* hobby2 = hobbiesGuard->GetString(static_cast<uint32_t>(1));
    ASSERT_NE(hobby2, nullptr);
    EXPECT_STREQ(hobby2, "游泳");
    
    const char* hobby3 = hobbiesGuard->GetString(static_cast<uint32_t>(2));
    ASSERT_NE(hobby3, nullptr);
    EXPECT_STREQ(hobby3, "编程");
    
    // 测试GetString通过索引访问with default
    const char* defaultHobby = hobbiesGuard->GetString(static_cast<uint32_t>(10), "默认爱好");
    EXPECT_STREQ(defaultHobby, "默认爱好");
    
    // 获取scores数组
    const IJson* scoresGuard = jsonGuard->GetArray("scores");
    ASSERT_NE(scoresGuard, nullptr);
    
    // 测试GetInt32通过索引访问（需要明确指定uint32_t类型）
    int32_t score1 = scoresGuard->GetInt32(static_cast<uint32_t>(0));
    EXPECT_EQ(score1, 95);
    
    int32_t score2 = scoresGuard->GetInt32(static_cast<uint32_t>(1));
    EXPECT_EQ(score2, 87);
    
    int32_t score3 = scoresGuard->GetInt32(static_cast<uint32_t>(2));
    EXPECT_EQ(score3, 92);
    
    // 测试GetInt32通过索引访问with default
    int32_t defaultScore = scoresGuard->GetInt32(static_cast<uint32_t>(10), 999);
    EXPECT_EQ(defaultScore, 999);
    
    // 测试数组越界访问（负数索引会被转换为很大的uint32_t）
    int32_t outOfBoundsInt = scoresGuard->GetInt32(static_cast<uint32_t>(-1));
    EXPECT_EQ(outOfBoundsInt, 0);
    
    // 测试GetType通过索引访问（需要明确指定uint32_t类型）
    IJson::JsonType strType = hobbiesGuard->GetType(static_cast<uint32_t>(0));
    EXPECT_EQ(strType, IJson::JsonType::kString);
    
    IJson::JsonType intType = scoresGuard->GetType(static_cast<uint32_t>(0));
    EXPECT_EQ(intType, IJson::JsonType::kInt64);
}

// 测试数组追加接口
TEST_F(CppxJsonTest, TestArrayAppendOperations)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个数组类型的JSON对象
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试AppendString
    int32_t result = arrayGuard->AppendString("第一个字符串");
    EXPECT_EQ(result, 0);
    
    result = arrayGuard->AppendString("第二个字符串");
    EXPECT_EQ(result, 0);
    
    // 验证追加的字符串（需要明确指定uint32_t类型）
    const char* str1 = arrayGuard->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(str1, nullptr);
    EXPECT_STREQ(str1, "第一个字符串");
    
    const char* str2 = arrayGuard->GetString(static_cast<uint32_t>(1));
    ASSERT_NE(str2, nullptr);
    EXPECT_STREQ(str2, "第二个字符串");
    
    // 测试AppendInt32
    result = arrayGuard->AppendInt32(100);
    EXPECT_EQ(result, 0);
    
    result = arrayGuard->AppendInt32(200);
    EXPECT_EQ(result, 0);
    
    // 验证追加的整数（需要明确指定uint32_t类型）
    int32_t int1 = arrayGuard->GetInt32(static_cast<uint32_t>(2));
    EXPECT_EQ(int1, 100);
    
    int32_t int2 = arrayGuard->GetInt32(static_cast<uint32_t>(3));
    EXPECT_EQ(int2, 200);
    
    // 测试AppendBool
    result = arrayGuard->AppendBool(true);
    EXPECT_EQ(result, 0);
    
    result = arrayGuard->AppendBool(false);
    EXPECT_EQ(result, 0);
    
    // 验证追加的布尔值（需要明确指定uint32_t类型）
    bool bool1 = arrayGuard->GetBool(static_cast<uint32_t>(4));
    EXPECT_TRUE(bool1);
    
    bool bool2 = arrayGuard->GetBool(static_cast<uint32_t>(5));
    EXPECT_FALSE(bool2);
    
    // 测试AppendObject
    JsonGuard subObjGuard;
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "子对象");
    subObjGuard->SetInt32("value", 42);
    
    result = arrayGuard->AppendObject(subObjGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证追加的对象（需要明确指定uint32_t类型）
    const IJson* retrievedObj = arrayGuard->GetObject(static_cast<uint32_t>(6));
    ASSERT_NE(retrievedObj, nullptr);
    const char* objName = retrievedObj->GetString("name");
    ASSERT_NE(objName, nullptr);
    EXPECT_STREQ(objName, "子对象");
    
    int32_t objValue = retrievedObj->GetInt32("value");
    EXPECT_EQ(objValue, 42);
    
    // 测试AppendArray
    JsonGuard subArrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("数组元素1");
    subArrayGuard->AppendInt32(123);
    
    result = arrayGuard->AppendArray(subArrayGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证追加的数组（需要明确指定uint32_t类型）
    const IJson* retrievedArray = arrayGuard->GetArray(static_cast<uint32_t>(7));
    ASSERT_NE(retrievedArray, nullptr);
    const char* arrayStr = retrievedArray->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(arrayStr, nullptr);
    EXPECT_STREQ(arrayStr, "数组元素1");
    
    int32_t arrayInt = retrievedArray->GetInt32(static_cast<uint32_t>(1));
    EXPECT_EQ(arrayInt, 123);
    
    // 测试无效参数
    result = arrayGuard->AppendString(nullptr);
    EXPECT_NE(result, 0);
    
    result = arrayGuard->AppendObject(nullptr);
    EXPECT_NE(result, 0);
    
    result = arrayGuard->AppendArray(nullptr);
    EXPECT_NE(result, 0);
}

// 测试GetSize接口
TEST_F(CppxJsonTest, TestGetSize)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试对象的大小（应该包含所有键值对）
    uint32_t objectSize = jsonGuard->GetSize();
    EXPECT_GT(objectSize, 0);
    
    // 获取hobbies数组并测试其大小
    const IJson* hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard, nullptr);
    uint32_t hobbiesSize = hobbiesGuard->GetSize();
    EXPECT_EQ(hobbiesSize, 3); // 根据测试数据，hobbies数组有3个元素
    
    // 获取scores数组并测试其大小
    const IJson* scoresGuard = jsonGuard->GetArray("scores");
    ASSERT_NE(scoresGuard, nullptr);
    uint32_t scoresSize = scoresGuard->GetSize();
    EXPECT_EQ(scoresSize, 3); // 根据测试数据，scores数组有3个元素
    
    // 测试空数组的大小
    JsonGuard emptyArrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(emptyArrayGuard.get(), nullptr);
    uint32_t emptyArraySize = emptyArrayGuard->GetSize();
    EXPECT_EQ(emptyArraySize, 0);
    
    // 测试空对象的大小
    JsonGuard emptyObjGuard(IJson::JsonType::kObject);
    ASSERT_NE(emptyObjGuard.get(), nullptr);
    uint32_t emptyObjSize = emptyObjGuard->GetSize();
    EXPECT_EQ(emptyObjSize, 0);
    
    // 测试基本类型的大小（应该返回0）
    // 注意：头文件中没有kString, kInt32等作为独立类型创建，这些是值类型
    // 对象类型只能是kObject或kArray
}

// 测试数组索引访问的GetType接口
TEST_F(CppxJsonTest, TestGetTypeByIndex)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个包含不同类型元素的数组
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 添加不同类型的元素
    arrayGuard->AppendString("字符串元素");
    arrayGuard->AppendInt32(42);
    arrayGuard->AppendBool(true);
    
    // 添加一个对象
    JsonGuard subObjGuard;
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "子对象");
    arrayGuard->AppendObject(subObjGuard.get());
    
    // 添加一个数组
    JsonGuard subArrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("数组元素");
    arrayGuard->AppendArray(subArrayGuard.get());
    
    // 测试通过索引获取类型（需要明确指定uint32_t类型）
    IJson::JsonType type0 = arrayGuard->GetType(static_cast<uint32_t>(0));
    EXPECT_EQ(type0, IJson::JsonType::kString);
    
    IJson::JsonType type1 = arrayGuard->GetType(static_cast<uint32_t>(1));
    EXPECT_EQ(type1, IJson::JsonType::kInt64);
    
    IJson::JsonType type2 = arrayGuard->GetType(static_cast<uint32_t>(2));
    EXPECT_EQ(type2, IJson::JsonType::kBool);
    
    IJson::JsonType type3 = arrayGuard->GetType(static_cast<uint32_t>(3));
    EXPECT_EQ(type3, IJson::JsonType::kObject);
    
    IJson::JsonType type4 = arrayGuard->GetType(static_cast<uint32_t>(4));
    EXPECT_EQ(type4, IJson::JsonType::kArray);
    
    // 测试越界访问
    IJson::JsonType outOfBoundsType = arrayGuard->GetType(static_cast<uint32_t>(10));
    EXPECT_EQ(outOfBoundsType, IJson::JsonType::kInvalid);
    
    // 测试负数索引（会被转换为很大的uint32_t）
    IJson::JsonType negativeType = arrayGuard->GetType(static_cast<uint32_t>(-1));
    EXPECT_EQ(negativeType, IJson::JsonType::kInvalid);
}

// 测试数组操作的边界情况
TEST_F(CppxJsonTest, TestArrayBoundaryConditions)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个数组
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试在空数组上访问元素（需要明确指定uint32_t类型）
    const char* emptyStr = arrayGuard->GetString(static_cast<uint32_t>(0));
    EXPECT_EQ(emptyStr, nullptr);
    
    int32_t emptyInt = arrayGuard->GetInt32(static_cast<uint32_t>(0));
    EXPECT_EQ(emptyInt, 0);
    
    bool emptyBool = arrayGuard->GetBool(static_cast<uint32_t>(0));
    EXPECT_FALSE(emptyBool);
    
    const IJson* emptyObj = arrayGuard->GetObject(static_cast<uint32_t>(0));
    EXPECT_EQ(emptyObj, nullptr);
    
    const IJson* emptyArray = arrayGuard->GetArray(static_cast<uint32_t>(0));
    EXPECT_EQ(emptyArray, nullptr);
    
    // 添加一些元素
    arrayGuard->AppendString("test");
    arrayGuard->AppendInt32(123);
    arrayGuard->AppendBool(false);
    
    // 测试获取数组大小
    uint32_t size = arrayGuard->GetSize();
    EXPECT_EQ(size, 3);
    
    // 测试访问各个元素（需要明确指定uint32_t类型）
    const char* firstStr = arrayGuard->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(firstStr, nullptr);
    EXPECT_STREQ(firstStr, "test");
    
    int32_t secondInt = arrayGuard->GetInt32(static_cast<uint32_t>(1));
    EXPECT_EQ(secondInt, 123);
    
    bool thirdBool = arrayGuard->GetBool(static_cast<uint32_t>(2));
    EXPECT_FALSE(thirdBool);
    
    // 测试访问超出范围的元素（需要明确指定uint32_t类型）
    const char* outOfRangeStr = arrayGuard->GetString(static_cast<uint32_t>(10), "默认值");
    EXPECT_STREQ(outOfRangeStr, "默认值");
    
    int32_t outOfRangeInt = arrayGuard->GetInt32(static_cast<uint32_t>(10), 999);
    EXPECT_EQ(outOfRangeInt, 999);
    
    bool outOfRangeBool = arrayGuard->GetBool(static_cast<uint32_t>(10), true);
    EXPECT_TRUE(outOfRangeBool);
}

// 测试数组和对象的混合操作
TEST_F(CppxJsonTest, TestArrayObjectMixedOperations)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个包含数组的对象
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 在数组中添加不同类型的元素
    arrayGuard->AppendString("字符串");
    arrayGuard->AppendInt32(100);
    arrayGuard->AppendBool(true);
    
    // 创建一个子对象并添加到数组
    JsonGuard subObjGuard;
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "数组中的对象");
    subObjGuard->SetInt32("id", 1);
    arrayGuard->AppendObject(subObjGuard.get());
    
    // 创建一个子数组并添加到数组
    JsonGuard subArrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("子数组元素1");
    subArrayGuard->AppendString("子数组元素2");
    arrayGuard->AppendArray(subArrayGuard.get());
    
    // 将数组设置到主对象
    int32_t result = jsonGuard->SetArray("mixedArray", arrayGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证混合数组的内容
    const IJson* retrievedArray = jsonGuard->GetArray("mixedArray");
    ASSERT_NE(retrievedArray, nullptr);
    
    // 验证数组大小
    uint32_t arraySize = retrievedArray->GetSize();
    EXPECT_EQ(arraySize, 5);
    
    // 验证字符串元素（需要明确指定uint32_t类型）
    const char* str = retrievedArray->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "字符串");
    
    // 验证整数元素（需要明确指定uint32_t类型）
    int32_t intVal = retrievedArray->GetInt32(static_cast<uint32_t>(1));
    EXPECT_EQ(intVal, 100);
    
    // 验证布尔元素（需要明确指定uint32_t类型）
    bool boolVal = retrievedArray->GetBool(static_cast<uint32_t>(2));
    EXPECT_TRUE(boolVal);
    
    // 验证对象元素（需要明确指定uint32_t类型）
    const IJson* obj = retrievedArray->GetObject(static_cast<uint32_t>(3));
    ASSERT_NE(obj, nullptr);
    const char* objName = obj->GetString("name");
    ASSERT_NE(objName, nullptr);
    EXPECT_STREQ(objName, "数组中的对象");
    
    int32_t objId = obj->GetInt32("id");
    EXPECT_EQ(objId, 1);
    
    // 验证数组元素（需要明确指定uint32_t类型）
    const IJson* arr = retrievedArray->GetArray(static_cast<uint32_t>(4));
    ASSERT_NE(arr, nullptr);
    uint32_t subArraySize = arr->GetSize();
    EXPECT_EQ(subArraySize, 2);
    
    const char* subStr1 = arr->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(subStr1, nullptr);
    EXPECT_STREQ(subStr1, "子数组元素1");
    
    const char* subStr2 = arr->GetString(static_cast<uint32_t>(1));
    ASSERT_NE(subStr2, nullptr);
    EXPECT_STREQ(subStr2, "子数组元素2");
}

// 测试GetInt64、GetUint32、GetUint64、GetDouble接口（通过key）
TEST_F(CppxJsonTest, TestGetNumericTypesByKey)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 设置各种数值类型
    jsonGuard->SetInt64("int64Value", INT64_MAX);
    jsonGuard->SetUint32("uint32Value", UINT32_MAX);
    jsonGuard->SetUint64("uint64Value", UINT64_MAX);
    jsonGuard->SetDouble("doubleValue", 3.141592653589793);
    
    // 测试GetInt64
    int64_t int64Val = jsonGuard->GetInt64("int64Value");
    EXPECT_EQ(int64Val, INT64_MAX);
    
    int64_t int64Default = jsonGuard->GetInt64("nonexistent", -1);
    EXPECT_EQ(int64Default, -1);
    
    // 测试GetUint32
    uint32_t uint32Val = jsonGuard->GetUint32("uint32Value");
    EXPECT_EQ(uint32Val, UINT32_MAX);
    
    uint32_t uint32Default = jsonGuard->GetUint32("nonexistent", 999);
    EXPECT_EQ(uint32Default, 999);
    
    // 测试GetUint64
    uint64_t uint64Val = jsonGuard->GetUint64("uint64Value");
    EXPECT_EQ(uint64Val, UINT64_MAX);
    
    uint64_t uint64Default = jsonGuard->GetUint64("nonexistent", 888);
    EXPECT_EQ(uint64Default, 888);
    
    // 测试GetDouble
    double doubleVal = jsonGuard->GetDouble("doubleValue");
    EXPECT_DOUBLE_EQ(doubleVal, 3.141592653589793);
    
    double doubleDefault = jsonGuard->GetDouble("nonexistent", 2.718);
    EXPECT_DOUBLE_EQ(doubleDefault, 2.718);
}

// 测试SetInt64、SetUint32、SetUint64、SetDouble接口
TEST_F(CppxJsonTest, TestSetNumericTypes)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试SetInt64
    int32_t result = jsonGuard->SetInt64("int64Key", INT64_MIN);
    EXPECT_EQ(result, 0);
    int64_t int64Val = jsonGuard->GetInt64("int64Key");
    EXPECT_EQ(int64Val, INT64_MIN);
    
    // 测试SetUint32
    result = jsonGuard->SetUint32("uint32Key", 4294967295U);
    EXPECT_EQ(result, 0);
    uint32_t uint32Val = jsonGuard->GetUint32("uint32Key");
    EXPECT_EQ(uint32Val, 4294967295U);
    
    // 测试SetUint64
    result = jsonGuard->SetUint64("uint64Key", UINT64_MAX);
    EXPECT_EQ(result, 0);
    uint64_t uint64Val = jsonGuard->GetUint64("uint64Key");
    EXPECT_EQ(uint64Val, UINT64_MAX);
    
    // 测试SetDouble
    result = jsonGuard->SetDouble("doubleKey", -123.456789);
    EXPECT_EQ(result, 0);
    double doubleVal = jsonGuard->GetDouble("doubleKey");
    EXPECT_DOUBLE_EQ(doubleVal, -123.456789);
    
    // 测试无效参数
    result = jsonGuard->SetInt64(nullptr, 0);
    EXPECT_NE(result, 0);
    
    result = jsonGuard->SetUint32(nullptr, 0);
    EXPECT_NE(result, 0);
    
    result = jsonGuard->SetUint64(nullptr, 0);
    EXPECT_NE(result, 0);
    
    result = jsonGuard->SetDouble(nullptr, 0.0);
    EXPECT_NE(result, 0);
}

// 测试GetInt64、GetUint32、GetUint64、GetDouble接口（通过index）
TEST_F(CppxJsonTest, TestGetNumericTypesByIndex)
{
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 添加各种数值类型
    arrayGuard->AppendInt64(INT64_MAX);
    arrayGuard->AppendUint32(UINT32_MAX);
    arrayGuard->AppendUint64(UINT64_MAX);
    arrayGuard->AppendDouble(2.718281828);
    
    // 测试GetInt64通过索引
    int64_t int64Val = arrayGuard->GetInt64(static_cast<uint32_t>(0));
    EXPECT_EQ(int64Val, INT64_MAX);
    
    int64_t int64Default = arrayGuard->GetInt64(static_cast<uint32_t>(10), -1);
    EXPECT_EQ(int64Default, -1);
    
    // 测试GetUint32通过索引
    uint32_t uint32Val = arrayGuard->GetUint32(static_cast<uint32_t>(1));
    EXPECT_EQ(uint32Val, UINT32_MAX);
    
    uint32_t uint32Default = arrayGuard->GetUint32(static_cast<uint32_t>(10), 999);
    EXPECT_EQ(uint32Default, 999);
    
    // 测试GetUint64通过索引
    uint64_t uint64Val = arrayGuard->GetUint64(static_cast<uint32_t>(2));
    EXPECT_EQ(uint64Val, UINT64_MAX);
    
    uint64_t uint64Default = arrayGuard->GetUint64(static_cast<uint32_t>(10), 888);
    EXPECT_EQ(uint64Default, 888);
    
    // 测试GetDouble通过索引
    double doubleVal = arrayGuard->GetDouble(static_cast<uint32_t>(3));
    EXPECT_DOUBLE_EQ(doubleVal, 2.718281828);
    
    double doubleDefault = arrayGuard->GetDouble(static_cast<uint32_t>(10), 1.414);
    EXPECT_DOUBLE_EQ(doubleDefault, 1.414);
}

// 测试AppendInt64、AppendUint32、AppendUint64、AppendDouble接口
TEST_F(CppxJsonTest, TestAppendNumericTypes)
{
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试AppendInt64
    int32_t result = arrayGuard->AppendInt64(INT64_MIN);
    EXPECT_EQ(result, 0);
    int64_t int64Val = arrayGuard->GetInt64(static_cast<uint32_t>(0));
    EXPECT_EQ(int64Val, INT64_MIN);
    
    // 测试AppendUint32
    result = arrayGuard->AppendUint32(1234567890U);
    EXPECT_EQ(result, 0);
    uint32_t uint32Val = arrayGuard->GetUint32(static_cast<uint32_t>(1));
    EXPECT_EQ(uint32Val, 1234567890U);
    
    // 测试AppendUint64
    result = arrayGuard->AppendUint64(UINT64_MAX);
    EXPECT_EQ(result, 0);
    uint64_t uint64Val = arrayGuard->GetUint64(static_cast<uint32_t>(2));
    EXPECT_EQ(uint64Val, UINT64_MAX);
    
    // 测试AppendDouble
    result = arrayGuard->AppendDouble(1.414213562);
    EXPECT_EQ(result, 0);
    double doubleVal = arrayGuard->GetDouble(static_cast<uint32_t>(3));
    EXPECT_DOUBLE_EQ(doubleVal, 1.414213562);
    
    // 验证数组大小
    uint32_t size = arrayGuard->GetSize();
    EXPECT_EQ(size, 4);
}

// 测试GetObject拷贝版本（通过key）
TEST_F(CppxJsonTest, TestGetObjectCopyByKey)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 创建一个目标对象用于接收拷贝
    JsonGuard targetGuard;
    ASSERT_NE(targetGuard.get(), nullptr);
    
    // 测试GetObject拷贝版本
    result = jsonGuard->GetObject("address", targetGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证拷贝的对象内容
    const char* city = targetGuard->GetString("city");
    ASSERT_NE(city, nullptr);
    EXPECT_STREQ(city, "北京");
    
    const char* zipCode = targetGuard->GetString("zipCode");
    ASSERT_NE(zipCode, nullptr);
    EXPECT_STREQ(zipCode, "100000");
    
    // 测试获取不存在的对象
    JsonGuard emptyGuard;
    result = jsonGuard->GetObject("nonexistent", emptyGuard.get());
    EXPECT_NE(result, 0);
    
    // 测试无效参数
    result = jsonGuard->GetObject(nullptr, targetGuard.get());
    EXPECT_NE(result, 0);
    
    result = jsonGuard->GetObject("address", nullptr);
    EXPECT_NE(result, 0);
}

// 测试GetArray拷贝版本（通过key）
TEST_F(CppxJsonTest, TestGetArrayCopyByKey)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 创建一个目标数组用于接收拷贝
    JsonGuard targetGuard(IJson::JsonType::kArray);
    ASSERT_NE(targetGuard.get(), nullptr);
    
    // 测试GetArray拷贝版本
    result = jsonGuard->GetArray("hobbies", targetGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证拷贝的数组内容
    uint32_t size = targetGuard->GetSize();
    EXPECT_EQ(size, 3);
    
    const char* hobby1 = targetGuard->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(hobby1, nullptr);
    EXPECT_STREQ(hobby1, "读书");
    
    const char* hobby2 = targetGuard->GetString(static_cast<uint32_t>(1));
    ASSERT_NE(hobby2, nullptr);
    EXPECT_STREQ(hobby2, "游泳");
    
    // 测试获取不存在的数组
    JsonGuard emptyGuard(IJson::JsonType::kArray);
    result = jsonGuard->GetArray("nonexistent", emptyGuard.get());
    EXPECT_NE(result, 0);
    
    // 测试无效参数
    result = jsonGuard->GetArray(nullptr, targetGuard.get());
    EXPECT_NE(result, 0);
    
    result = jsonGuard->GetArray("hobbies", nullptr);
    EXPECT_NE(result, 0);
}

// 测试GetObject拷贝版本（通过index）
TEST_F(CppxJsonTest, TestGetObjectCopyByIndex)
{
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 添加一个对象到数组
    JsonGuard subObjGuard;
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "测试对象");
    subObjGuard->SetInt32("id", 100);
    arrayGuard->AppendObject(subObjGuard.get());
    
    // 创建一个目标对象用于接收拷贝
    JsonGuard targetGuard;
    ASSERT_NE(targetGuard.get(), nullptr);
    
    // 测试GetObject拷贝版本（通过index）
    int32_t result = arrayGuard->GetObject(static_cast<uint32_t>(0), targetGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证拷贝的对象内容
    const char* name = targetGuard->GetString("name");
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "测试对象");
    
    int32_t id = targetGuard->GetInt32("id");
    EXPECT_EQ(id, 100);
    
    // 测试越界访问
    JsonGuard emptyGuard;
    result = arrayGuard->GetObject(static_cast<uint32_t>(10), emptyGuard.get());
    EXPECT_NE(result, 0);
    
    // 测试无效参数
    result = arrayGuard->GetObject(static_cast<uint32_t>(0), nullptr);
    EXPECT_NE(result, 0);
}

// 测试GetArray拷贝版本（通过index）
TEST_F(CppxJsonTest, TestGetArrayCopyByIndex)
{
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 添加一个数组到数组
    JsonGuard subArrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("元素1");
    subArrayGuard->AppendString("元素2");
    arrayGuard->AppendArray(subArrayGuard.get());
    
    // 创建一个目标数组用于接收拷贝
    JsonGuard targetGuard(IJson::JsonType::kArray);
    ASSERT_NE(targetGuard.get(), nullptr);
    
    // 测试GetArray拷贝版本（通过index）
    int32_t result = arrayGuard->GetArray(static_cast<uint32_t>(0), targetGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证拷贝的数组内容
    uint32_t size = targetGuard->GetSize();
    EXPECT_EQ(size, 2);
    
    const char* elem1 = targetGuard->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(elem1, nullptr);
    EXPECT_STREQ(elem1, "元素1");
    
    const char* elem2 = targetGuard->GetString(static_cast<uint32_t>(1));
    ASSERT_NE(elem2, nullptr);
    EXPECT_STREQ(elem2, "元素2");
    
    // 测试越界访问
    JsonGuard emptyGuard(IJson::JsonType::kArray);
    result = arrayGuard->GetArray(static_cast<uint32_t>(10), emptyGuard.get());
    EXPECT_NE(result, 0);
    
    // 测试无效参数
    result = arrayGuard->GetArray(static_cast<uint32_t>(0), nullptr);
    EXPECT_NE(result, 0);
}

// 测试SetObject零拷贝版本（返回IJson*）
TEST_F(CppxJsonTest, TestSetObjectZeroCopy)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试SetObject零拷贝版本
    IJson* subObj = jsonGuard->SetObject("subObject");
    ASSERT_NE(subObj, nullptr);
    
    // 在返回的对象上设置值
    int32_t result = subObj->SetString("name", "零拷贝对象");
    EXPECT_EQ(result, 0);
    result = subObj->SetInt32("value", 42);
    EXPECT_EQ(result, 0);
    
    // 验证设置成功
    const IJson* retrievedObj = jsonGuard->GetObject("subObject");
    ASSERT_NE(retrievedObj, nullptr);
    const char* name = retrievedObj->GetString("name");
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "零拷贝对象");
    
    int32_t value = retrievedObj->GetInt32("value");
    EXPECT_EQ(value, 42);
    
    // 测试无效参数
    IJson* nullObj = jsonGuard->SetObject(nullptr);
    EXPECT_EQ(nullObj, nullptr);
}

// 测试SetArray零拷贝版本（返回IJson*）
TEST_F(CppxJsonTest, TestSetArrayZeroCopy)
{
    JsonGuard jsonGuard;
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试SetArray零拷贝版本
    IJson* subArray = jsonGuard->SetArray("subArray");
    ASSERT_NE(subArray, nullptr);
    
    // 在返回的数组上添加元素
    int32_t result = subArray->AppendString("数组元素1");
    EXPECT_EQ(result, 0);
    result = subArray->AppendInt32(123);
    EXPECT_EQ(result, 0);
    
    // 验证设置成功
    const IJson* retrievedArray = jsonGuard->GetArray("subArray");
    ASSERT_NE(retrievedArray, nullptr);
    uint32_t size = retrievedArray->GetSize();
    EXPECT_EQ(size, 2);
    
    const char* str = retrievedArray->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "数组元素1");
    
    int32_t intVal = retrievedArray->GetInt32(static_cast<uint32_t>(1));
    EXPECT_EQ(intVal, 123);
    
    // 测试无效参数
    IJson* nullArray = jsonGuard->SetArray(nullptr);
    EXPECT_EQ(nullArray, nullptr);
}

// 测试AppendObject零拷贝版本（返回IJson*）
TEST_F(CppxJsonTest, TestAppendObjectZeroCopy)
{
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试AppendObject零拷贝版本
    IJson* subObj = arrayGuard->AppendObject();
    ASSERT_NE(subObj, nullptr);
    
    // 在返回的对象上设置值
    int32_t result = subObj->SetString("name", "追加的对象");
    EXPECT_EQ(result, 0);
    result = subObj->SetInt32("id", 200);
    EXPECT_EQ(result, 0);
    
    // 验证追加成功
    uint32_t size = arrayGuard->GetSize();
    EXPECT_EQ(size, 1);
    
    const IJson* retrievedObj = arrayGuard->GetObject(static_cast<uint32_t>(0));
    ASSERT_NE(retrievedObj, nullptr);
    const char* name = retrievedObj->GetString("name");
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "追加的对象");
    
    int32_t id = retrievedObj->GetInt32("id");
    EXPECT_EQ(id, 200);
}

// 测试AppendArray零拷贝版本（返回IJson*）
TEST_F(CppxJsonTest, TestAppendArrayZeroCopy)
{
    JsonGuard arrayGuard(IJson::JsonType::kArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试AppendArray零拷贝版本
    IJson* subArray = arrayGuard->AppendArray();
    ASSERT_NE(subArray, nullptr);
    
    // 在返回的数组上添加元素
    int32_t result = subArray->AppendString("子数组元素");
    EXPECT_EQ(result, 0);
    result = subArray->AppendBool(true);
    EXPECT_EQ(result, 0);
    
    // 验证追加成功
    uint32_t size = arrayGuard->GetSize();
    EXPECT_EQ(size, 1);
    
    const IJson* retrievedArray = arrayGuard->GetArray(static_cast<uint32_t>(0));
    ASSERT_NE(retrievedArray, nullptr);
    uint32_t subSize = retrievedArray->GetSize();
    EXPECT_EQ(subSize, 2);
    
    const char* str = retrievedArray->GetString(static_cast<uint32_t>(0));
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "子数组元素");
    
    bool boolVal = retrievedArray->GetBool(static_cast<uint32_t>(1));
    EXPECT_TRUE(boolVal);
}

// 测试Create方法的不同类型参数
TEST_F(CppxJsonTest, TestCreateWithDifferentTypes)
{
    // 测试创建对象类型
    IJson* objJson = IJson::Create(IJson::JsonType::kObject);
    ASSERT_NE(objJson, nullptr);
    EXPECT_EQ(objJson->GetType(), IJson::JsonType::kObject);
    IJson::Destroy(objJson);
    
    // 测试创建数组类型
    IJson* arrayJson = IJson::Create(IJson::JsonType::kArray);
    ASSERT_NE(arrayJson, nullptr);
    EXPECT_EQ(arrayJson->GetType(), IJson::JsonType::kArray);
    IJson::Destroy(arrayJson);
}

