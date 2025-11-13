cmake_minimum_required(VERSION 3.12)
project(base LANGUAGES CXX)

# ==================== C++标准配置 ====================
# 默认使用C++17，允许通过CMAKE_CXX_STANDARD指定
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================== 构建类型配置 ====================
# 默认编译Release版本，可以通过CMAKE_BUILD_TYPE指定Debug版本
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ==================== 代码覆盖率配置 ====================
# 启用代码覆盖率选项（通过 -DENABLE_COVERAGE=ON 启用）
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")
    # 覆盖率需要Debug构建类型以获得更好的结果
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Coverage is typically used with Debug build type. Current build type: ${CMAKE_BUILD_TYPE}")
    endif()
    
    # 为GCC和Clang设置覆盖率编译选项
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC使用 --coverage (等同于 -fprofile-arcs -ftest-coverage)
        add_compile_options(--coverage)
        add_link_options(--coverage)
        message(STATUS "Using GCC coverage flags: --coverage")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang使用 --coverage (等同于 -fprofile-instr-generate -fcoverage-mapping)
        add_compile_options(--coverage)
        add_link_options(--coverage)
        message(STATUS "Using Clang coverage flags: --coverage")
    else()
        message(WARNING "Coverage may not be supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# ==================== 输出目录配置 ====================
# 编译后的动态库放置到base/../lib中
set(LIB_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUTPUT_DIR})

# ==================== 编译选项 ====================
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# ==================== JSONCPP 依赖配置 ====================
# 检查并自动clone jsoncpp submodule
set(JSONCPP_DIR ${CMAKE_SOURCE_DIR}/../deps/jsoncpp)
set(JSONCPP_CMAKELISTS ${JSONCPP_DIR}/CMakeLists.txt)

# 检查jsoncpp submodule是否存在（通过检查CMakeLists.txt文件）
if(NOT EXISTS ${JSONCPP_CMAKELISTS})
    message(STATUS "jsoncpp submodule not found, attempting to clone...")
    
    # 查找git命令
    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(FATAL_ERROR 
            "git not found! Please install git or manually clone the jsoncpp submodule.\n"
            "Expected jsoncpp at: ${JSONCPP_DIR}"
        )
    endif()
    
    # 获取项目根目录（libcppx目录）
    get_filename_component(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)
    
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
        message(STATUS "Initializing jsoncpp submodule...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive deps/jsoncpp
            WORKING_DIRECTORY ${PROJECT_ROOT}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
            OUTPUT_VARIABLE GIT_SUBMODULE_OUTPUT
            ERROR_VARIABLE GIT_SUBMODULE_ERROR
        )
        
        # 检查是否成功
        if(NOT EXISTS ${JSONCPP_CMAKELISTS})
            message(WARNING 
                "Failed to clone jsoncpp submodule automatically.\n"
                "Git output: ${GIT_SUBMODULE_OUTPUT}\n"
                "Git error: ${GIT_SUBMODULE_ERROR}\n"
                "Please run manually: git submodule update --init --recursive deps/jsoncpp"
            )
            message(FATAL_ERROR 
                "jsoncpp submodule is required but not found at: ${JSONCPP_DIR}\n"
                "Please ensure the submodule is initialized before building."
            )
        else()
            message(STATUS "jsoncpp submodule cloned successfully")
        endif()
    else()
        # 不在git仓库中，提示手动clone
        message(FATAL_ERROR 
            "Not in a git repository and jsoncpp submodule not found.\n"
            "Please manually clone jsoncpp to: ${JSONCPP_DIR}\n"
            "Or initialize git submodules if this is a git repository."
        )
    endif()
endif()

# 确保jsoncpp构建静态库和对象库（对象库用于合并到静态库中）
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp static library")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp shared library")
set(BUILD_OBJECT_LIBS ON CACHE BOOL "Build jsoncpp object library" FORCE)

# 禁用jsoncpp测试用例编译
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile JsonCpp test executables" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests" FORCE)

# 设置jsoncpp使用-fPIC编译（因为静态库要链接到动态库）
# 需要在add_subdirectory之前设置，以确保jsoncpp编译时使用-fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 添加jsoncpp子目录（需要指定二进制目录，因为jsoncpp不在base目录下）
set(JSONCPP_BINARY_DIR ${CMAKE_BINARY_DIR}/jsoncpp)
add_subdirectory(${JSONCPP_DIR} ${JSONCPP_BINARY_DIR})

# 确保jsoncpp静态库使用-fPIC编译，并使用和base一样的C++标准
if(TARGET jsoncpp_static)
    set_target_properties(jsoncpp_static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
    )
endif()

# ==================== 收集源文件 ====================
# 收集所有源文件（排除 build 目录和 CMake 生成的文件）
file(GLOB_RECURSE BASE_SOURCES
    "${CMAKE_SOURCE_DIR}/sources/*.cpp"
    "${CMAKE_SOURCE_DIR}/sources/*.cc"
)

# 过滤掉 build 目录和 CMake 生成的文件
list(FILTER BASE_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER BASE_SOURCES EXCLUDE REGEX ".*CMakeCXXCompilerId\\.cpp$")
list(FILTER BASE_SOURCES EXCLUDE REGEX ".*CMakeCCompilerId\\.c$")

# ==================== 创建动态库 ====================
add_library(base_shared SHARED ${BASE_SOURCES})

# 设置动态库的输出名称和版本
set_target_properties(base_shared PROPERTIES
    OUTPUT_NAME "cppx_base"
    VERSION 1.0.0
    SOVERSION 1
)

# 设置包含目录
target_include_directories(base_shared
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../deps/jsoncpp/include
)

# 链接jsoncpp静态库到动态库中（静态库会被包含到动态库中）
# 使用 PUBLIC 确保 jsoncpp 的符号被包含在动态库中
target_link_libraries(base_shared PUBLIC jsoncpp_static)

# 设置动态库的输出路径（不包括依赖的jsoncpp）
set_target_properties(base_shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIB_OUTPUT_DIR}
)

# ==================== 创建静态库 ====================
add_library(base_static STATIC ${BASE_SOURCES})

# 设置静态库的输出名称
set_target_properties(base_static PROPERTIES
    OUTPUT_NAME "cppx_base"
)

# 设置包含目录
target_include_directories(base_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../deps/jsoncpp/include
)

# 链接jsoncpp到静态库中
# 优先使用对象库（如果存在），这样可以将jsoncpp的代码直接包含到静态库中
# 使用 PUBLIC 确保依赖信息（如包含目录）也会传递给依赖 base_static 的目标
if(TARGET jsoncpp_object)
    # 使用对象库，jsoncpp的代码会被直接包含到base_static中
    target_link_libraries(base_static PUBLIC jsoncpp_object)
    # 确保对象库使用正确的C++标准
    set_target_properties(jsoncpp_object PROPERTIES
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )
else()
    # 如果没有对象库，使用静态库（代码不会合并，但依赖关系会记录）
    target_link_libraries(base_static PUBLIC jsoncpp_static)
endif()

# 设置静态库的输出路径（不包括依赖的jsoncpp）
set_target_properties(base_static PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUTPUT_DIR}
)

# ==================== 安装配置 ====================
# 设置默认安装路径（可通过CMAKE_INSTALL_PREFIX指定）
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../install CACHE PATH "Install path prefix" FORCE)
endif()

# 安装库文件
install(TARGETS base_shared base_static
    EXPORT baseTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# 安装CMake配置文件（可选，用于其他项目使用）
install(EXPORT baseTargets
    FILE baseTargets.cmake
    NAMESPACE base::
    DESTINATION lib/cmake/base
)

