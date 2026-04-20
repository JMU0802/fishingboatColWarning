cmake_minimum_required(VERSION 3.16)

# --------------------------------------------------------------------------
# ais_core 静态库
# 包含：AIS 解码 + 碰撞避让算法
# 对 visualization 和 pi_app 均可链接
# --------------------------------------------------------------------------
add_library(ais_core STATIC
    ais/myaisdecoder.cpp
    collision/colwarning.cpp
)

target_include_directories(ais_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(ais_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
