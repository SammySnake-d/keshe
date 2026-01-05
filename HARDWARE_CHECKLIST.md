# 硬件到货测试清单

## 🔧 测试前准备

1. 切换到 Real 模式：
   ```cpp
   // include/AppConfig.h
   #define USE_MOCK_HARDWARE   0
   ```

2. 连接串口监视器：
   ```bash
   pio device monitor -b 115200
   ```

---

## 📋 按顺序测试（由简到难）

### ✅ Step 1: PSRAM 验证（最重要！）
```bash
pio run -e test-psram -t upload && pio device monitor
```
**通过标准：** 显示 `Total PSRAM: 2097152 bytes`

**失败排查：**
- 检查 `board_build.arduino.memory_type = qio_qspi`
- 确认板子型号是 N8R2

---

### ✅ Step 2: 电池电压
```bash
pio run -e test-battery -t upload && pio device monitor
```
**通过标准：** 电压在 3.0V-4.5V 范围内

**失败排查：**
- GPIO 11 是否连接分压电路
- 分压电阻 R16=R17=2kΩ

---

### ✅ Step 3: LSM6DS3 倾斜传感器
```bash
pio run -e test-lsm6ds3 -t upload && pio device monitor
```
**通过标准：** I2C 地址 0x6A 响应，加速度 ≈ 1g

**失败排查：**
- SDA → GPIO 17, SCL → GPIO 18
- SDO 接地 = 0x6A，接 VCC = 0x6B
- 电源 3.3V

---

### ✅ Step 4: GPS 模块
```bash
pio run -e test-gps -t upload && pio device monitor
```
**通过标准：** 收到 NMEA 数据，定位成功

**失败排查：**
- TX → GPIO 6, RX → GPIO 7
- 室外或窗边测试（室内无信号）
- 首次定位需要 1-5 分钟

---

### ✅ Step 5: 音频传感器
```bash
pio run -e test-audio -t upload && pio device monitor
```
**通过标准：** ADC 读数随声音变化

**失败排查：**
- GPIO 8 / GPIO 19 连接
- 检查传感器供电

---

### ✅ Step 6: EC800K 4G 模块
```bash
pio run -e test-ec800k -t upload && pio device monitor
```
**通过标准：** AT 指令响应，网络注册成功

**失败排查：**
- TX → GPIO 4, RX → GPIO 5, PWR → GPIO 3
- SIM 卡是否插好
- 天线是否连接
- 等待 10-30 秒网络注册

---

### ✅ Step 7: OV2640 摄像头（最复杂）
```bash
pio run -e test-ov2640 -t upload && pio device monitor
```
**通过标准：** 初始化成功，拍照返回 JPEG 数据

**失败排查：**
1. **XCLK 必须飞线到 GPIO 2！** （参考 docs/Hardware_Fix_XCLK.md）
2. I2C: SIOD → GPIO 17, SIOC → GPIO 18
3. 数据线 D0-D7 顺序检查
4. PSRAM 必须先通过测试
5. 电源需要 3.3V@200mA

---

## 🐛 常见问题速查

| 现象 | 可能原因 | 解决方案 |
|------|----------|----------|
| 无限重启 | PSRAM 配置错误 | 检查 memory_type |
| I2C 无响应 | 地址/连线错误 | 用 I2C Scanner 扫描 |
| 摄像头初始化失败 | XCLK 未连接 | 飞线到 GPIO 2 |
| 4G 无网络 | SIM/天线问题 | 检查硬件连接 |
| GPS 无定位 | 室内无信号 | 移到室外测试 |

---

## 📝 测试记录

| 模块 | 测试日期 | 结果 | 备注 |
|------|----------|------|------|
| PSRAM | | ⬜ | |
| 电池 | | ⬜ | |
| LSM6DS3 | | ⬜ | |
| GPS | | ⬜ | |
| 音频 | | ⬜ | |
| EC800K | | ⬜ | |
| OV2640 | | ⬜ | |

---

## 🚀 全部通过后

运行完整程序：
```bash
pio run -e esp32-s3-pole-monitor -t upload && pio device monitor
```
