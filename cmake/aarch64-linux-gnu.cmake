# ===========================================================================
# Raspberry Pi 4 / Pi 5  —— AArch64 (ARM64) 交叉编译工具链
# 使用方式:
#   cmake -B build-pi -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake \
#         -DBUILD_VISUALIZATION=OFF ..
# ===========================================================================

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 工具链前缀（Ubuntu/Debian: sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu）
set(TOOLCHAIN_PREFIX aarch64-linux-gnu)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_STRIP        ${TOOLCHAIN_PREFIX}-strip)

# 让 CMake 在目标 sysroot 而非宿主机目录中查找库和头文件
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 生成适用于 Raspberry Pi OS (Bookworm/Bullseye) 的二进制文件
set(CMAKE_C_FLAGS_INIT   "-march=armv8-a")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a")
