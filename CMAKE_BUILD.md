# libcppx CMake 构建系统使用指南

## 概述

libcppx项目采用分层的CMake构建系统，支持：
- 独立编译base库和network库
- 独立编译各库的单元测试
- 整体编译、测试和安装
- 自动管理依赖库（deps），避免重复编译

## 目录结构

```
libcppx/
├── CMakeLists.txt          # 根目录CMakeLists，管理整体项目和deps
├── base/
│   ├── CMakeLists.txt      # base库的CMakeLists
│   ├── include/            # base库头文件
│   ├── sources/            # base库源文件
│   └── unittest/           # base库单元测试
├── network/
│   ├── CMakeLists.txt      # network库的CMakeLists
│   ├── include/            # network库头文件
│   ├── sources/            # network库源文件
│   └── unittest/           # network库单元测试
└── deps/                   # 依赖库
    ├── jsoncpp/            # jsoncpp依赖
    └── googletest/         # googletest依赖
```

## 构建选项

### 主要选项

- `BUILD_BASE`: 是否编译base库（默认：ON）
- `BUILD_NETWORK`: 是否编译network库（默认：ON）
- `BUILD_TESTS`: 是否编译所有测试（默认：ON）
- `BUILD_BASE_TESTS`: 是否编译base库测试（默认：跟随BUILD_TESTS）
- `BUILD_NETWORK_TESTS`: 是否编译network库测试（默认：跟随BUILD_TESTS）
- `BUILD_SHARED_LIBS`: 是否构建共享库（默认：ON）

### 测试相关选项说明

- `BUILD_TESTS=OFF`: 关闭所有组件的测试编译
- `BUILD_BASE_TESTS=OFF`: 只关闭base组件的测试编译
- `BUILD_NETWORK_TESTS=OFF`: 只关闭network组件的测试编译

**注意**：即使关闭了测试编译，如果组件需要googletest作为依赖，googletest仍会被编译。

## 使用场景

### 1. 整体编译（推荐）

在项目根目录执行：

```bash
mkdir build && cd build
cmake ..
make
```

这将编译：
- base库（动态库）
- network库（动态库）
- base库的单元测试
- network库的单元测试

### 2. 只编译base库

```bash
mkdir build && cd build
cmake .. -DBUILD_NETWORK=OFF
make
```

或者只编译base库，不编译测试：

```bash
mkdir build && cd build
cmake .. -DBUILD_NETWORK=OFF -DBUILD_BASE_TESTS=OFF
make
```

### 3. 只编译network库

```bash
mkdir build && cd build
cmake .. -DBUILD_BASE=OFF
make
```

注意：如果network库依赖base库，需要先编译base库或确保base库已安装。

### 4. 独立编译base库（在base目录）

```bash
cd base
mkdir build && cd build
cmake .. -DBUILD_BASE=ON
make
```

### 5. 独立编译base库的单元测试

```bash
cd base
mkdir build && cd build
cmake .. -DBUILD_BASE=ON -DBUILD_BASE_TESTS=ON
make
# 运行测试
./bin/cppx_base_unittest
```

### 6. 独立编译network库（在network目录）

```bash
cd network
mkdir build && cd build
cmake .. -DBUILD_NETWORK=ON
make
```

注意：如果network库依赖base库，需要先编译base库或确保base库已安装。

### 7. 独立编译network库的单元测试

```bash
cd network
mkdir build && cd build
cmake .. -DBUILD_NETWORK=ON -DBUILD_NETWORK_TESTS=ON
make
# 运行测试
./bin/cppx_network_unittest
```

### 8. 只编译测试并运行

```bash
mkdir build && cd build
cmake .. -DBUILD_BASE=OFF -DBUILD_NETWORK=OFF -DBUILD_BASE_TESTS=ON
make
ctest
```

### 9. 整体编译、测试和安装

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
ctest  # 运行所有测试
make install  # 安装库和头文件
```

安装后的目录结构：
```
/usr/local/
├── lib/
│   ├── libcppx_base.so
│   └── libcppx_network.so
└── include/
    └── cppx/
        ├── base/
        └── network/
