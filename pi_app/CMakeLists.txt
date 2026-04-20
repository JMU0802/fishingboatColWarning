cmake_minimum_required(VERSION 3.16)

# --------------------------------------------------------------------------
# 树莓派无头应用 —— 纯计算，无 UI 依赖
# 可交叉编译到 AArch64
# --------------------------------------------------------------------------
add_executable(ais_pi_app
    main.cpp
)

target_link_libraries(ais_pi_app PRIVATE
    ais_core
)

# Release 模式下用 strip 缩小二进制体积
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CMAKE_STRIP)
    add_custom_command(TARGET ais_pi_app POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:ais_pi_app>
        COMMENT "Strip binary for deployment"
    )
endif()
