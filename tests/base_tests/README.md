# Base Tests 使用说明

## 前置条件

1. **确保 base 库已编译**
   ```bash
   cd /home/yangsem/repositories/libcppx/base
   mkdir build && cd build
   cmake ..
   make
   ```
   编译后会在 `libcppx/lib/` 目录下生成 `libcppx_base.a` 或 `libcppx_base.so`

2. **确保 googletest submodule 已初始化**（CMakeLists.txt 会自动处理）

## 编译测试

### 方法一：在 base_tests 目录下构建（推荐）

```bash
# 进入测试目录
cd /home/yangsem/repositories/libcppx/tests/base_tests

# 创建构建目录
mkdir build && cd build

# 配置（默认Debug版本，C++17）
cmake ..

# 编译
make

# 可执行文件在 tests/base_tests/ 目录下
# 运行测试
../base_tests
```

### 方法二：指定构建类型

```bash
cd /home/yangsem/repositories/libcppx/tests/base_tests
mkdir build && cd build

# 编译Release版本
cmake -DCMAKE_BUILD_TYPE=Release ..
make
../base_tests
```

### 方法三：指定C++标准

```bash
cd /home/yangsem/repositories/libcppx/tests/base_tests
mkdir build && cd build

# 使用C++20标准
cmake -DCMAKE_CXX_STANDARD=20 ..
make
../base_tests
```

### 方法四：组合配置

```bash
cd /home/yangsem/repositories/libcppx/tests/base_tests
mkdir build && cd build

# Release版本 + C++20
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 ..
make
../base_tests
```

## 运行测试

### 直接运行可执行文件

```bash
# 在 tests/base_tests/ 目录下
./base_tests

# 或者从构建目录运行
cd build
../base_tests
```

### 使用 CTest 运行

```bash
cd build
ctest

# 显示详细输出
ctest --verbose

# 运行特定测试
ctest -R base_tests
```

### 使用 Google Test 参数

```bash
# 列出所有测试
./base_tests --gtest_list_tests

# 运行特定测试
./base_tests --gtest_filter=CppxJsonTest.*

# 运行特定测试用例
./base_tests --gtest_filter=CppxJsonTest.TestParseString

# 显示详细输出
./base_tests --gtest_color=yes

# 重复运行测试
./base_tests --gtest_repeat=10
```

## 清理构建

```bash
# 删除构建目录
rm -rf build

# 或者只清理编译产物
cd build
make clean
```

## 常见问题

### 1. base 库未找到

**错误信息：**
```
base library (libcppx_base) not found.
```

**解决方法：**
```bash
# 先编译 base 库
cd /home/yangsem/repositories/libcppx/base
mkdir build && cd build
cmake ..
make
```

### 2. googletest submodule 未初始化

**错误信息：**
```
googletest submodule is required but not found
```

**解决方法：**
CMakeLists.txt 会自动尝试 clone，如果失败，手动执行：
```bash
cd /home/yangsem/repositories/libcppx
git submodule update --init --recursive deps/googletest
```

### 3. 找不到头文件

**错误信息：**
```
fatal error: utilities/json.h: No such file or directory
```

**解决方法：**
确保 base 库已编译，并且 `libcppx/base/include` 目录存在。

### 4. 链接错误

**错误信息：**
```
undefined reference to `cppx::base::...`
```

**解决方法：**
- 确保 base 库已编译
- 检查库文件是否在 `libcppx/lib/` 目录下
- 如果是动态库，确保运行时能找到库文件（设置 LD_LIBRARY_PATH）

## 目录结构

```
libcppx/
├── base/              # base 库源码
│   ├── include/       # 头文件
│   └── sources/       # 源文件
├── lib/               # 编译后的库文件
│   ├── libcppx_base.a
│   └── libcppx_base.so
├── deps/
│   ├── jsoncpp/       # jsoncpp 依赖
│   └── googletest/    # googletest 依赖
└── tests/
    └── base_tests/    # 测试目录
        ├── CMakeLists.txt
        ├── unittest_main.cpp
        ├── utilities/
        │   ├── test_json.cpp
        │   └── test_time.cpp
        └── thread/
            └── test_task_scheduler.cpp
```

## 快速开始脚本

创建一个 `build.sh` 脚本：

```bash
#!/bin/bash
set -e

# 进入测试目录
cd "$(dirname "$0")"

# 创建构建目录
mkdir -p build
cd build

# 配置
cmake .. "$@"

# 编译
make -j$(nproc)

# 运行测试
echo "Running tests..."
../base_tests
```

使用方法：
```bash
chmod +x build.sh
./build.sh                    # 默认配置
./build.sh -DCMAKE_BUILD_TYPE=Release  # Release版本
```

