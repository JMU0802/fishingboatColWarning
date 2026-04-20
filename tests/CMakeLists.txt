cmake_minimum_required(VERSION 3.16)

# --------------------------------------------------------------------------
# 单元测试 —— 使用轻量的 doctest（单头文件，无需安装）
# --------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(test_ais
    test_ais_decoder.cpp
    test_colwarning.cpp
)

target_link_libraries(test_ais PRIVATE
    ais_core
    doctest::doctest
)

include(doctest)
doctest_discover_tests(test_ais)
