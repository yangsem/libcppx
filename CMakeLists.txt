cmake_minimum_required(VERSION 3.12)
project(libcppx LANGUAGES CXX)

# 设置C++标准（可配置，默认C++17）
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to use (11, 14, 17, 20, etc.)")
else()
    set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} CACHE STRING "C++ standard to use (11, 14, 17, 20, etc.)" FORCE)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

# 生成 compile_commands.json 用于IDE支持
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置构建类型（如果没有指定，默认为Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 输出目录设置（统一放在项目根目录下的.libcppx目录）
set(CPPX_OUTPUT_ROOT ${CMAKE_SOURCE_DIR}/.libcppx)
set(CPPX_LIB_DIR ${CPPX_OUTPUT_ROOT}/lib)
set(CPPX_BIN_DIR ${CPPX_OUTPUT_ROOT}/bin)
set(CPPX_TEST_DIR ${CPPX_OUTPUT_ROOT}/test)
set(CPPX_DEPS_DIR ${CPPX_OUTPUT_ROOT}/deps)

# 创建输出目录
file(MAKE_DIRECTORY ${CPPX_LIB_DIR})
file(MAKE_DIRECTORY ${CPPX_BIN_DIR})
file(MAKE_DIRECTORY ${CPPX_TEST_DIR})
file(MAKE_DIRECTORY ${CPPX_DEPS_DIR})

# 设置默认输出目录（项目库和可执行文件）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CPPX_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPPX_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CPPX_LIB_DIR})

message(STATUS "Output directories:")
message(STATUS "  lib:  ${CPPX_LIB_DIR}")
message(STATUS "  bin:  ${CPPX_BIN_DIR}")
message(STATUS "  test: ${CPPX_TEST_DIR}")
message(STATUS "  deps: ${CPPX_DEPS_DIR}")

# 编译选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# ==================== 组件定义 ====================
# 定义所有可用的组件及其依赖关系
# 格式：组件名:依赖组件列表（用分号分隔）
set(CPPX_COMPONENTS
    "base:"                    # base组件无依赖
    "network:base"             # network组件依赖base
    # 未来可以在这里添加新组件，例如：
    # "database:base"
    # "http:base;network"
)

# 解析组件列表
set(CPPX_COMPONENT_NAMES "")
set(CPPX_COMPONENT_DEPS "")
foreach(COMPONENT ${CPPX_COMPONENTS})
    string(REPLACE ":" ";" COMPONENT_PARTS ${COMPONENT})
    list(GET COMPONENT_PARTS 0 COMPONENT_NAME)
    list(APPEND CPPX_COMPONENT_NAMES ${COMPONENT_NAME})
    if(COMPONENT MATCHES ":")
        string(REGEX REPLACE "^[^:]+:" "" COMPONENT_DEPS ${COMPONENT})
        list(APPEND CPPX_COMPONENT_DEPS "${COMPONENT_NAME}:${COMPONENT_DEPS}")
    else()
        list(APPEND CPPX_COMPONENT_DEPS "${COMPONENT_NAME}:")
    endif()
endforeach()

message(STATUS "Available components: ${CPPX_COMPONENT_NAMES}")

# ==================== 智能组件选择逻辑 ====================
# 检查用户是否指定了要编译的组件
set(_USER_SPECIFIED_COMPONENTS FALSE)
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(DEFINED BUILD_${COMPONENT_UPPER})
        set(_USER_SPECIFIED_COMPONENTS TRUE)
        break()
    endif()
endforeach()

# 如果用户未指定任何组件，则编译所有组件
if(NOT _USER_SPECIFIED_COMPONENTS)
    message(STATUS "No component specified, building all components: ${CPPX_COMPONENT_NAMES}")
    foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
        string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
        set(BUILD_${COMPONENT_UPPER} ON CACHE BOOL "Build ${COMPONENT} library")
    endforeach()