```

## 依赖管理

### deps目录的依赖

项目会自动检测`deps`目录下的依赖库：
- `deps/jsoncpp`: jsoncpp库（base库需要）
- `deps/googletest`: GoogleTest框架（测试需要）

### 依赖编译机制

1. **首次编译**：如果deps目录存在，CMake会自动编译依赖库
2. **增量编译**：如果依赖库已编译且源文件未改变，不会重复编译
3. **自动更新**：如果依赖库源文件改变，CMake会自动重新编译

### 使用系统依赖

如果`deps`目录不存在，CMake会尝试查找系统的依赖包：
- jsoncpp: 通过`find_package(jsoncpp)`查找
- googletest: 通过`find_package(GTest)`查找

## 输出目录

编译后的文件输出位置：
- 库文件：`build/lib/`
- 可执行文件（测试）：`build/bin/`
- 依赖库：`build/deps_build/`

## 常见问题

### Q: 如何只编译静态库？

```bash
cmake .. -DBUILD_SHARED_LIBS=OFF
```

### Q: 如何禁用所有测试？

```bash
cmake .. -DBUILD_TESTS=OFF
```

### Q: 如何关闭编译测试用例？

有多种方式可以关闭测试用例的编译：

**方式1：关闭所有测试**
```bash
cmake -B build -DBUILD_TESTS=OFF
```

**方式2：关闭特定组件的测试**
```bash
# 关闭base组件的测试
cmake -B build -DBUILD_BASE_TESTS=OFF

# 关闭network组件的测试
cmake -B build -DBUILD_NETWORK_TESTS=OFF

# 同时关闭多个组件的测试
cmake -B build -DBUILD_BASE_TESTS=OFF -DBUILD_NETWORK_TESTS=OFF
```

**方式3：只编译库，不编译测试**
```bash
# 只编译base库，不编译测试
cmake -B build -DBUILD_BASE=ON -DBUILD_BASE_TESTS=OFF

# 只编译network库，不编译测试
cmake -B build -DBUILD_NETWORK=ON -DBUILD_NETWORK_TESTS=OFF
```

### Q: 如何执行测试用例？

有多种方式可以执行测试用例：

**方式1：使用ctest（推荐）**
```bash
# 在build目录下执行所有测试
cd build
ctest

# 显示详细输出
ctest --output-on-failure

# 显示更详细的输出
ctest -V

# 只运行特定组件的测试
ctest -R base_unittest
ctest -R network_unittest
```

**方式2：直接运行测试可执行文件**
```bash
# 运行base组件的测试
./build/test/cppx_base_unittest

# 运行network组件的测试（如果存在）
./build/test/cppx_network_unittest

# 使用详细输出模式
./build/test/cppx_base_unittest --gtest_color=yes
```

**方式3：使用cmake --build执行测试**
```bash
# 编译并运行测试
cmake --build build --target test

# 或者
cd build && make test
```

### Q: network库依赖base库吗？

如果network库依赖base库，需要确保：
1. 先编译base库，或
2. base库已安装到系统路径

### Q: 如何清理编译结果？

```bash
rm -rf build
```

### Q: 如何只重新编译某个库？

```bash
cd build
make cppx_base  # 只编译base库
make cppx_network  # 只编译network库
```

## 开发建议

1. **开发base库时**：在base目录下独立编译和测试
2. **开发network库时**：在network目录下独立编译和测试
3. **集成测试时**：在根目录整体编译和测试
4. **发布前**：确保整体编译、测试和安装都正常

## 示例命令总结

```bash
# 整体编译（包含测试）
cmake -B build && cmake --build build

# 只编译库，不编译测试
cmake -B build -DBUILD_TESTS=OFF && cmake --build build

# 只编译base库
cmake -B build -DBUILD_NETWORK=OFF && cmake --build build

# 只编译network库（会自动编译base）
cmake -B build -DBUILD_BASE=OFF && cmake --build build

# 编译并运行所有测试
cmake -B build && cmake --build build && cd build && ctest

# 编译并运行测试（显示详细输出）
cmake -B build && cmake --build build && cd build && ctest --output-on-failure

# 直接运行测试可执行文件
cmake -B build && cmake --build build && ./build/test/cppx_base_unittest

# 编译并安装
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local && cmake --build build && cmake --install build
```

## 测试用例管理

### 关闭测试用例编译

**关闭所有测试：**
```bash
cmake -B build -DBUILD_TESTS=OFF
```

**关闭特定组件测试：**
```bash
# 关闭base测试
cmake -B build -DBUILD_BASE_TESTS=OFF

# 关闭network测试
cmake -B build -DBUILD_NETWORK_TESTS=OFF
```

### 执行测试用例

**使用ctest（推荐）：**
```bash
cd build
ctest                    # 运行所有测试
ctest -V                 # 详细输出
ctest --output-on-failure  # 失败时显示输出
ctest -R base_unittest # 只运行base测试
```

**直接运行可执行文件：**
```bash
./build/test/cppx_base_unittest
./build/test/cppx_base_unittest --gtest_color=yes  # 彩色输出
```

**使用CMake测试目标：**
```bash
cmake --build build --target test
```

