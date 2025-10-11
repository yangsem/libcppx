#include <gtest/gtest.h>
#include <cppx_json.h>
#include <fstream>
#include <cstring>
#include <thread>
#include <vector>
#include <climits>

using namespace cppx::base;

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
    
    // 测试CreateWithGuard方法
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试销毁
    IJson::Destroy(pJson);
}

// 测试JSON字符串解析
TEST_F(CppxJsonTest, TestParseString)
{
    auto jsonGuard = IJson::CreateWithGuard();
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
    auto jsonGuard = IJson::CreateWithGuard();
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
    auto jsonGuard = IJson::CreateWithGuard();
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
    
    // 测试GetInt
    int32_t age = jsonGuard->GetInt("age");
    EXPECT_EQ(age, 25);
    
    // 测试GetInt with default
    int32_t defaultInt = jsonGuard->GetInt("nonexistent", 999);
    EXPECT_EQ(defaultInt, 999);
    
    // 测试GetBool
    bool isActive = jsonGuard->GetBool("isActive");
    EXPECT_TRUE(isActive);
    
    // 测试GetBool with default
    bool defaultBool = jsonGuard->GetBool("nonexistent", false);
    EXPECT_FALSE(defaultBool);
    
    // 测试GetObject
    auto addressGuard = jsonGuard->GetObject("address");
    ASSERT_NE(addressGuard.get(), nullptr);
    const char* city = addressGuard->GetString("city");
    ASSERT_NE(city, nullptr);
    EXPECT_STREQ(city, "北京");
    
    // 测试GetArray
    auto hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard.get(), nullptr);
    
    // 测试获取不存在的对象
    auto nonexistentGuard = jsonGuard->GetObject("nonexistent");
    EXPECT_EQ(nonexistentGuard.get(), nullptr);
    
    // 测试获取不存在的数组
    auto nonexistentArray = jsonGuard->GetArray("nonexistent");
    EXPECT_EQ(nonexistentArray.get(), nullptr);
}

// 测试Set操作
TEST_F(CppxJsonTest, TestSetOperations)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试SetString
    int32_t result = jsonGuard->SetString("name", "新用户");
    EXPECT_EQ(result, 0);
    const char* name = jsonGuard->GetString("name");
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "新用户");
    
    // 测试SetInt
    result = jsonGuard->SetInt("age", 30);
    EXPECT_EQ(result, 0);
    int32_t age = jsonGuard->GetInt("age");
    EXPECT_EQ(age, 30);
    
    // 测试SetBool
    result = jsonGuard->SetBool("isActive", false);
    EXPECT_EQ(result, 0);
    bool isActive = jsonGuard->GetBool("isActive");
    EXPECT_FALSE(isActive);
    
    // 测试SetObject
    auto subJsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(subJsonGuard.get(), nullptr);
    subJsonGuard->SetString("country", "中国");
    subJsonGuard->SetString("province", "北京");
    
    result = jsonGuard->SetObject("location", subJsonGuard.get());
    EXPECT_EQ(result, 0);
    
    auto locationGuard = jsonGuard->GetObject("location");
    ASSERT_NE(locationGuard.get(), nullptr);
    const char* country = locationGuard->GetString("country");
    ASSERT_NE(country, nullptr);
    EXPECT_STREQ(country, "中国");
    
    // 测试SetArray
    auto arrayJsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(arrayJsonGuard.get(), nullptr);
    // 注意：这里需要根据实际的数组设置方法来实现
    
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

// 测试ToString功能
TEST_F(CppxJsonTest, TestToString)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 设置一些数据
    jsonGuard->SetString("name", "测试");
    jsonGuard->SetInt("age", 25);
    jsonGuard->SetBool("active", true);
    
    // 测试普通格式
    auto strGuard = jsonGuard->ToString(false);
    ASSERT_NE(strGuard.get(), nullptr);
    EXPECT_NE(strlen(strGuard.get()), 0);
    
    // 测试美化格式
    auto prettyStrGuard = jsonGuard->ToString(true);
    ASSERT_NE(prettyStrGuard.get(), nullptr);
    EXPECT_NE(strlen(prettyStrGuard.get()), 0);
    
    // 美化格式应该比普通格式长（包含换行和缩进）
    EXPECT_GT(strlen(prettyStrGuard.get()), strlen(strGuard.get()));
}

