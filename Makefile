# ===========================================================================
# 顶层 Makefile —— 快捷命令封装，实际构建由 CMake 驱动
# ===========================================================================

PI_USER  ?= pi
PI_IP    ?= 192.168.1.100
PI_DIR   ?= /home/$(PI_USER)/ais_col_warning

BUILD_PC    := build-pc
BUILD_PI    := build-pi
BINARY_PI   := $(BUILD_PI)/pi_app/ais_pi_app
BINARY_VIZ  := $(BUILD_PC)/visualization/ais_visualizer

TOOLCHAIN   := cmake/aarch64-linux-gnu.cmake

.PHONY: all pc pi deploy clean help

# --------------------------------------------------------------------------
all: pc pi

# --------------------------------------------------------------------------
# 本机构建（可视化 + 测试）
# --------------------------------------------------------------------------
pc:
	@mkdir -p $(BUILD_PC)
	cmake -B $(BUILD_PC) \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DBUILD_VISUALIZATION=ON \
	      -DBUILD_PI_APP=OFF \
	      -DBUILD_TESTS=ON \
	      .
	cmake --build $(BUILD_PC) --parallel

# --------------------------------------------------------------------------
# 树莓派 ARM64 交叉编译
# --------------------------------------------------------------------------
pi:
	@mkdir -p $(BUILD_PI)
	cmake -B $(BUILD_PI) \
	      -DCMAKE_BUILD_TYPE=Release \
	      -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
	      -DBUILD_VISUALIZATION=OFF \
	      -DBUILD_PI_APP=ON \
	      -DBUILD_TESTS=OFF \
	      .
	cmake --build $(BUILD_PI) --parallel

# --------------------------------------------------------------------------
# 部署：交叉编译 → scp 传输 → (可选) 远程启动
# --------------------------------------------------------------------------
deploy: pi
	@echo ">>> 传输二进制到 $(PI_USER)@$(PI_IP):$(PI_DIR)"
	ssh $(PI_USER)@$(PI_IP) "mkdir -p $(PI_DIR)"
	scp $(BINARY_PI) $(PI_USER)@$(PI_IP):$(PI_DIR)/
	scp data/AISMSG.txt $(PI_USER)@$(PI_IP):$(PI_DIR)/
	@echo ">>> 部署完成。通过 'make run-pi' 在树莓派上启动程序。"

# 远程运行（可选）
run-pi:
	ssh $(PI_USER)@$(PI_IP) "cd $(PI_DIR) && ./ais_pi_app < AISMSG.txt"

# --------------------------------------------------------------------------
# 本机可视化（WSL2 兼容：强制软件渲染）
# --------------------------------------------------------------------------
run-viz: pc
	QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu --no-sandbox --disable-dev-shm-usage" \
	LIBGL_ALWAYS_SOFTWARE=1 \
	$(BINARY_VIZ)

# --------------------------------------------------------------------------
# 测试
# --------------------------------------------------------------------------
test: pc
	cd $(BUILD_PC) && ctest --output-on-failure

# --------------------------------------------------------------------------
clean:
	rm -rf $(BUILD_PC) $(BUILD_PI)

# --------------------------------------------------------------------------
help:
	@echo ""
	@echo "  make pc          — 本机构建（可视化 + 单元测试）"
	@echo "  make pi          — 交叉编译 ARM64（树莓派）"
	@echo "  make deploy      — 编译并通过 scp 发送到树莓派"
	@echo "  make run-pi      — SSH 远程启动树莓派程序"
	@echo "  make run-viz     — 本机启动可视化界面"
	@echo "  make test        — 运行单元测试"
	@echo "  make clean       — 删除所有构建产物"
	@echo ""
	@echo "  配置变量（可在命令行覆盖）:"
	@echo "    PI_USER=$(PI_USER)  PI_IP=$(PI_IP)  PI_DIR=$(PI_DIR)"
	@echo ""
