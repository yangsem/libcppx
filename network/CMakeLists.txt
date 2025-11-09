# ==================== Network Library ====================
# 支持独立编译：如果从network目录直接运行cmake，需要设置基本配置

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
file(GLOB_RECURSE NETWORK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cc"
)

# 创建network库（目标名称：cppx_network）
add_library(cppx_network ${LIBRARY_TYPE} ${NETWORK_SOURCES})

# 设置目标属性
target_include_directories(cppx_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/cppx/network>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/sources
)

# network组件明确依赖base组件
# 检查是否在顶层构建（BUILD_BASE已定义）或独立构建
if(DEFINED BUILD_BASE AND BUILD_BASE AND TARGET cppx_base)
    # 在顶层构建中，直接链接base目标
    target_link_libraries(cppx_network PUBLIC cppx_base)
    message(STATUS "network: linked to base (from top-level build)")
elseif(TARGET cppx_base)
    # base目标存在（可能是独立编译的）
    target_link_libraries(cppx_network PUBLIC cppx_base)
    message(STATUS "network: linked to base (standalone base found)")
else()
    # 如果base未编译，尝试查找项目根目录下的base库
    # 只查找项目目录，不在系统路径查找
    # 支持新的输出目录结构：build/lib 或 .libcppx/lib
    set(CPPX_BASE_LIB_SEARCH_PATHS
        ${CMAKE_SOURCE_DIR}/.libcppx/lib
        ${CMAKE_BINARY_DIR}/lib
        ${CMAKE_BINARY_DIR}/../.libcppx/lib
        ${CMAKE_SOURCE_DIR}/../.libcppx/lib
    )
    # 如果CPPX_LIB_DIR已定义（从顶层CMakeLists.txt），也添加到搜索路径
    if(DEFINED CPPX_LIB_DIR)
        list(INSERT CPPX_BASE_LIB_SEARCH_PATHS 0 ${CPPX_LIB_DIR})
    endif()
    
    find_library(CPPX_BASE_LIB 
        NAMES cppx_base libcppx_base
        PATHS ${CPPX_BASE_LIB_SEARCH_PATHS}
        NO_DEFAULT_PATH
    )
    
    if(CPPX_BASE_LIB)
        target_link_libraries(cppx_network PUBLIC ${CPPX_BASE_LIB})
        # 查找base的头文件（只在项目目录查找）
        find_path(CPPX_BASE_INCLUDE_DIR
            NAMES cppx/base/logger/logger.h
            PATHS ${CMAKE_SOURCE_DIR}/base/include
                  ${CMAKE_SOURCE_DIR}/../base/include
            NO_DEFAULT_PATH
        )
        if(CPPX_BASE_INCLUDE_DIR)
            target_include_directories(cppx_network PUBLIC ${CPPX_BASE_INCLUDE_DIR})
        endif()
        message(STATUS "network: linked to base (found in project lib directory)")
    else()
        message(FATAL_ERROR "network requires base library. Please build base first. Expected location: ${CMAKE_SOURCE_DIR}/lib")
    endif()
endif()

# 设置编译选项（使用全局C++标准设置）
# 如果CMAKE_CXX_STANDARD未设置，则使用C++17作为默认值
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set_property(TARGET cppx_network PROPERTY CXX_STANDARD 17)
else()
    set_property(TARGET cppx_network PROPERTY CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif()
set_property(TARGET cppx_network PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET cppx_network PROPERTY CXX_EXTENSIONS OFF)

# 设置输出名称
set_target_properties(cppx_network PROPERTIES
    OUTPUT_NAME cppx_network
    VERSION 1.0.0
    SOVERSION 1
)

# ==================== Network Unit Tests ====================
# 如果BUILD_NETWORK_TESTS未定义（独立编译），默认编译测试
if(NOT DEFINED BUILD_NETWORK_TESTS)
    set(BUILD_NETWORK_TESTS ON)
    message(STATUS "BUILD_NETWORK_TESTS not defined, defaulting to ON (standalone build)")
endif()

if(BUILD_NETWORK_TESTS)
    # 收集测试源文件
    file(GLOB_RECURSE NETWORK_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/unittest/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/unittest/*.cc"
    )
    
    if(NETWORK_TEST_SOURCES)
        # 排除unittest_main.cpp（如果单独存在）
        list(FILTER NETWORK_TEST_SOURCES EXCLUDE REGEX ".*unittest_main\\.cpp$")
        
        # 检查是否有unittest_main.cpp
        set(NETWORK_UNITTEST_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/unittest/unittest_main.cpp")
        if(NOT EXISTS ${NETWORK_UNITTEST_MAIN})
            # 如果没有unittest_main.cpp，创建一个简单的
            message(STATUS "Creating unittest_main.cpp for network tests")
            file(WRITE ${NETWORK_UNITTEST_MAIN}
                "#include <gtest/gtest.h>\n\n"
                "int main(int argc, char **argv)\n"
                "{\n"
                "    testing::InitGoogleTest(&argc, argv);\n"
                "    return RUN_ALL_TESTS();\n"
                "}\n"
            )
        endif()
        
        # 创建测试可执行文件
        add_executable(cppx_network_unittest
            ${NETWORK_TEST_SOURCES}
            ${NETWORK_UNITTEST_MAIN}
        )
        
        # 链接network库和base库
        target_link_libraries(cppx_network_unittest
            PRIVATE
                cppx_network
        )
        
        # 如果base库存在，也链接它
        if(DEFINED BUILD_BASE AND BUILD_BASE AND TARGET cppx_base)
            target_link_libraries(cppx_network_unittest PRIVATE cppx_base)
        endif()
        
        # 链接GoogleTest（只使用deps目录下的googletest）
        if(TARGET gtest AND TARGET gtest_main)
            # 使用add_subdirectory方式添加的googletest
            target_link_libraries(cppx_network_unittest PRIVATE gtest gtest_main)
        elseif(GTEST_FOUND)
            # 如果googletest是通过顶层CMakeLists.txt设置的
            target_link_libraries(cppx_network_unittest PRIVATE ${GTEST_LIBRARIES})
            if(GTEST_INCLUDE_DIRS)
                target_include_directories(cppx_network_unittest PRIVATE ${GTEST_INCLUDE_DIRS})
            endif()
        else()
            message(FATAL_ERROR "googletest not found. Please ensure deps/googletest exists. Run: git submodule update --init --recursive")
        endif()
        
        # 设置测试包含目录
        target_include_directories(cppx_network_unittest
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/sources
        )
        
        # 如果base存在，添加base的include目录
        if(DEFINED BUILD_BASE AND BUILD_BASE)
            target_include_directories(cppx_network_unittest PRIVATE
                ${CMAKE_SOURCE_DIR}/base/include
            )
        endif()
        
        # 设置输出目录（使用全局定义的测试输出目录）
        # 如果CPPX_TEST_DIR已定义（从顶层CMakeLists.txt），则使用它
        # 否则使用默认的.libcppx/test（独立编译的情况）
        if(DEFINED CPPX_TEST_DIR)
            set_target_properties(cppx_network_unittest PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CPPX_TEST_DIR}
            )
        else()
            set_target_properties(cppx_network_unittest PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/.libcppx/test
            )
        endif()
    else()
        message(STATUS "No network test sources found, skipping network unittest")
    endif()
endif()

