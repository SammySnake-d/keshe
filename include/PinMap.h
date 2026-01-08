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

#define PIN_I2C_BUS0_SDA 17
#define PIN_I2C_BUS0_SCL 18

// [模块 1: 摄像头 OV2640]
// 电源控制: U1.16 -> IO46
#define PIN_CAM_PWDN 46 // High=OFF, Low=ON

// 数据总线 (注意: 此处数字均为 GPIO 编号)
// U1.9  -> IO16
// U1.31 -> IO38
// U1.8  -> IO15
// U1.32 -> IO39
// U1.34 -> IO41
// U1.33 -> IO40
// U1.35 -> IO42
// U1.23 -> IO21
#define PIN_CAM_D0 16
#define PIN_CAM_D1 38
#define PIN_CAM_D2 15
#define PIN_CAM_D3 39
#define PIN_CAM_D4 41
#define PIN_CAM_D5 40
#define PIN_CAM_D6 42
#define PIN_CAM_D7 21

// 信号控制
// U1.26 -> IO45
// U1.25 -> IO48
// U1.24 -> IO47
#define PIN_CAM_VSYNC 48
#define PIN_CAM_HREF 47
#define PIN_CAM_PCLK 45               // 对应网表 'CAM_DCLK' (U1.26)
#define PIN_CAM_SIOD PIN_I2C_BUS0_SDA // I2C SDA
#define PIN_CAM_SIOC PIN_I2C_BUS0_SCL // I2C SCL

// [模块 2: 4G 模块 EC800K - 已移除，改用 WiFi]
// U1.5  -> IO5 (未使用)
// U1.4  -> IO4 (未使用)
// U1.15 -> IO3 (未使用)
// #define PIN_EC800_TX 5
// #define PIN_EC800_RX 4
// #define PIN_EC800_DTR 3

// [模块 3: GPS 模块 ATGM336H]
// U1.6  -> IO6
// U1.7  -> IO7
// U1.39 -> IO1
// 接线：ESP32 GPIO6 (TX) → GPS RX，ESP32 GPIO7 (RX) ← GPS TX
#define PIN_GPS_RX 7      // ESP32 的 RX 引脚（接收 GPS 数据）← GPIO7
#define PIN_GPS_TX 6      // ESP32 的 TX 引脚（发送到 GPS）→ GPIO6
#define PIN_GPS_PWR 1     // GPS 电源控制引脚

// [模块 4: 传感器与交互]
// 声音传感器 (模拟信号输出)
// 电路: GMI9767P-58DB麦克风 → LM321S5运放放大 → R5+C13滤波 → GPIO8
// U1.12 -> IO8 (ADC1_CH7)
#define PIN_MIC_ANALOG 8 // ADC 模拟输入（读取音量等级）
// 音频偏置控制: U1.13 -> IO19 (连接到运放电路)
#define PIN_MIC_CTRL 19

// 电池: U1.19 -> IO11
#define PIN_BAT_ADC 11
#define BAT_VOLTAGE_DIV 2.0f

// 按键: U1.17 -> IO9 (Net: BT_CAM)
#define PIN_BUTTON_CAM 9 // 之前漏掉的按键

// [模块 5: LSM6DS3 倾斜传感器]
// I2C 通信 (GPIO 17/18)

#define PIN_LSM_SDA PIN_I2C_BUS0_SDA
#define PIN_LSM_SCL PIN_I2C_BUS0_SCL
