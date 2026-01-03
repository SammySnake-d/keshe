# 硬件模块单元测试指南

## 📋 测试结构

```
test/
├── test_battery/          # 电池电压采集测试
├── test_lsm6ds3/          # LSM6DS3 倾斜传感器测试
├── test_ov2640/           # OV2640 摄像头测试
├── test_ec800k/           # EC800K 4G模块测试（待添加）
├── test_gps/              # ATGM336H GPS测试（待添加）
├── test_audio/            # 音频传感器测试（待添加）
└── README.md              # 本文档
```

## 🎯 测试理念

### Mock First - Real Second

**每个测试包含两部分：**

1. **Mock 测试**（算法验证）
   - 不依赖真实硬件
   - 验证计算逻辑正确性
   - 快速迭代开发
   - ✅ **必须先通过 Mock 测试**

2. **Real 测试**（硬件验证）
   - 验证硬件连线
   - 检查通信协议
   - 确认数据准确性
   - ⚠️ **Mock 通过后才能进行**

## 🚀 快速开始

### 1. 电池电压采集测试

```bash
# 运行测试
pio test -e test-battery

# 期望输出
[TEST] Mock: 电压计算逻辑 ... ✓
[TEST] Mock: 电量百分比计算 ... ✓
[TEST] Real: ADC 配置 ... ✓
[TEST] Real: 电压读取 ... ✓
```

**通过标准：**
- ✅ Mock 测试：所有计算逻辑正确
- ✅ Real 测试：ADC 读取电压在 3.0V-4.5V 范围内
- ✅ 采样稳定性：标准差 < 0.1V

**硬件连接：**
```
GPIO 11 → 电池分压电路 (R16=R17=2kΩ)
```

---

### 2. LSM6DS3 倾斜传感器测试

```bash
# 运行测试
pio test -e test-lsm6ds3

# 期望输出
[TEST] Mock: 角度计算 ... ✓
[TEST] Mock: 零点校准 ... ✓
[TEST] Mock: 阈值检测（5°） ... ✓
[TEST] Real: I2C 通信 ... ✓
[TEST] Real: 加速度计读取 ... ✓
```

**通过标准：**
- ✅ Mock 测试：角度计算误差 < 1°
- ✅ Real 测试：I2C 地址 0x6A 响应
- ✅ 加速度计：总加速度 ≈ 1g（±0.3g）
- ✅ 采样稳定性：标准差 < 2°

**硬件连接：**
```
SDA → GPIO 17
SCL → GPIO 18
INT1 → GPIO 10 (可选)
VCC → 3.3V
GND → GND
```

**故障排查：**
```
❌ "LSM6DS3 未响应"
   → 检查 I2C 连线
   → 确认 I2C 地址（SDO 接地=0x6A，接VCC=0x6B）
   → 测量 VCC 是否为 3.3V

❌ "总加速度不是 1g"
   → 传感器未静止
   → 检查焊接/接触不良
```

---

### 3. OV2640 摄像头测试

```bash
# 运行测试
pio test -e test-ov2640

# 期望输出
[TEST] Mock: JPEG 生成 ... ✓
[TEST] Mock: PSRAM 模拟 ... ✓
[TEST] Real: XCLK 时钟生成 ... ✓
[TEST] Real: 摄像头初始化 ... ✓
[TEST] Real: 拍照测试 ... ✓
```

**通过标准：**
- ✅ Mock 测试：JPEG 头标识正确（FFD8）
- ✅ PSRAM：2MB 可用，分配成功
- ✅ Real 测试：成功拍照，图片 > 1KB
- ✅ 连续拍照：3张照片均成功

**硬件连接：**
```
⚠️ 关键：XCLK → GPIO 2 (需要飞线连接！)

I2C (SCCB):
  SIOD → GPIO 17
  SIOC → GPIO 18

数据线:
  D0-D7 → GPIO 16,38,15,39,41,40,42,21
  VSYNC → GPIO 48
  HREF → GPIO 47
  PCLK → GPIO 45
  PWDN → GPIO 46
```

