/**
 * AppConfig.h
 * 系统级编译开关与聚合
 */
#pragma once

#include <Arduino.h>

// ==================== 版本信息 ====================
#define FIRMWARE_VERSION "v2.0-MVP"
#define BUILD_DATE __DATE__ " " __TIME__
#define DEVICE_ID "POLE_001" // 设备唯一标识（生产时替换为实际ID）

// --- 核心模式开关 ---
#define USE_MOCK_HARDWARE 0 // 1=仿真模式, 0=真实硬件 ← 改成 0

// --- 功能裁剪开关 ---
#define ENABLE_CAMERA 1         // 是否启用摄像头
#define ENABLE_GPS 1            // 是否启用GPS
#define ENABLE_DEEP_SLEEP 0     // 深度睡眠 ← 0=禁用(测试), 1=启用(生产)

// ==================== 调试宏 ====================
// ==================== 调试系统配置 ====================
// 调试级别定义
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1 // 仅错误
#define LOG_LEVEL_INFO 2  // 关键流程信息 (默认)
#define LOG_LEVEL_DEBUG 3 // 详细调试数据

// 当前调试级别设置 (修改此处控制日志详细程度)
#define APP_LOG_LEVEL LOG_LEVEL_DEBUG
#define DEBUG_SERIAL_ENABLE (APP_LOG_LEVEL > LOG_LEVEL_NONE)

#if DEBUG_SERIAL_ENABLE
// 基础输出宏 (兼容现有代码)
#define DEBUG_PRINT(x)                                                         \
  if (APP_LOG_LEVEL >= LOG_LEVEL_DEBUG)                                        \
  Serial.print(x)
#define DEBUG_PRINTLN(x)                                                       \
  if (APP_LOG_LEVEL >= LOG_LEVEL_DEBUG)                                        \
  Serial.println(x)
#define DEBUG_PRINTF(fmt, ...)                                                 \
  if (APP_LOG_LEVEL >= LOG_LEVEL_DEBUG)                                        \
  Serial.printf(fmt, ##__VA_ARGS__)

// 结构化日志宏 (推荐新代码使用)
#define LOG_E(tag, fmt, ...)                                                   \
  if (APP_LOG_LEVEL >= LOG_LEVEL_ERROR)                                        \
  Serial.printf("[%s] [ERR] " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...)                                                   \
  if (APP_LOG_LEVEL >= LOG_LEVEL_INFO)                                         \
  Serial.printf("[%s] [INF] " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...)                                                   \
  if (APP_LOG_LEVEL >= LOG_LEVEL_DEBUG)                                        \
  Serial.printf("[%s] [DBG] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(fmt, ...)
#define LOG_E(tag, fmt, ...)
#define LOG_I(tag, fmt, ...)
#define LOG_D(tag, fmt, ...)
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
