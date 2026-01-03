#!/bin/bash
# Wokwi 环境切换脚本
# 用法: ./switch-wokwi.sh test-battery

if [ -z "$1" ]; then
    echo "用法: ./switch-wokwi.sh <环境名>"
    echo "可用环境: test-battery, test-lsm6ds3, test-ov2640, test-ec800k, test-gps, test-audio, test-psram"
    exit 1
fi

ENV_NAME=$1
BUILD_DIR=".pio/build/$ENV_NAME"

if [ ! -d "$BUILD_DIR" ]; then
    echo "错误: 环境 $ENV_NAME 不存在或未构建"
    echo "请先运行: pio run -e $ENV_NAME"
    exit 1
fi

# 更新 wokwi.toml
cat > wokwi.toml << EOF
[wokwi]
version = 1
firmware = "$BUILD_DIR/firmware.bin"
elf = "$BUILD_DIR/firmware.elf"
gdbServerPort = 3333
EOF

echo "✅ Wokwi 已切换到环境: $ENV_NAME"
echo "📁 固件路径: $BUILD_DIR/firmware.bin"