// 测试GetType功能
TEST_F(CppxJsonTest, TestGetType)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试各种类型的检测
    IJson::JsonType type = jsonGuard->GetType("name");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeString);
    
    type = jsonGuard->GetType("age");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeNumber);
    
    type = jsonGuard->GetType("isActive");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeBoolean);
    
    type = jsonGuard->GetType("address");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeObject);
    
    type = jsonGuard->GetType("hobbies");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeArray);
    
    type = jsonGuard->GetType("metadata");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeNull);
    
    // 测试获取不存在的键的类型
    type = jsonGuard->GetType("nonexistent");
    EXPECT_EQ(type, IJson::JsonType::kJsonTypeNull);
}

// 测试错误处理和边界情况
TEST_F(CppxJsonTest, TestErrorHandling)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试空JSON对象的各种操作
    const char* str = jsonGuard->GetString("nonexistent");
    EXPECT_EQ(str, nullptr);
    
    int32_t intVal = jsonGuard->GetInt("nonexistent");
    EXPECT_EQ(intVal, 0);
    
    bool boolVal = jsonGuard->GetBool("nonexistent");
    EXPECT_FALSE(boolVal);
    
    auto objGuard = jsonGuard->GetObject("nonexistent");
    EXPECT_EQ(objGuard.get(), nullptr);
    
    auto arrGuard = jsonGuard->GetArray("nonexistent");
    EXPECT_EQ(arrGuard.get(), nullptr);
    
    // 测试类型不匹配的情况
    jsonGuard->SetString("testKey", "string value");
    
    // 尝试以错误类型获取
    int32_t wrongType = jsonGuard->GetInt("testKey");
    EXPECT_EQ(wrongType, 0); // 应该返回默认值
    
    bool wrongBool = jsonGuard->GetBool("testKey");
    EXPECT_FALSE(wrongBool); // 应该返回默认值
}

// 测试复杂嵌套结构
TEST_F(CppxJsonTest, TestComplexNestedStructure)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建嵌套结构
    auto userGuard = IJson::CreateWithGuard();
    userGuard->SetString("name", "张三");
    userGuard->SetInt("age", 28);
    
    auto profileGuard = IJson::CreateWithGuard();
    profileGuard->SetString("email", "zhangsan@example.com");
    profileGuard->SetString("phone", "13800138000");
    
    userGuard->SetObject("profile", profileGuard.get());
    
    auto skillsGuard = IJson::CreateWithGuard();
    // 这里需要根据实际的数组操作方法来设置数组元素
    
    userGuard->SetArray("skills", skillsGuard.get());
    
    // 设置到主对象
    jsonGuard->SetObject("user", userGuard.get());
    
    // 验证嵌套结构
    auto retrievedUser = jsonGuard->GetObject("user");
    ASSERT_NE(retrievedUser.get(), nullptr);
    
    const char* userName = retrievedUser->GetString("name");
    ASSERT_NE(userName, nullptr);
    EXPECT_STREQ(userName, "张三");
    
    auto retrievedProfile = retrievedUser->GetObject("profile");
    ASSERT_NE(retrievedProfile.get(), nullptr);
    
    const char* email = retrievedProfile->GetString("email");
    ASSERT_NE(email, nullptr);
    EXPECT_STREQ(email, "zhangsan@example.com");
}

// 测试Guard类的移动语义
TEST_F(CppxJsonTest, TestGuardMoveSemantics)
{
    // 测试JsonGuard的移动构造
    auto guard1 = IJson::CreateWithGuard();
    ASSERT_NE(guard1.get(), nullptr);
    
    auto guard2 = std::move(guard1);
    EXPECT_EQ(guard1.get(), nullptr); // 移动后应该为空
    ASSERT_NE(guard2.get(), nullptr); // 新对象应该有效
    
    // 测试JsonStrGuard的移动构造
    guard2->SetString("test", "value");
    auto strGuard1 = guard2->ToString();
    ASSERT_NE(strGuard1.get(), nullptr);
    
    auto strGuard2 = std::move(strGuard1);
    EXPECT_EQ(strGuard1.get(), nullptr); // 移动后应该为空
    ASSERT_NE(strGuard2.get(), nullptr); // 新对象应该有效
}

// 性能测试 - 大量数据操作
TEST_F(CppxJsonTest, TestPerformanceWithLargeData)
{
    auto jsonGuard = IJson::CreateWithGuard();
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
    auto strGuard = jsonGuard->ToString();
    ASSERT_NE(strGuard.get(), nullptr);
    EXPECT_GT(strlen(strGuard.get()), 0);
}

