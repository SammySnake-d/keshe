/**
 * AppConfig.h
 * 系统级编译开关与聚合
 */
#pragma once

#include <Arduino.h>

// ==================== 版本信息 ====================
#define FIRMWARE_VERSION            "v2.0-MVP"
#define BUILD_DATE                  __DATE__ " " __TIME__
#define DEVICE_ID                   "POLE_001"  // 设备唯一标识（生产时替换为实际ID）

// --- 核心模式开关 ---
#define USE_MOCK_HARDWARE   1     // 1=仿真模式, 0=真实硬件

// --- 功能裁剪开关 ---
#define ENABLE_CAMERA       1     // 是否启用摄像头
#define ENABLE_GPS          1     // 是否启用GPS
#define ENABLE_DEEP_SLEEP   0     // 深度睡眠（Wokwi 不支持 RTC 内存！改用 delay 模拟）

// ==================== 调试宏 ====================
#define DEBUG_SERIAL_ENABLE         1       // 串口调试输出
#define DEBUG_VERBOSE               0       // 详细日志

#if DEBUG_SERIAL_ENABLE
    #define DEBUG_PRINT(x)          Serial.print(x)
    #define DEBUG_PRINTLN(x)        Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...)  Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
#endif

// ==================== 系统状态码 ====================
enum SystemState {
    STATE_INIT = 0,
    STATE_CHECK_BATTERY,
    STATE_READ_SENSORS,
    STATE_EVALUATE,
    STATE_ALARM,
    STATE_SLEEP,
    STATE_ERROR
};

// ==================== 报警类型 ====================
enum AlarmType {
    ALARM_NONE = 0,
    ALARM_TILT,
    ALARM_LOW_BATTERY,
    ALARM_LOST_SIGNAL
};

// --- 聚合引入 ---
// 方便其他文件只 include 这一个文件就能拿到所有配置
#include "PinMap.h"
#include "Settings.h"
