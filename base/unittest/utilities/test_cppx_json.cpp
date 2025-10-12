#include <gtest/gtest.h>
#include <utilities/cppx_json.h>
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
    auto arrayJsonGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(arrayJsonGuard.get(), nullptr);
    arrayJsonGuard->AppendBool(true);
    arrayJsonGuard->AppendInt(1);
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
    auto jsonGuard = IJson::CreateWithGuard();
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
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);

    // 设置一些数据
    jsonGuard->SetString("name", "测试");
    jsonGuard->SetInt("age", 25);
    jsonGuard->SetBool("active", true);

    // 测试清空前，类型为object
    EXPECT_EQ(jsonGuard->GetType(), IJson::JsonType::kJsonTypeObject);
    
    // 测试Clear
    jsonGuard->Clear();
    // 测试获取key为name的值
    const char* name = jsonGuard->GetString("name");
    EXPECT_EQ(name, nullptr);
    // 测试获取key为age的值
    int32_t age = jsonGuard->GetInt("age");
    EXPECT_EQ(age, 0);
    // 测试获取key为active的值
    bool active = jsonGuard->GetBool("active");
    EXPECT_EQ(active, false);

    EXPECT_EQ(jsonGuard->GetType(), IJson::JsonType::kJsonTypeObject);
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

// 测试数组索引访问接口
TEST_F(CppxJsonTest, TestArrayIndexAccess)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析包含数组的测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 获取hobbies数组
    auto hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard.get(), nullptr);
    
    // 测试GetString通过索引访问
    const char* hobby1 = hobbiesGuard->GetString(0);
    ASSERT_NE(hobby1, nullptr);
    EXPECT_STREQ(hobby1, "读书");
    
    const char* hobby2 = hobbiesGuard->GetString(1);
    ASSERT_NE(hobby2, nullptr);
    EXPECT_STREQ(hobby2, "游泳");
    
    const char* hobby3 = hobbiesGuard->GetString(2);
    ASSERT_NE(hobby3, nullptr);
    EXPECT_STREQ(hobby3, "编程");
    
    // 测试GetString通过索引访问with default
    const char* defaultHobby = hobbiesGuard->GetString(10, "默认爱好");
    EXPECT_STREQ(defaultHobby, "默认爱好");
    
    // 获取scores数组
    auto scoresGuard = jsonGuard->GetArray("scores");
    ASSERT_NE(scoresGuard.get(), nullptr);
    
    // 测试GetInt通过索引访问
    int32_t score1 = scoresGuard->GetInt(0);
    EXPECT_EQ(score1, 95);
    
    int32_t score2 = scoresGuard->GetInt(1);
    EXPECT_EQ(score2, 87);
    
    int32_t score3 = scoresGuard->GetInt(2);
    EXPECT_EQ(score3, 92);
    
    // 测试GetInt通过索引访问with default
    int32_t defaultScore = scoresGuard->GetInt(10, 999);
    EXPECT_EQ(defaultScore, 999);
    
    // 测试数组越界访问
    const char* outOfBoundsStr = hobbiesGuard->GetString(-1);
    EXPECT_EQ(outOfBoundsStr, nullptr);
    
    int32_t outOfBoundsInt = scoresGuard->GetInt(-1);
    EXPECT_EQ(outOfBoundsInt, 0);
    
    // 测试GetType通过索引访问
    IJson::JsonType strType = hobbiesGuard->GetType(0);
    EXPECT_EQ(strType, IJson::JsonType::kJsonTypeString);
    
    IJson::JsonType intType = scoresGuard->GetType(0);
    EXPECT_EQ(intType, IJson::JsonType::kJsonTypeNumber);
}