// 测试数组操作（如果支持的话）
TEST_F(CppxJsonTest, TestArrayOperations)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析包含数组的测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 获取数组并验证
    auto hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard.get(), nullptr);
    
    // 验证数组类型
    IJson::JsonType arrayType = hobbiesGuard->GetType();
    EXPECT_EQ(arrayType, IJson::JsonType::kJsonTypeArray);
    
    // 获取分数数组
    auto scoresGuard = jsonGuard->GetArray("scores");
    ASSERT_NE(scoresGuard.get(), nullptr);
    
    // 验证数组类型
    IJson::JsonType scoresType = scoresGuard->GetType();
    EXPECT_EQ(scoresType, IJson::JsonType::kJsonTypeArray);
}

// 测试空值和null处理
TEST_F(CppxJsonTest, TestNullAndEmptyValues)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试null值
    IJson::JsonType nullType = jsonGuard->GetType("metadata");
    EXPECT_EQ(nullType, IJson::JsonType::kJsonTypeNull);
    
    // 测试空字符串
    jsonGuard->SetString("emptyString", "");
    const char* emptyStr = jsonGuard->GetString("emptyString");
    ASSERT_NE(emptyStr, nullptr);
    EXPECT_STREQ(emptyStr, "");
    
    // 测试零值
    jsonGuard->SetInt("zeroValue", 0);
    int32_t zero = jsonGuard->GetInt("zeroValue");
    EXPECT_EQ(zero, 0);
    
    // 测试false值
    jsonGuard->SetBool("falseValue", false);
    bool falseVal = jsonGuard->GetBool("falseValue");
    EXPECT_FALSE(falseVal);
}

// 测试特殊字符和Unicode
TEST_F(CppxJsonTest, TestSpecialCharactersAndUnicode)
{
    auto jsonGuard = IJson::CreateWithGuard();
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
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 测试最大和最小int32值
    jsonGuard->SetInt("maxInt", INT32_MAX);
    int32_t maxInt = jsonGuard->GetInt("maxInt");
    EXPECT_EQ(maxInt, INT32_MAX);
    
    jsonGuard->SetInt("minInt", INT32_MIN);
    int32_t minInt = jsonGuard->GetInt("minInt");
    EXPECT_EQ(minInt, INT32_MIN);
    
    // 测试零值
    jsonGuard->SetInt("zero", 0);
    int32_t zero = jsonGuard->GetInt("zero");
    EXPECT_EQ(zero, 0);
    
    // 测试负数
    jsonGuard->SetInt("negative", -12345);
    int32_t negative = jsonGuard->GetInt("negative");
    EXPECT_EQ(negative, -12345);
}

// 测试内存管理
TEST_F(CppxJsonTest, TestMemoryManagement)
{
    // 测试多次创建和销毁
    for (int i = 0; i < 100; ++i)
    {
        auto jsonGuard = IJson::CreateWithGuard();
        ASSERT_NE(jsonGuard.get(), nullptr);
        
        jsonGuard->SetString("test", "value");
        jsonGuard->SetInt("number", i);
        
        auto strGuard = jsonGuard->ToString();
        ASSERT_NE(strGuard.get(), nullptr);
        
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
                auto jsonGuard = IJson::CreateWithGuard();
                ASSERT_NE(jsonGuard.get(), nullptr);
                
                jsonGuard->SetString("thread_test", "value");
                jsonGuard->SetInt("thread_id", i);
                
                auto strGuard = jsonGuard->ToString();
                ASSERT_NE(strGuard.get(), nullptr);
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
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先设置一些有效数据
    jsonGuard->SetString("valid", "data");
    jsonGuard->SetInt("number", 42);
    
    // 尝试解析无效JSON（应该失败但不影响现有数据）
    const char* invalidJson = R"({"invalid": json)";
    int32_t result = jsonGuard->Parse(invalidJson);
    EXPECT_NE(result, 0);
    
    // 验证原有数据仍然存在
    const char* validData = jsonGuard->GetString("valid");
    EXPECT_STREQ(validData, "data");
    
    int32_t number = jsonGuard->GetInt("number");
    EXPECT_EQ(number, 42);
    
    // 现在解析有效JSON
    const char* validJson = R"({"new": "data", "value": 123})";
    result = jsonGuard->Parse(validJson);
    EXPECT_EQ(result, 0);
    
    // 验证新数据
    const char* newData = jsonGuard->GetString("new");
    EXPECT_STREQ(newData, "data");
    
    int32_t value = jsonGuard->GetInt("value");
    EXPECT_EQ(value, 123);
}

