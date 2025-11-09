# ==================== Base Library ====================
# 支持独立编译：如果从base目录直接运行cmake，需要设置基本配置

# 如果LIBRARY_TYPE未定义（独立编译），则设置为SHARED
if(NOT DEFINED LIBRARY_TYPE)
    set(LIBRARY_TYPE SHARED)
    message(STATUS "LIBRARY_TYPE not defined, defaulting to SHARED (standalone build)")
endif()

# 如果CMAKE_CXX_STANDARD未设置，设置默认值
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "CMAKE_CXX_STANDARD not defined, defaulting to C++17 (standalone build)")
endif()

# 收集源文件
file(GLOB_RECURSE BASE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cc"
)

# 创建base库（目标名称：cppx_base）
add_library(cppx_base ${LIBRARY_TYPE} ${BASE_SOURCES})

# 设置目标属性
target_include_directories(cppx_base
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/cppx/base>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/sources
)

# 链接jsoncpp（只使用deps目录下的jsoncpp）
if(TARGET jsoncpp_static)
    # 使用add_subdirectory方式添加的jsoncpp
    target_link_libraries(cppx_base PUBLIC jsoncpp_static)
elseif(JSONCPP_FOUND)
    # 如果jsoncpp是通过顶层CMakeLists.txt设置的
    target_link_libraries(cppx_base PUBLIC ${JSONCPP_LIBRARIES})
    if(JSONCPP_INCLUDE_DIRS)
        target_include_directories(cppx_base PUBLIC ${JSONCPP_INCLUDE_DIRS})
    endif()
else()
    message(FATAL_ERROR "jsoncpp not found. Please ensure deps/jsoncpp exists. Run: git submodule update --init --recursive")
endif()

# 设置编译选项（使用全局C++标准设置）
# 如果CMAKE_CXX_STANDARD未设置，则使用C++17作为默认值
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set_property(TARGET cppx_base PROPERTY CXX_STANDARD 17)
else()
    set_property(TARGET cppx_base PROPERTY CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif()
set_property(TARGET cppx_base PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET cppx_base PROPERTY CXX_EXTENSIONS OFF)

# 设置输出名称
set_target_properties(cppx_base PROPERTIES
    OUTPUT_NAME cppx_base
    VERSION 1.0.0
    SOVERSION 1
)

# ==================== Base Unit Tests ====================
# 如果BUILD_BASE_TESTS未定义（独立编译），默认编译测试
if(NOT DEFINED BUILD_BASE_TESTS)
    set(BUILD_BASE_TESTS ON)
    message(STATUS "BUILD_BASE_TESTS not defined, defaulting to ON (standalone build)")
endif()

if(BUILD_BASE_TESTS)
    # 收集测试源文件
    file(GLOB_RECURSE BASE_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/unittest/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/unittest/*.cc"
    )
    
    # 排除unittest_main.cpp（如果单独存在）
    list(FILTER BASE_TEST_SOURCES EXCLUDE REGEX ".*unittest_main\\.cpp$")
    
    # 创建测试可执行文件
    add_executable(cppx_base_unittest
        ${BASE_TEST_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/unittest/unittest_main.cpp
    )
    
    # 链接base库和gtest
    target_link_libraries(cppx_base_unittest
        PRIVATE
            cppx_base
    )
    
    # 链接GoogleTest（只使用deps目录下的googletest）
    if(TARGET gtest AND TARGET gtest_main)
        # 使用add_subdirectory方式添加的googletest
        target_link_libraries(cppx_base_unittest PRIVATE gtest gtest_main)
    elseif(GTEST_FOUND)
        # 如果googletest是通过顶层CMakeLists.txt设置的
        target_link_libraries(cppx_base_unittest PRIVATE ${GTEST_LIBRARIES})
        if(GTEST_INCLUDE_DIRS)
            target_include_directories(cppx_base_unittest PRIVATE ${GTEST_INCLUDE_DIRS})
        endif()
    else()
        message(FATAL_ERROR "googletest not found. Please ensure deps/googletest exists. Run: git submodule update --init --recursive")
    endif()
    
    # 设置测试包含目录
    target_include_directories(cppx_base_unittest
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/sources
    )
    
    # 设置输出目录（使用全局定义的测试输出目录）
    # 如果CPPX_TEST_DIR已定义（从顶层CMakeLists.txt），则使用它
    # 顶层构建时：build/test（使用cmake -B build）或 .libcppx/test（就地构建）
    # 独立编译时：使用默认的.libcppx/test
    if(DEFINED CPPX_TEST_DIR)
        set_target_properties(cppx_base_unittest PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CPPX_TEST_DIR}
        )
    else()
        set_target_properties(cppx_base_unittest PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/.libcppx/test
        )
    endif()
endif()

