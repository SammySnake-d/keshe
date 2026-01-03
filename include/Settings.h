
/**
 * Settings.h
 * 业务逻辑参数配置
 */
#pragma once

// ==================== 传感器阈值 ====================
#define TILT_THRESHOLD              5.0f    // 倾斜报警角度 (度)
#define BAT_LOW_LIMIT               3.4f    // 低电量保护阈值 (V)
#define BAT_CRITICAL_LIMIT          3.2f    // 极低电量阈值 (V)

// ==================== 休眠策略 ====================
#define SLEEP_DURATION_NORMAL       5       // 正常休眠时长 (秒) - Wokwi 测试用
#define SLEEP_DURATION_LOW_BAT      10      // 低电量休眠时长 (秒) - Wokwi 测试用
#define SLEEP_DURATION_ALARM        3       // 报警后短休眠 (秒) - Wokwi 测试用

// --- 时间参数 ---
#define HEARTBEAT_SEC       3600   // 心跳/巡检间隔 (秒)
#define DEBOUNCE_COUNT      5      // 倾斜防抖采样次数
#define GPS_TIMEOUT_MS      30000  // GPS 搜星超时时间 (毫秒)

// ==================== MQTT 配置 ====================
#define MQTT_SERVER                 "broker.emqx.io"
#define MQTT_PORT                   1883
#define MQTT_TOPIC_ALARM            "pole/alarm"
#define MQTT_TOPIC_STATUS           "pole/status"
#define MQTT_CLIENT_ID_PREFIX       "ESP32S3_Pole_"

// ==================== 通信超时 ====================
#define NETWORK_CONNECT_TIMEOUT     30000   // 网络连接超时 (ms)
#define MQTT_PUBLISH_TIMEOUT        10000   // MQTT 发布超时 (ms)

// --- 硬件参数校准 ---
#define BAT_VOLTAGE_DIV     2.0f   // 电池分压系数 (R16+R17)/R16
#define ADC_REF_VOLTAGE     3.3f   // ESP32 ADC 参考电压