// 测试数组追加接口
TEST_F(CppxJsonTest, TestArrayAppendOperations)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个数组类型的JSON对象
    auto arrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试AppendString
    int32_t result = arrayGuard->AppendString("第一个字符串");
    EXPECT_EQ(result, 0);
    
    result = arrayGuard->AppendString("第二个字符串");
    EXPECT_EQ(result, 0);
    
    // 验证追加的字符串
    const char* str1 = arrayGuard->GetString(0);
    ASSERT_NE(str1, nullptr);
    EXPECT_STREQ(str1, "第一个字符串");
    
    const char* str2 = arrayGuard->GetString(1);
    ASSERT_NE(str2, nullptr);
    EXPECT_STREQ(str2, "第二个字符串");
    
    // 测试AppendInt
    result = arrayGuard->AppendInt(100);
    EXPECT_EQ(result, 0);
    
    result = arrayGuard->AppendInt(200);
    EXPECT_EQ(result, 0);
    
    // 验证追加的整数
    int32_t int1 = arrayGuard->GetInt(2);
    EXPECT_EQ(int1, 100);
    
    int32_t int2 = arrayGuard->GetInt(3);
    EXPECT_EQ(int2, 200);
    
    // 测试AppendBool
    result = arrayGuard->AppendBool(true);
    EXPECT_EQ(result, 0);
    
    result = arrayGuard->AppendBool(false);
    EXPECT_EQ(result, 0);
    
    // 验证追加的布尔值
    bool bool1 = arrayGuard->GetBool(4);
    EXPECT_TRUE(bool1);
    
    bool bool2 = arrayGuard->GetBool(5);
    EXPECT_FALSE(bool2);
    
    // 测试AppendObject
    auto subObjGuard = IJson::CreateWithGuard();
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "子对象");
    subObjGuard->SetInt("value", 42);
    
    result = arrayGuard->AppendObject(subObjGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证追加的对象
    auto retrievedObj = arrayGuard->GetObject(6);
    ASSERT_NE(retrievedObj.get(), nullptr);
    const char* objName = retrievedObj->GetString("name");
    ASSERT_NE(objName, nullptr);
    EXPECT_STREQ(objName, "子对象");
    
    int32_t objValue = retrievedObj->GetInt("value");
    EXPECT_EQ(objValue, 42);
    
    // 测试AppendArray
    auto subArrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("数组元素1");
    subArrayGuard->AppendInt(123);
    
    result = arrayGuard->AppendArray(subArrayGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证追加的数组
    auto retrievedArray = arrayGuard->GetArray(7);
    ASSERT_NE(retrievedArray.get(), nullptr);
    const char* arrayStr = retrievedArray->GetString(0);
    ASSERT_NE(arrayStr, nullptr);
    EXPECT_STREQ(arrayStr, "数组元素1");
    
    int32_t arrayInt = retrievedArray->GetInt(1);
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
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 先解析测试数据
    int32_t result = jsonGuard->ParseFile("test_data.json");
    ASSERT_EQ(result, 0);
    
    // 测试对象的大小（应该包含所有键值对）
    uint32_t objectSize = jsonGuard->GetSize();
    EXPECT_GT(objectSize, 0);
    
    // 获取hobbies数组并测试其大小
    auto hobbiesGuard = jsonGuard->GetArray("hobbies");
    ASSERT_NE(hobbiesGuard.get(), nullptr);
    uint32_t hobbiesSize = hobbiesGuard->GetSize();
    EXPECT_EQ(hobbiesSize, 3); // 根据测试数据，hobbies数组有3个元素
    
    // 获取scores数组并测试其大小
    auto scoresGuard = jsonGuard->GetArray("scores");
    ASSERT_NE(scoresGuard.get(), nullptr);
    uint32_t scoresSize = scoresGuard->GetSize();
    EXPECT_EQ(scoresSize, 3); // 根据测试数据，scores数组有3个元素
    
    // 测试空数组的大小
    auto emptyArrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(emptyArrayGuard.get(), nullptr);
    uint32_t emptyArraySize = emptyArrayGuard->GetSize();
    EXPECT_EQ(emptyArraySize, 0);
    
    // 测试空对象的大小
    auto emptyObjGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeObject);
    ASSERT_NE(emptyObjGuard.get(), nullptr);
    uint32_t emptyObjSize = emptyObjGuard->GetSize();
    EXPECT_EQ(emptyObjSize, 0);
    
    // 测试基本类型的大小（应该返回0）
    auto stringGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeString);
    ASSERT_NE(stringGuard.get(), nullptr);
    stringGuard->SetString("test", "value");
    uint32_t stringSize = stringGuard->GetSize();
    EXPECT_EQ(stringSize, 0);
    
    auto intGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeNumber);
    ASSERT_NE(intGuard.get(), nullptr);
    intGuard->SetInt("test", 42);
    uint32_t intSize = intGuard->GetSize();
    EXPECT_EQ(intSize, 0);
    
    auto boolGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeBoolean);
    ASSERT_NE(boolGuard.get(), nullptr);
    boolGuard->SetBool("test", true);
    uint32_t boolSize = boolGuard->GetSize();
    EXPECT_EQ(boolSize, 0);
}

