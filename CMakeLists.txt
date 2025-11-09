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

# 输出目录设置
# deps的编译产物使用其默认路径（不修改）
# 自有代码的编译产物放在build下（当使用cmake -B build时）
# 如果CMAKE_BINARY_DIR不是源码目录（即使用了-B参数），则使用CMAKE_BINARY_DIR
# 否则使用.libcppx（就地构建的情况）
if(NOT CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    # 使用了-B参数，输出到build目录
    set(CPPX_OUTPUT_ROOT ${CMAKE_BINARY_DIR})
else()
    # 就地构建，输出到.libcppx
    set(CPPX_OUTPUT_ROOT ${CMAKE_SOURCE_DIR}/.libcppx)
endif()
set(CPPX_LIB_DIR ${CPPX_OUTPUT_ROOT}/lib)
set(CPPX_BIN_DIR ${CPPX_OUTPUT_ROOT}/bin)
set(CPPX_TEST_DIR ${CPPX_OUTPUT_ROOT}/test)

# 创建输出目录
file(MAKE_DIRECTORY ${CPPX_LIB_DIR})
file(MAKE_DIRECTORY ${CPPX_BIN_DIR})
file(MAKE_DIRECTORY ${CPPX_TEST_DIR})

# 设置默认输出目录（项目库和可执行文件）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CPPX_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPPX_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CPPX_LIB_DIR})

message(STATUS "Output directories:")
message(STATUS "  lib:  ${CPPX_LIB_DIR}")
message(STATUS "  bin:  ${CPPX_BIN_DIR}")
message(STATUS "  test: ${CPPX_TEST_DIR}")
message(STATUS "  deps: using default paths (not modified)")

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

# 检查是否需要编译jsoncpp（任何组件都需要base，base需要jsoncpp）
set(_NEED_JSONCPP FALSE)
foreach(COMPONENT ${CPPX_COMPONENT_NAMES})
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)
    if(BUILD_${COMPONENT_UPPER})
        set(_NEED_JSONCPP TRUE)
        break()
    endif()
endforeach()

# ==================== JSONCPP ====================
if(_NEED_JSONCPP AND EXISTS ${CMAKE_SOURCE_DIR}/deps/jsoncpp)
    message(STATUS "jsoncpp library will be compiled (using default output path)...")
    
    # 保存当前的BUILD_SHARED_LIBS值和编译选项
    set(SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(SAVED_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE})
    set(SAVED_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    
    # 设置jsoncpp构建选项（在add_subdirectory之前设置）
    # 禁用测试用例
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile jsoncpp test executables" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Run jsoncpp unit-tests as post build step" FORCE)
    set(JSONCPP_WITH_EXAMPLE OFF CACHE BOOL "Compile jsoncpp example" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp shared library" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "Build jsoncpp static library" FORCE)
    
    # 启用位置无关代码（-fPIC），因为静态库需要链接到共享库
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable position independent code" FORCE)
    
    # 设置jsoncpp使用与主项目相同的C++标准（支持默认C++17或命令行传入的版本）
    set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} CACHE STRING "C++ standard for jsoncpp" FORCE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard for jsoncpp" FORCE)
    set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "Disable C++ extensions for jsoncpp" FORCE)
    message(STATUS "jsoncpp will use C++ standard: ${CMAKE_CXX_STANDARD}")
    
    # 使用add_subdirectory方式，deps使用其默认输出路径
    # 编译deps库前先clean（清理构建目录）
    if(EXISTS ${DEPS_BUILD_DIR}/jsoncpp)
        message(STATUS "Cleaning jsoncpp build directory...")
        file(REMOVE_RECURSE ${DEPS_BUILD_DIR}/jsoncpp)
    endif()
    
    add_subdirectory(${CMAKE_SOURCE_DIR}/deps/jsoncpp ${DEPS_BUILD_DIR}/jsoncpp)
    
    # 强制设置jsoncpp目标的C++标准（jsoncpp的CMakeLists.txt会设置C++11，需要覆盖）
    if(TARGET jsoncpp_static)
        set_target_properties(jsoncpp_static PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )
        message(STATUS "jsoncpp_static target set to use C++ standard: ${CMAKE_CXX_STANDARD}")
    endif()
    if(TARGET jsoncpp_object)
        set_target_properties(jsoncpp_object PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )
    endif()
    
    # 恢复BUILD_SHARED_LIBS的值（如果之前设置过）
    if(DEFINED SAVED_BUILD_SHARED_LIBS)
        set(BUILD_SHARED_LIBS ${SAVED_BUILD_SHARED_LIBS} CACHE BOOL "Build shared libraries" FORCE)
    endif()
    
    # 恢复CMAKE_POSITION_INDEPENDENT_CODE的值（如果之前设置过）
    if(DEFINED SAVED_POSITION_INDEPENDENT_CODE)
        set(CMAKE_POSITION_INDEPENDENT_CODE ${SAVED_POSITION_INDEPENDENT_CODE} CACHE BOOL "Enable position independent code" FORCE)
    endif()
    
    # 恢复CMAKE_CXX_STANDARD的值（如果之前设置过）
    if(DEFINED SAVED_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD ${SAVED_CXX_STANDARD} CACHE STRING "C++ standard to use" FORCE)
    endif()
    
    set(JSONCPP_FOUND TRUE)
    set(JSONCPP_LIBRARIES jsoncpp_static)
    set(JSONCPP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/deps/jsoncpp/include)
    message(STATUS "Using bundled jsoncpp from deps/jsoncpp")