else()
    # 用户指定了至少一个组件，只编译指定的组件
    message(STATUS "Building specified components only")
    foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
        string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
        if(NOT DEFINED BUILD_${COMPONENT_UPPER})
            set(BUILD_${COMPONENT_UPPER} OFF CACHE BOOL "Build ${COMPONENT} library")
        endif()
    endforeach()
    
    # 自动解析并添加依赖组件
    foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
        string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
        if(BUILD_${COMPONENT_UPPER})
            # 查找该组件的依赖
            foreach(COMPONENT_DEP ${CPPX_COMPONENT_DEPS})
                if(COMPONENT_DEP MATCHES "^${COMPONENT}:")
                    string(REGEX REPLACE "^${COMPONENT}:" "" DEPS_STR ${COMPONENT_DEP})
                    if(DEPS_STR)
                        string(REPLACE ";" " " DEPS_LIST ${DEPS_STR})
                        message(STATUS "${COMPONENT} depends on: ${DEPS_LIST}")
                        foreach(DEP ${DEPS_STR})
                            string(TOUPPER ${DEP} DEP_UPPER)
                            if(NOT BUILD_${DEP_UPPER})
                                set(BUILD_${DEP_UPPER} ON CACHE BOOL "Build ${DEP} library (required by ${COMPONENT})" FORCE)
                                message(STATUS "  -> Auto-enabling ${DEP} (dependency of ${COMPONENT})")
                            endif()
                        endforeach()
                    endif()
                endif()
            endforeach()
        endif()
    endforeach()
endif()

# 显示最终要编译的组件列表
set(_BUILD_LIST "")
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(BUILD_${COMPONENT_UPPER})
        list(APPEND _BUILD_LIST ${COMPONENT})
    endif()
endforeach()
message(STATUS "Components to build: ${_BUILD_LIST}")

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# 为每个组件创建测试选项
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    option(BUILD_${COMPONENT_UPPER}_TESTS "Build ${COMPONENT} unit tests" ${BUILD_TESTS})
endforeach()

# 设置库类型
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()

# ==================== 自动拉取依赖库 ====================
# 检查并自动拉取git submodule依赖
if(EXISTS ${CMAKE_SOURCE_DIR}/.gitmodules)
    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/jsoncpp OR NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/googletest)
        message(STATUS "Dependencies not found, initializing git submodules...")
        find_program(GIT_EXECUTABLE git)
        if(GIT_EXECUTABLE)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMODULE_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            if(GIT_SUBMODULE_RESULT EQUAL 0)
                message(STATUS "Git submodules initialized successfully")
            else()
                message(WARNING "Failed to initialize git submodules. Please run: git submodule update --init --recursive")
            endif()
        else()
            message(WARNING "Git not found. Please run: git submodule update --init --recursive")
        endif()
    endif()
endif()

# ==================== 管理依赖库 ====================
# 设置deps构建目录（CMake的add_subdirectory会自动处理重复编译问题）
set(DEPS_BUILD_DIR ${CMAKE_BINARY_DIR}/deps_build)

# ==================== JSONCPP ====================
if(EXISTS ${CMAKE_SOURCE_DIR}/deps/jsoncpp)
    # 使用add_subdirectory方式，CMake会自动处理：
    # 1. 如果已经编译过，不会重复编译
    # 2. 如果源文件未改变，会使用已编译的结果
    # 3. 如果源文件改变，会自动重新编译
    # 保存当前的BUILD_SHARED_LIBS值和输出目录
    set(SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(SAVED_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(SAVED_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    
    # 设置jsoncpp构建选项（在add_subdirectory之前设置）
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp shared library" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp static library" FORCE)
    
    # 设置jsoncpp的输出目录为.libcppx/deps（静态库）
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPPX_DEPS_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CPPX_DEPS_DIR})
    
    add_subdirectory(${CMAKE_SOURCE_DIR}/deps/jsoncpp ${DEPS_BUILD_DIR}/jsoncpp)
    
    # 恢复输出目录
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SAVED_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SAVED_ARCHIVE_OUTPUT_DIRECTORY})
    
    # 恢复BUILD_SHARED_LIBS的值（如果之前设置过）
    if(DEFINED SAVED_BUILD_SHARED_LIBS)
        set(BUILD_SHARED_LIBS ${SAVED_BUILD_SHARED_LIBS} CACHE BOOL "Build shared libraries" FORCE)
    endif()
    set(JSONCPP_FOUND TRUE)
    set(JSONCPP_LIBRARIES jsoncpp_static)
    set(JSONCPP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/deps/jsoncpp/include)
    message(STATUS "Using bundled jsoncpp from deps/jsoncpp")
else()
    message(FATAL_ERROR "jsoncpp not found. Please ensure deps/jsoncpp exists. Run: git submodule update --init --recursive")
endif()

