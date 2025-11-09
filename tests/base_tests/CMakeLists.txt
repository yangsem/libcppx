cmake_minimum_required(VERSION 3.12)
project(base_tests LANGUAGES CXX)

# ==================== C++标准配置 ====================
# 默认使用C++17，允许通过CMAKE_CXX_STANDARD指定
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================== 构建类型配置 ====================
# 默认编译Debug版本，可以通过CMAKE_BUILD_TYPE指定Release版本
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ==================== 输出目录配置 ====================
# 统一将所有构建产物放置到build目录下
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ==================== 编译选项 ====================
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# ==================== GOOGLETEST 依赖配置 ====================
# 检查并自动clone googletest submodule
set(GOOGLETEST_DIR ${CMAKE_SOURCE_DIR}/../../deps/googletest)
set(GOOGLETEST_CMAKELISTS ${GOOGLETEST_DIR}/CMakeLists.txt)

# 检查googletest submodule是否存在
if(NOT EXISTS ${GOOGLETEST_CMAKELISTS})
    message(STATUS "googletest submodule not found, attempting to clone...")
    
    # 查找git命令
    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(FATAL_ERROR 
            "git not found! Please install git or manually clone the googletest submodule.\n"
            "Expected googletest at: ${GOOGLETEST_DIR}"
        )
    endif()
    
    # 获取项目根目录（libcppx目录）
    get_filename_component(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../.. ABSOLUTE)
    
    # 检查是否在git仓库中
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --git-dir
        WORKING_DIRECTORY ${PROJECT_ROOT}
        RESULT_VARIABLE GIT_REPO_CHECK
        OUTPUT_QUIET
        ERROR_QUIET
    )
    
    if(GIT_REPO_CHECK EQUAL 0)
        # 在git仓库中，尝试初始化并更新submodule
        message(STATUS "Initializing googletest submodule...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive deps/googletest
            WORKING_DIRECTORY ${PROJECT_ROOT}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
            OUTPUT_VARIABLE GIT_SUBMODULE_OUTPUT
            ERROR_VARIABLE GIT_SUBMODULE_ERROR
        )
        
        # 检查是否成功
        if(NOT EXISTS ${GOOGLETEST_CMAKELISTS})
            message(WARNING 
                "Failed to clone googletest submodule automatically.\n"
                "Git output: ${GIT_SUBMODULE_OUTPUT}\n"
                "Git error: ${GIT_SUBMODULE_ERROR}\n"
                "Please run manually: git submodule update --init --recursive deps/googletest"
            )
            message(FATAL_ERROR 
                "googletest submodule is required but not found at: ${GOOGLETEST_DIR}\n"
                "Please ensure the submodule is initialized before building."
            )
        else()
            message(STATUS "googletest submodule cloned successfully")
        endif()
    else()
        # 不在git仓库中，提示手动clone
        message(FATAL_ERROR 
            "Not in a git repository and googletest submodule not found.\n"
            "Please manually clone googletest to: ${GOOGLETEST_DIR}\n"
            "Or initialize git submodules if this is a git repository."
        )
    endif()
endif()

# 确保googletest构建静态库
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build googletest shared library" FORCE)
set(gtest_build_tests OFF CACHE BOOL "Build all of gtest's own tests" FORCE)
set(gtest_build_samples OFF CACHE BOOL "Build gtest's sample programs" FORCE)
set(gmock_build_tests OFF CACHE BOOL "Build all of gmock's own tests" FORCE)

# 设置googletest的输出目录到build目录下
set(GOOGLETEST_BINARY_DIR ${CMAKE_BINARY_DIR}/googletest)

# 添加googletest子目录（需要指定二进制目录，因为googletest不在base_tests目录下）
# googletest 内部的 CMAKE_BINARY_DIR 会是 GOOGLETEST_BINARY_DIR
add_subdirectory(${GOOGLETEST_DIR} ${GOOGLETEST_BINARY_DIR})

# 在add_subdirectory之后，显式设置googletest目标的输出目录
# 确保所有googletest的构建产物都在build/googletest目录下
if(TARGET gtest)
    set_target_properties(gtest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
    )
endif()

if(TARGET gtest_main)
    set_target_properties(gtest_main PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
    )
endif()

if(TARGET gmock)
    set_target_properties(gmock PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
    )
endif()

if(TARGET gmock_main)
    set_target_properties(gmock_main PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${GOOGLETEST_BINARY_DIR}/lib
    )
endif()

# ==================== BASE 库依赖配置 ====================
# 查找base库（静态库或动态库）
set(BASE_LIB_DIR ${CMAKE_SOURCE_DIR}/../../lib)
set(BASE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../base/include)

# 检查base库是否存在
if(EXISTS ${BASE_LIB_DIR}/libcppx_base.a)
    set(BASE_LIB_TYPE STATIC)
    set(BASE_LIB_PATH ${BASE_LIB_DIR}/libcppx_base.a)
elseif(EXISTS ${BASE_LIB_DIR}/libcppx_base.so)
    set(BASE_LIB_TYPE SHARED)
    set(BASE_LIB_PATH ${BASE_LIB_DIR}/libcppx_base.so)
else()
    message(WARNING "base library not found in ${BASE_LIB_DIR}, trying to find it...")
    # 尝试查找base库
    find_library(BASE_LIB_PATH
        NAMES cppx_base
        PATHS ${BASE_LIB_DIR}
        NO_DEFAULT_PATH
    )
    if(NOT BASE_LIB_PATH)
        message(FATAL_ERROR 
            "base library (libcppx_base) not found.\n"
            "Please build the base library first.\n"
            "Expected location: ${BASE_LIB_DIR}"
        )
    endif()
endif()

# ==================== 收集测试源文件 ====================
# 收集所有测试源文件（排除 build 目录和 CMake 生成的文件）
file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/*.cpp"
    "${CMAKE_SOURCE_DIR}/*/*.cpp"
    "${CMAKE_SOURCE_DIR}/*/*/*.cpp"
)

# 过滤掉 build 目录和 CMake 生成的文件
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*CMakeCXXCompilerId\\.cpp$")
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*CMakeCCompilerId\\.c$")

# 如果没有找到测试文件，给出警告
if(NOT TEST_SOURCES)
    message(WARNING "No test source files found in ${CMAKE_SOURCE_DIR}")
endif()

# ==================== 创建测试可执行文件 ====================
add_executable(base_tests ${TEST_SOURCES})
# 可执行文件会自动输出到 CMAKE_RUNTIME_OUTPUT_DIRECTORY（已设置为 build 目录）

# 设置包含目录
target_include_directories(base_tests
    PRIVATE
        ${BASE_INCLUDE_DIR}
        ${GOOGLETEST_DIR}/googletest/include
        ${GOOGLETEST_DIR}/googlemock/include
)

# 链接库
target_link_libraries(base_tests
    PRIVATE
        gtest
        gtest_main
        ${BASE_LIB_PATH}
)

# 如果是动态库，需要设置运行时库路径
if(BASE_LIB_TYPE STREQUAL "SHARED")
    set_target_properties(base_tests PROPERTIES
        INSTALL_RPATH "${BASE_LIB_DIR}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# ==================== 启用测试 ====================
enable_testing()

# 添加测试（可执行文件在build目录下）
add_test(NAME base_tests COMMAND ${CMAKE_BINARY_DIR}/base_tests)