**故障排查：**
```
❌ "摄像头初始化失败"
   → XCLK 未连接到 GPIO 2
   → SCCB (I2C) 连线错误
   → 电源不足（需要 3.3V@200mA）

❌ "拍照失败"
   → 数据线 D0-D7 连接错误
   → VSYNC/HREF/PCLK 信号线错误
   → PSRAM 未启用
```

---

## 🧪 测试命令汇总

### 运行单个模块测试
```bash
pio test -e test-battery    # 电池测试
pio test -e test-lsm6ds3     # 传感器测试
pio test -e test-ov2640      # 摄像头测试
```

### 运行所有测试
```bash
pio test
```

### 查看测试详细输出
```bash
pio test -e test-battery -v
```

### 编译但不运行
```bash
pio test -e test-battery --without-uploading
```

---

## 📊 测试报告解读

### 成功示例
```
test/test_battery/test_battery.cpp:
  ✓ test_mock_voltage_calculation
  ✓ test_mock_percentage_calculation
  ✓ test_real_adc_configuration
  ✓ test_real_voltage_reading

4 Tests 0 Failures 0 Ignored
PASSED
```

### 失败示例
```
test/test_lsm6ds3/test_lsm6ds3.cpp:
  ✓ test_mock_angle_calculation
  ✗ test_real_i2c_communication
    → Expected 0 Was 5
    → "LSM6DS3 未响应，请检查连线"

2 Tests 1 Failures 0 Ignored
FAILED
```

---

## 🔧 调试技巧

### 1. 启用详细日志
```ini
; platformio.ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5  ; Verbose 日志
```

### 2. 串口监视器
```bash
pio device monitor -b 115200
```

### 3. 隔离测试单个用例
```cpp
void setup() {
    UNITY_BEGIN();
    // RUN_TEST(test_mock_xxx);  // 注释掉不需要的测试
    RUN_TEST(test_real_xxx);     // 只运行这一个
    UNITY_END();
}
```

### 4. 添加调试输出
```cpp
Serial.printf("DEBUG: ADC raw = %d\n", analogRead(PIN_BAT_ADC));
```

---

## 📝 添加新测试模块

### 步骤 1：创建测试文件
```bash
mkdir test/test_new_module
touch test/test_new_module/test_new_module.cpp
```

### 步骤 2：编写测试代码
```cpp
#include <Arduino.h>
#include <unity.h>

// Mock 测试
void test_mock_logic() {
    TEST_ASSERT_EQUAL(expected, actual);
}

// Real 测试
void test_real_hardware() {
    // 初始化硬件
    // 读取数据
    // 验证结果
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_mock_logic);
    RUN_TEST(test_real_hardware);
    UNITY_END();
}

void loop() {}
```

### 步骤 3：添加测试环境
```ini
; platformio.ini
[env:test-new-module]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
test_framework = unity
test_filter = test_new_module
```

### 步骤 4：运行测试
```bash
pio test -e test-new-module
```

---

## ✅ 测试清单

在将模块集成到主项目前，确保：

- [ ] **Mock 测试全部通过**
  - 算法逻辑正确
  - 边界条件处理
  - 数据格式验证

- [ ] **Real 测试全部通过**
  - 硬件通信正常
  - 数据读取准确
  - 采样稳定可靠

- [ ] **连续运行测试**
  - 运行 10 次无失败
  - 无内存泄漏
  - 无异常重启

- [ ] **文档完整**
  - 硬件连接说明
  - 通过标准定义
  - 故障排查指南

---

## 🆘 常见问题

### Q1: "unity.h: No such file"
**A:** Unity 测试框架未安装
```bash
pio lib install "throwtheswitch/Unity"
```

### Q2: Mock 测试通过，Real 测试失败
**A:** 硬件连线问题
1. 检查引脚连接
2. 测量电源电压
3. 使用万用表测试通断

### Q3: 所有测试都通过，但主项目不工作
**A:** 模块间冲突
1. 检查 I2C 地址冲突
2. 检查引脚复用
3. 检查时序问题（如 I2C 需要释放总线）

---

## 📚 参考资料

- [Unity 测试框架](https://github.com/ThrowTheSwitch/Unity)
- [PlatformIO 测试指南](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [ESP32-S3 技术手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

---

**最后更新：** 2026-01-03  
**维护者：** AI Assistant