elseif(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/jsoncpp)
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
        message(STATUS "googletest libraries will be compiled (using default output path)...")
        
        # 保存当前的BUILD_SHARED_LIBS值和编译选项
        set(SAVED_BUILD_SHARED_LIBS_GTEST ${BUILD_SHARED_LIBS})
        set(SAVED_POSITION_INDEPENDENT_CODE_GTEST ${CMAKE_POSITION_INDEPENDENT_CODE})
        set(SAVED_CXX_STANDARD_GTEST ${CMAKE_CXX_STANDARD})
        
        # 确保googletest构建静态库（googletest默认就是静态库，但明确设置以确保）
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build googletest shared library" FORCE)
        # 禁用googletest的测试（googletest默认不编译测试，但明确设置以确保）
        set(gtest_build_tests OFF CACHE BOOL "Build googletest tests" FORCE)
        set(gtest_build_samples OFF CACHE BOOL "Build googletest samples" FORCE)
        # 启用位置无关代码（-fPIC），因为静态库可能被链接到共享库
        set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable position independent code" FORCE)
        
        # 设置googletest使用与主项目相同的C++标准（支持默认C++17或命令行传入的版本）
        set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} CACHE STRING "C++ standard for googletest" FORCE)
        set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard for googletest" FORCE)
        set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "Disable C++ extensions for googletest" FORCE)
        message(STATUS "googletest will use C++ standard: ${CMAKE_CXX_STANDARD}")
        
        # 使用add_subdirectory方式，deps使用其默认输出路径
        # 编译deps库前先clean（清理构建目录）
        if(EXISTS ${DEPS_BUILD_DIR}/googletest)
            message(STATUS "Cleaning googletest build directory...")
            file(REMOVE_RECURSE ${DEPS_BUILD_DIR}/googletest)
        endif()
        
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/googletest ${DEPS_BUILD_DIR}/googletest)
        
        # 强制设置googletest目标的C++标准（确保与主项目一致）
        if(TARGET gtest)
            set_target_properties(gtest PROPERTIES
                CXX_STANDARD ${CMAKE_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
            )
        endif()
        if(TARGET gtest_main)
            set_target_properties(gtest_main PROPERTIES
                CXX_STANDARD ${CMAKE_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
            )
        endif()
        if(TARGET gmock)
            set_target_properties(gmock PROPERTIES
                CXX_STANDARD ${CMAKE_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
            )
        endif()
        if(TARGET gmock_main)
            set_target_properties(gmock_main PROPERTIES
                CXX_STANDARD ${CMAKE_CXX_STANDARD}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
            )
        endif()
        
        # 恢复BUILD_SHARED_LIBS
        if(DEFINED SAVED_BUILD_SHARED_LIBS_GTEST)
            set(BUILD_SHARED_LIBS ${SAVED_BUILD_SHARED_LIBS_GTEST} CACHE BOOL "Build shared libraries" FORCE)
        endif()
        # 恢复CMAKE_POSITION_INDEPENDENT_CODE
        if(DEFINED SAVED_POSITION_INDEPENDENT_CODE_GTEST)
            set(CMAKE_POSITION_INDEPENDENT_CODE ${SAVED_POSITION_INDEPENDENT_CODE_GTEST} CACHE BOOL "Enable position independent code" FORCE)
        endif()
        # 恢复CMAKE_CXX_STANDARD
        if(DEFINED SAVED_CXX_STANDARD_GTEST)
            set(CMAKE_CXX_STANDARD ${SAVED_CXX_STANDARD_GTEST} CACHE STRING "C++ standard to use" FORCE)
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

