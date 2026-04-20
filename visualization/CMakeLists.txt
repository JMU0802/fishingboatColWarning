cmake_minimum_required(VERSION 3.16)

# --------------------------------------------------------------------------
# PC 可视化应用 —— 仅在本机（非交叉编译）构建时启用
# 依赖：Qt5 Widgets + WebEngineWidgets
#
# 安装依赖（Ubuntu/WSL2）:
#   sudo apt install qtbase5-dev qtwebengine5-dev
# --------------------------------------------------------------------------

find_package(Qt5 REQUIRED COMPONENTS Core Widgets WebEngineWidgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(ais_visualizer
    main.cpp
    mainwindow.cpp
    aislistmodel.cpp
    aisreader.cpp
)

target_include_directories(ais_visualizer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ais_visualizer PRIVATE
    ais_core
    Qt5::Core
    Qt5::Widgets
    Qt5::WebEngineWidgets
)

# 将地图 HTML 资源复制到构建目录
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/map.html
    ${CMAKE_CURRENT_BINARY_DIR}/resources/map.html
    COPYONLY
)
