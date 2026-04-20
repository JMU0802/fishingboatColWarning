# AIS 碰撞预警系统 (FishingBoatColWarning)

## 项目简介

基于 AIS（船舶自动识别系统）数据的碰撞预警系统，支持：
- **本机可视化**：Qt5 + Leaflet.js 地图实时展示 AIS 目标
- **树莓派部署**：纯计算无 UI，运行 AIS 解码 + CPA/TCPA 碰撞避让算法

---

## 项目结构

```
fishingboatColWarning/
├── CMakeLists.txt            # 根 CMake，统一管理所有子模块
├── Makefile                  # 快捷命令封装
├── cmake/
│   └── aarch64-linux-gnu.cmake   # 树莓派 ARM64 交叉编译工具链
│
├── core/                     # 共享核心库 (ais_core)，与平台无关
│   ├── ais/
│   │   ├── myaisdecoder.h    # AIS 解码器接口 & 数据结构
│   │   └── myaisdecoder.cpp  # AIS 解码器实现
│   └── collision/
│       ├── colwarning.h      # CPA/TCPA 碰撞避让接口
│       └── colwarning.cpp    # CPA/TCPA 碰撞避让实现
│
├── visualization/            # PC 端可视化应用 (Qt5，仅本机编译)
│   ├── main.cpp
│   ├── mainwindow.h/.cpp     # 主窗口（地图 + 列表）
│   ├── aislistmodel.h/.cpp   # Qt 模型（AIS 目标列表）
│   ├── aisreader.h/.cpp      # AIS 数据源（文件/串口）
│   └── resources/
│       └── map.html          # Leaflet.js 地图页面
│
├── pi_app/                   # 树莓派端无头应用 (可交叉编译)
│   └── main.cpp
│
├── tests/                    # 单元测试 (doctest，仅本机编译)
│   ├── test_ais_decoder.cpp
│   └── test_colwarning.cpp
│
└── data/
    └── AISMSG.txt            # AIS 报文测试数据
```

---

## 模块职责说明

| 模块 | 平台 | 职责 |
|------|------|------|
| `core/ais_core` | 全平台（静态库） | AIS 解码 + 碰撞算法，零 UI 依赖 |
| `visualization` | WSL2/PC（本机） | Qt5 地图可视化，链接 ais_core |
| `pi_app` | 树莓派 ARM64 | 无头运行，链接 ais_core |
| `tests` | WSL2/PC（本机） | 单元测试 |

---

## 构建说明

### 前置依赖（WSL2 Ubuntu）

```bash
# C++ 工具链
sudo apt install build-essential cmake

# 交叉编译工具链（树莓派 ARM64）
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Qt5（本机可视化）
sudo apt install qtbase5-dev qtwebengine5-dev
```

### 快捷命令

```bash
make pc          # 本机构建：可视化 + 单元测试
make pi          # 交叉编译：树莓派 ARM64
make deploy      # 编译 + scp 发送到树莓派
make run-viz     # 启动可视化界面
make run-pi      # SSH 远程启动树莓派程序
make test        # 运行单元测试
make clean       # 清理构建产物
```

### 自定义树莓派地址

```bash
make deploy PI_USER=pi PI_IP=192.168.1.100
```

### 手动 CMake 构建

```bash
# 本机（可视化）
cmake -B build-pc -DCMAKE_BUILD_TYPE=Debug .
cmake --build build-pc --parallel

# 交叉编译（树莓派）
cmake -B build-pi \
      -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_VISUALIZATION=OFF \
      .
cmake --build build-pi --parallel
```

---

## 碰撞避让算法

使用**向量法（相对运动线 RML）**计算 CPA/TCPA：

- **CPA**（Closest Point of Approach）：最近会遇距离（海里）
- **TCPA**（Time to CPA）：到达最近会遇点的时间（分钟）
- 告警条件：`CPA < 0.5 NM` 且 `0 < TCPA ≤ 6 min`

阈值可通过 `ColWarning::SetCpaLimit()` / `SetTcpaLimit()` 调整。

---

## 可视化界面

- **地图**：Leaflet.js（OpenStreetMap 底图），AIS 目标以三角形图标显示，箭头方向为 COG
- **列表**：Qt QTableView，显示 MMSI / 船名 / 船型 / SOG / COG / 经纬度 / 航行状态
- **交互**：点击列表中的行，地图自动定位到该目标
- **数据源**：默认读取 `data/AISMSG.txt`；可在 `AisReader::Config` 中切换到串口


# 安装依赖
sudo apt install build-essential cmake gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qtbase5-dev qtwebengine5-dev

make pc          # 编译本机可视化 + 单元测试
make test        # 跑单元测试
make run-viz     # 启动 AIS 地图可视化界面

make pi          # 交叉编译树莓派版本
make deploy PI_IP=192.168.1.xxx   # 编译并部署到树莓派