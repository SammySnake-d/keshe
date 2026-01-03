/**
 * PinMap.h
 * 硬件引脚映射 - 基于网表 Netlist $NETS 深度解析
 * 对应模组: ESP32-S3-WROOM-1
 * 
 * 修正说明: 已将模组物理引脚号(U1.x)转换为软件所需的 GPIO 编号
 */
#pragma once

// ==========================================
// 📌 PCB 已连接引脚 (Fixed Connections)
// ==========================================

// [I2C 总线] (摄像头与IMU共用)
// U1.10 -> IO17
// U1.11 -> IO18
#define PIN_I2C_SDA         17
#define PIN_I2C_SCL         18

// [模块 1: 摄像头 OV2640]
// 电源控制: U1.16 -> IO46
#define PIN_CAM_PWDN        46  // High=OFF, Low=ON

// 数据总线 (注意: 此处数字均为 GPIO 编号)
// U1.9  -> IO16
// U1.31 -> IO38
// U1.8  -> IO15
// U1.32 -> IO39
// U1.34 -> IO41
// U1.33 -> IO40
// U1.35 -> IO42
// U1.23 -> IO21
#define PIN_CAM_D0          16
#define PIN_CAM_D1          38
#define PIN_CAM_D2          15
#define PIN_CAM_D3          39
#define PIN_CAM_D4          41
#define PIN_CAM_D5          40
#define PIN_CAM_D6          42
#define PIN_CAM_D7          21

// 信号控制
// U1.26 -> IO45
// U1.25 -> IO48
// U1.24 -> IO47
#define PIN_CAM_VSYNC       48
#define PIN_CAM_HREF        47
#define PIN_CAM_PCLK        45  // 对应网表 'CAM_DCLK' (U1.26)
#define PIN_CAM_SIOD        17  // I2C SDA
#define PIN_CAM_SIOC        18  // I2C SCL

// [模块 2: 4G 模块 EC800K]
// U1.5  -> IO5
// U1.4  -> IO4
// U1.15 -> IO3 (注意: IO3也是JTAG脚)
#define PIN_EC800_TX        5   // ESP32_TX -> EC800_RX
#define PIN_EC800_RX        4   // ESP32_RX -> EC800_TX
#define PIN_EC800_DTR       3   // 休眠控制: HIGH=Sleep

// [模块 3: GPS 模块 ATGM336H]
// U1.6  -> IO6
// U1.7  -> IO7
// U1.39 -> IO1 (注意: U1.39是物理脚，对应IO1)
#define PIN_GPS_TX          6   // ESP32_RX
#define PIN_GPS_RX          7   // ESP32_TX
#define PIN_GPS_PWR         1   // Low=ON

// [模块 4: 传感器与交互]
// 声音传感器 (模拟信号输出)
// 电路: GMI9767P-58DB麦克风 → LM321S5运放放大 → R5+C13滤波 → GPIO8
// U1.12 -> IO8 (ADC1_CH7)
#define PIN_MIC_ANALOG      8   // ADC 模拟输入（读取音量等级）
// 音频偏置控制: U1.13 -> IO19 (连接到运放电路)
#define PIN_MIC_CTRL        19 

// 电池: U1.19 -> IO11
#define PIN_BAT_ADC         11
#define BAT_VOLTAGE_DIV     2.0f

// 按键: U1.17 -> IO9 (Net: BT_CAM)
#define PIN_BUTTON_CAM      9   // 之前漏掉的按键

// [模块 5: LSM6DS3 倾斜传感器]
// I2C 通信 (GPIO 17/18)

// ==========================================
// 🔧 扩展与未使用 (Extensions)
// ==========================================

// ==========================================
// 🟢 剩余可用 GPIO (Available for Future)
// ==========================================
// IO 13, 14, 20, 35, 36, 37 可用于扩展
// (U1.22, U1.14, U1.27, U1.29, U1.30)

// ==========================================
// 🔧 摄像头补充引脚 (需要飞线连接)
// ==========================================
// OV2640 必须要有 XCLK 主时钟才能工作
// ESP32-S3 通过 LEDC PWM 外设生成 20MHz 时钟信号
// 
// 硬件连接方式:
//   方案A (推荐): GPIO2 (U1.21) 飞线到 OV2640 的 XCLK 引脚
//   方案B: 使用外部 20MHz 晶振连接到 OV2640 的 XCLK
#define PIN_CAM_XCLK        2   // U1.21 -> IO2 (LEDC 生成 20MHz 时钟)
#define PIN_CAM_RESET       -1  // 未连接 (可选，通过 SCCB 软复位)


// LSM6DS3 中断引脚 (PCB 未连接，需飞线)
// 选择依据: RTC GPIO + 物理位置靠近 I2C
#define PIN_LSM6DS3_INT1    10  // U1.18 -> IO10 (倾斜中断，需 4.7kΩ 上拉)
#define PIN_LSM6DS3_INT2    12  // U1.20 -> IO12 (自由落体中断，可选)

// ==========================================
// ⚠️ 潜在冲突警告
// ==========================================
// IO 3 (EC800_DTR): 也是 JTAG TCK。初始化时需确保 pinMode 为 OUTPUT。
// IO 46 (CAM_PWDN): 也是 Boot Log 引脚，启动时可能有输出。
// IO 1/3/4/5/6/7/8/9/11/15/16/17/18/19/21/38/39/40/41/42/45/46/47/48 已被占用。