// 测试数组索引访问的GetType接口
TEST_F(CppxJsonTest, TestGetTypeByIndex)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个包含不同类型元素的数组
    auto arrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 添加不同类型的元素
    arrayGuard->AppendString("字符串元素");
    arrayGuard->AppendInt(42);
    arrayGuard->AppendBool(true);
    
    // 添加一个对象
    auto subObjGuard = IJson::CreateWithGuard();
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "子对象");
    arrayGuard->AppendObject(subObjGuard.get());
    
    // 添加一个数组
    auto subArrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("数组元素");
    arrayGuard->AppendArray(subArrayGuard.get());
    
    // 测试通过索引获取类型
    IJson::JsonType type0 = arrayGuard->GetType(0);
    EXPECT_EQ(type0, IJson::JsonType::kJsonTypeString);
    
    IJson::JsonType type1 = arrayGuard->GetType(1);
    EXPECT_EQ(type1, IJson::JsonType::kJsonTypeNumber);
    
    IJson::JsonType type2 = arrayGuard->GetType(2);
    EXPECT_EQ(type2, IJson::JsonType::kJsonTypeBoolean);
    
    IJson::JsonType type3 = arrayGuard->GetType(3);
    EXPECT_EQ(type3, IJson::JsonType::kJsonTypeObject);
    
    IJson::JsonType type4 = arrayGuard->GetType(4);
    EXPECT_EQ(type4, IJson::JsonType::kJsonTypeArray);
    
    // 测试越界访问
    IJson::JsonType outOfBoundsType = arrayGuard->GetType(10);
    EXPECT_EQ(outOfBoundsType, IJson::JsonType::kJsonTypeNull);
    
    // 测试负数索引
    IJson::JsonType negativeType = arrayGuard->GetType(-1);
    EXPECT_EQ(negativeType, IJson::JsonType::kJsonTypeNull);
}

// 测试数组操作的边界情况
TEST_F(CppxJsonTest, TestArrayBoundaryConditions)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个数组
    auto arrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 测试在空数组上访问元素
    const char* emptyStr = arrayGuard->GetString(0);
    EXPECT_EQ(emptyStr, nullptr);
    
    int32_t emptyInt = arrayGuard->GetInt(0);
    EXPECT_EQ(emptyInt, 0);
    
    bool emptyBool = arrayGuard->GetBool(0);
    EXPECT_FALSE(emptyBool);
    
    auto emptyObj = arrayGuard->GetObject(0);
    EXPECT_EQ(emptyObj.get(), nullptr);
    
    auto emptyArray = arrayGuard->GetArray(0);
    EXPECT_EQ(emptyArray.get(), nullptr);
    
    // 添加一些元素
    arrayGuard->AppendString("test");
    arrayGuard->AppendInt(123);
    arrayGuard->AppendBool(false);
    
    // 测试获取数组大小
    uint32_t size = arrayGuard->GetSize();
    EXPECT_EQ(size, 3);
    
    // 测试访问各个元素
    const char* firstStr = arrayGuard->GetString(0);
    ASSERT_NE(firstStr, nullptr);
    EXPECT_STREQ(firstStr, "test");
    
    int32_t secondInt = arrayGuard->GetInt(1);
    EXPECT_EQ(secondInt, 123);
    
    bool thirdBool = arrayGuard->GetBool(2);
    EXPECT_FALSE(thirdBool);
    
    // 测试访问超出范围的元素
    const char* outOfRangeStr = arrayGuard->GetString(10, "默认值");
    EXPECT_STREQ(outOfRangeStr, "默认值");
    
    int32_t outOfRangeInt = arrayGuard->GetInt(10, 999);
    EXPECT_EQ(outOfRangeInt, 999);
    
    bool outOfRangeBool = arrayGuard->GetBool(10, true);
    EXPECT_TRUE(outOfRangeBool);
}