# ==================== GoogleTest ====================
# 检查是否有任何组件需要测试
set(_NEED_GTEST FALSE)
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(BUILD_${COMPONENT_UPPER}_TESTS)
        set(_NEED_GTEST TRUE)
        break()
    endif()
endforeach()

if(BUILD_TESTS OR _NEED_GTEST)
    if(EXISTS ${CMAKE_SOURCE_DIR}/deps/googletest)
        # 保存当前的输出目录和BUILD_SHARED_LIBS
        set(SAVED_LIBRARY_OUTPUT_DIRECTORY_GTEST ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
        set(SAVED_ARCHIVE_OUTPUT_DIRECTORY_GTEST ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
        set(SAVED_BUILD_SHARED_LIBS_GTEST ${BUILD_SHARED_LIBS})
        
        # 设置googletest的输出目录为.libcppx/deps（静态库）
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPPX_DEPS_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CPPX_DEPS_DIR})
        # 确保googletest构建静态库（googletest默认就是静态库，但明确设置以确保）
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build googletest shared library" FORCE)
        
        # 使用add_subdirectory方式，CMake会自动处理重复编译问题
        # EXCLUDE_FROM_ALL确保googletest不会在all目标中构建，只在需要时构建
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/googletest ${DEPS_BUILD_DIR}/googletest EXCLUDE_FROM_ALL)
        
        # 恢复输出目录和BUILD_SHARED_LIBS
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SAVED_LIBRARY_OUTPUT_DIRECTORY_GTEST})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SAVED_ARCHIVE_OUTPUT_DIRECTORY_GTEST})
        # 恢复BUILD_SHARED_LIBS
        if(DEFINED SAVED_BUILD_SHARED_LIBS_GTEST)
            set(BUILD_SHARED_LIBS ${SAVED_BUILD_SHARED_LIBS_GTEST} CACHE BOOL "Build shared libraries" FORCE)
        endif()
        set(GTEST_FOUND TRUE)
        message(STATUS "Using bundled googletest from deps/googletest")
    else()
        message(WARNING "googletest not found. Unit tests will be disabled. Run: git submodule update --init --recursive")
        set(BUILD_TESTS OFF)
        foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
            string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
            set(BUILD_${COMPONENT_UPPER}_TESTS OFF)
        endforeach()
    endif()
endif()

# ==================== 添加子模块 ====================
# 动态添加所有启用的组件
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(BUILD_${COMPONENT_UPPER})
        if(EXISTS ${CMAKE_SOURCE_DIR}/${COMPONENT}/CMakeLists.txt)
            add_subdirectory(${COMPONENT})
            message(STATUS "Added component: ${COMPONENT}")
        else()
            message(WARNING "Component ${COMPONENT} is enabled but CMakeLists.txt not found at ${CMAKE_SOURCE_DIR}/${COMPONENT}/CMakeLists.txt")
        endif()
    endif()
endforeach()

# ==================== 安装配置 ====================
# 动态安装所有启用的组件
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(BUILD_${COMPONENT_UPPER})
        # 生成目标名称：cppx_<component>
        set(COMPONENT_TARGET_NAME "cppx_${COMPONENT}")
        
        if(TARGET ${COMPONENT_TARGET_NAME})
            install(TARGETS ${COMPONENT_TARGET_NAME}
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
                RUNTIME DESTINATION bin
            )
            
            # 安装头文件
            if(EXISTS ${CMAKE_SOURCE_DIR}/${COMPONENT}/include)
                install(DIRECTORY ${COMPONENT}/include/ DESTINATION include/cppx/${COMPONENT})
            endif()
        endif()
    endif()
endforeach()

# ==================== 测试 ====================
# 动态添加所有启用的组件测试
set(_HAS_TESTS FALSE)
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(BUILD_${COMPONENT_UPPER}_TESTS)
        set(_HAS_TESTS TRUE)
        break()
    endif()
endforeach()

if(BUILD_TESTS OR _HAS_TESTS)
    enable_testing()
    
    foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
        string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
        if(BUILD_${COMPONENT_UPPER}_TESTS)
            # 生成测试目标名称：cppx_<component>_unittest
            set(COMPONENT_TEST_TARGET "cppx_${COMPONENT}_unittest")
            
            if(TARGET ${COMPONENT_TEST_TARGET})
                add_test(NAME ${COMPONENT}_unittest COMMAND ${COMPONENT_TEST_TARGET})
            endif()
        endif()
    endforeach()
endif()

