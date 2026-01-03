# PlatformIO 多环境测试指南

## 可用测试环境

### 1. 主程序环境
```bash
# 完整的智能灯杆监控系统
pio run --environment esp32-s3-pole-monitor --target upload
pio device monitor --environment esp32-s3-pole-monitor
```

### 2. PSRAM 测试环境

#### 快速验证（推荐首次使用）
```bash
pio run -e test-psram-simple -t upload
pio device monitor -e test-psram-simple
```
**功能：** 验证 2MB PSRAM 是否正常工作

**预期输出：**
```
Total PSRAM: 2097152 bytes (2.00 MB)
SUCCESS: PSRAM is enabled and working!
```

#### 完整测试
```bash
pio run -e test-psram-full -t upload
pio device monitor -e test-psram-full
```
**功能：** 4个详细测试用例，包含大块内存分配和相机缓冲区模拟

### 3. 相机模块测试
```bash
pio run -e test-camera -t upload
pio device monitor -e test-camera
```
**功能：** 测试 OV2640 相机初始化、拍照、JPEG 数据验证

**注意：** 需要先创建 `test/test_camera.cpp`

### 4. Mock 模式测试（Wokwi 仿真）
```bash
pio run -e test-mock -t upload
pio device monitor -e test-mock
```
**功能：** 不需要真实硬件，使用 Mock 模拟所有传感器

### 5. 4G 模块测试
```bash
pio run -e test-4g-modem -t upload
pio device monitor -e test-4g-modem
```
**功能：** 测试 EC800K 模块 AT 指令、网络注册、HTTP 通信

### 6. GPS 模块测试
```bash
pio run -e test-gps -t upload
pio device monitor -e test-gps
```
**功能：** 测试 ATGM336H GPS 模块数据解析

### 7. 传感器测试
```bash
pio run -e test-sensors -t upload
pio device monitor -e test-sensors
```
**功能：** 测试 LSM6DS3 倾角传感器和麦克风传感器

## 快速命令参考

| 命令简写 | 完整命令 | 说明 |
|---------|---------|------|
| `pio run -e <env>` | `pio run --environment <env>` | 编译指定环境 |
| `pio run -e <env> -t upload` | `pio run --environment <env> --target upload` | 编译并上传 |
| `pio device monitor -e <env>` | `pio device monitor --environment <env>` | 打开串口监视器 |
| `pio run -e <env> -t clean` | `pio run --environment <env> --target clean` | 清理编译文件 |

## 常用测试流程

### 1️⃣ 硬件验证流程
```bash
# Step 1: 验证 PSRAM
pio run -e test-psram-simple -t upload && pio device monitor -e test-psram-simple

# Step 2: 验证传感器
pio run -e test-sensors -t upload && pio device monitor -e test-sensors

# Step 3: 验证相机
pio run -e test-camera -t upload && pio device monitor -e test-camera

# Step 4: 验证 4G 模块
pio run -e test-4g-modem -t upload && pio device monitor -e test-4g-modem

# Step 5: 验证 GPS
pio run -e test-gps -t upload && pio device monitor -e test-gps

# Step 6: 运行完整程序
pio run -e esp32-s3-pole-monitor -t upload && pio device monitor
```

### 2️⃣ Wokwi 仿真流程
```bash
# 使用 Mock 模式
pio run -e test-mock -t upload

# 在 Wokwi 中运行
# （需要配置 wokwi.toml）
```

### 3️⃣ 快速切换环境
```bash
# 编译所有环境（查看是否有错误）
pio run

# 只编译特定环境
pio run -e test-psram-simple
pio run -e esp32-s3-pole-monitor

# 清理所有环境
pio run -t clean
```

## 环境配置说明

### build_src_filter 过滤规则

```ini
build_src_filter = 
    -<*>                    ; 排除所有文件
    +<../test/xxx.cpp>      ; 只包含指定测试文件
```

**效果：** 只编译单个测试文件，速度快，适合单元测试

### build_flags 宏定义

| 宏定义 | 说明 | 用途 |
|--------|------|------|
| `-DBOARD_HAS_PSRAM` | 启用 PSRAM | 必须 |
| `-DENABLE_CAMERA=1` | 启用相机模块 | 相机测试 |
| `-DENABLE_MOCK_MODE=1` | 启用 Mock 模式 | Wokwi 仿真 |
| `-DCORE_DEBUG_LEVEL=4` | Verbose 日志 | 调试 |

## 故障排查

### 问题 1: 编译错误 "file not found"

**原因：** 测试文件不存在

**解决：**
```bash
# 检查文件是否存在
ls test/test_psram_simple.cpp

# 如果不存在，从模板复制
cp test/test_psram_simple.cpp.template test/test_psram_simple.cpp
```

### 问题 2: 环境切换后还是旧代码

**原因：** 缓存问题

**解决：**
```bash
# 清理重新编译
pio run -e <env> -t clean
pio run -e <env> -t upload
```

### 问题 3: 串口监视器看不到输出

**原因：** 波特率不匹配

**解决：**
```bash
# 检查 platformio.ini 中的 monitor_speed
# 确保与代码中 Serial.begin(115200) 一致
pio device monitor --baud 115200
```

## VS Code 集成

在 VS Code 底部状态栏可以：
1. 点击 "Default (esp32-s3-pole-monitor)" 切换环境
2. 选择不同的测试环境
3. 点击 "→" 图标上传代码
4. 点击 "🔌" 图标打开串口监视器

## 推荐工作流程

### 新硬件调试
```bash
# 1. 先验证 PSRAM
pio run -e test-psram-simple -t upload && pio device monitor

# 2. 再逐个测试模块
pio run -e test-<module> -t upload && pio device monitor

# 3. 最后运行完整程序
pio run -e esp32-s3-pole-monitor -t upload
```

### 持续开发
```bash
# 开发时使用主环境
pio run -e esp32-s3-pole-monitor -t upload

# 遇到问题时切换到对应测试环境
pio run -e test-<specific-module> -t upload
```

### CI/CD 自动化测试
```bash
# 编译所有环境（验证没有语法错误）
pio run

# 或者指定特定环境列表
pio run -e test-psram-simple -e test-sensors -e esp32-s3-pole-monitor
```