// 测试数组和对象的混合操作
TEST_F(CppxJsonTest, TestArrayObjectMixedOperations)
{
    auto jsonGuard = IJson::CreateWithGuard();
    ASSERT_NE(jsonGuard.get(), nullptr);
    
    // 创建一个包含数组的对象
    auto arrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(arrayGuard.get(), nullptr);
    
    // 在数组中添加不同类型的元素
    arrayGuard->AppendString("字符串");
    arrayGuard->AppendInt(100);
    arrayGuard->AppendBool(true);
    
    // 创建一个子对象并添加到数组
    auto subObjGuard = IJson::CreateWithGuard();
    ASSERT_NE(subObjGuard.get(), nullptr);
    subObjGuard->SetString("name", "数组中的对象");
    subObjGuard->SetInt("id", 1);
    arrayGuard->AppendObject(subObjGuard.get());
    
    // 创建一个子数组并添加到数组
    auto subArrayGuard = IJson::CreateWithGuard(IJson::JsonType::kJsonTypeArray);
    ASSERT_NE(subArrayGuard.get(), nullptr);
    subArrayGuard->AppendString("子数组元素1");
    subArrayGuard->AppendString("子数组元素2");
    arrayGuard->AppendArray(subArrayGuard.get());
    
    // 将数组设置到主对象
    int32_t result = jsonGuard->SetArray("mixedArray", arrayGuard.get());
    EXPECT_EQ(result, 0);
    
    // 验证混合数组的内容
    auto retrievedArray = jsonGuard->GetArray("mixedArray");
    ASSERT_NE(retrievedArray.get(), nullptr);
    
    // 验证数组大小
    uint32_t arraySize = retrievedArray->GetSize();
    EXPECT_EQ(arraySize, 5);
    
    // 验证字符串元素
    const char* str = retrievedArray->GetString(0);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "字符串");
    
    // 验证整数元素
    int32_t intVal = retrievedArray->GetInt(1);
    EXPECT_EQ(intVal, 100);
    
    // 验证布尔元素
    bool boolVal = retrievedArray->GetBool(2);
    EXPECT_TRUE(boolVal);
    
    // 验证对象元素
    auto obj = retrievedArray->GetObject(3);
    ASSERT_NE(obj.get(), nullptr);
    const char* objName = obj->GetString("name");
    ASSERT_NE(objName, nullptr);
    EXPECT_STREQ(objName, "数组中的对象");
    
    int32_t objId = obj->GetInt("id");
    EXPECT_EQ(objId, 1);
    
    // 验证数组元素
    auto arr = retrievedArray->GetArray(4);
    ASSERT_NE(arr.get(), nullptr);
    uint32_t subArraySize = arr->GetSize();
    EXPECT_EQ(subArraySize, 2);
    
    const char* subStr1 = arr->GetString(0);
    ASSERT_NE(subStr1, nullptr);
    EXPECT_STREQ(subStr1, "子数组元素1");
    
    const char* subStr2 = arr->GetString(1);
    ASSERT_NE(subStr2, nullptr);
    EXPECT_STREQ(subStr2, "子数组元素2");
}

