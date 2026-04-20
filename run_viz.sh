#!/bin/bash
# 启动 AIS 可视化程序（兼容 WSL2 软件渲染模式）
cd "$(dirname "$0")"

export QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu --no-sandbox --disable-dev-shm-usage"
export LIBGL_ALWAYS_SOFTWARE=1

exec ./build-pc/visualization/ais_visualizer "$@"
