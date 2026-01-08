/**
 * @file main.cpp
 * @brief 通信电缆杆监测系统 - MVP 主程序
 * @architecture 多唤醒源状态机 (Timer + Audio + [Future: Tilt INT])
 * @note 主函数只负责流程控制，具体实现在 WorkflowManager 中
 */

#include "../include/AppConfig.h"
#include "core/SystemManager.h"
#include "core/WorkflowManager.h"
#include <Arduino.h>

// ==================== 全局变量 ====================
esp_sleep_wakeup_cause_t wakeupCause;

// RTC 内存：跨越重启保持
RTC_DATA_ATTR uint32_t bootCount = 0;

// WorkflowManager RTC 变量定义
RTC_DATA_ATTR uint32_t WorkflowManager::lastGpsUploadTime = 0;
RTC_DATA_ATTR uint32_t WorkflowManager::g_last_tilt_trigger_ms = 0;

// ==================== 函数声明 ====================
void printBootBanner();
void dispatchWakeupHandler();

// ==================== 主程序入口 ====================
void setup() {
#if DEBUG_SERIAL_ENABLE
  Serial.begin(115200);
#endif
  delay(500);

  printBootBanner();

  // 读取唤醒原因
  wakeupCause = esp_sleep_get_wakeup_cause();

  // 启动计数
  bootCount++;
  DEBUG_PRINTF("\n[MAIN] 🔢 启动计数: %lu (RTC 内存保持)\n", bootCount);

  // 分发到对应的处理流程
  SystemManager::printWakeupReason();
  dispatchWakeupHandler();
}

void loop() {
#if !ENABLE_DEEP_SLEEP
  // 测试模式：循环执行心跳流程
  static bool firstLoopDone = false;
  
  if (!firstLoopDone) {
    DEBUG_PRINTLN("\n[MAIN] 🔧 测试模式：进入循环心跳");
    firstLoopDone = true;
  }
  
  // 模拟定时器唤醒，执行心跳流程
  wakeupCause = ESP_SLEEP_WAKEUP_TIMER;
  SystemManager::printWakeupReason();
  dispatchWakeupHandler();
  // dispatchWakeupHandler 内部会调用 deepSleep()，在测试模式下只是短延迟
#else
  // 真实硬件模式：loop 永远不会执行（深度睡眠后重启）
  delay(10000);
#endif
}
// ==================== 辅助函数实现 ====================

/**
 * @brief 打印启动横幅
 */
void printBootBanner() {
  DEBUG_PRINTLN("\n\n╔════════════════════════════════════════════╗");
  DEBUG_PRINTLN("║   通信电缆杆监测系统 - Low Power Guardian  ║");
  DEBUG_PRINTLN("╠════════════════════════════════════════════╣");
  DEBUG_PRINTF("║   固件版本: %-28s║\n", FIRMWARE_VERSION);
  DEBUG_PRINTF("║   构建时间: %-28s║\n", __DATE__ " " __TIME__);

#if USE_MOCK_HARDWARE
  DEBUG_PRINTLN("║   运行模式: Mock (开发)               ║");
#else
  DEBUG_PRINTLN("║   运行模式: Real (生产)               ║");
#endif

  DEBUG_PRINTLN("╚════════════════════════════════════════════╝\n");
};
/**
 * @brief 根据唤醒原因分发到对应的处理函数
 */
void dispatchWakeupHandler() {
  switch (wakeupCause) {
  case ESP_SLEEP_WAKEUP_TIMER:
    // 定时器唤醒：心跳巡检
    WorkflowManager::handleTimerWakeup();
    break;

  case ESP_SLEEP_WAKEUP_EXT0:
    // 外部中断 0：声音触发（GPIO 8）
    WorkflowManager::handleAudioWakeup();
    break;

  case ESP_SLEEP_WAKEUP_EXT1:
    // 外部中断 1：倾斜中断（GPIO 10，未来启用）
    DEBUG_PRINTLN("\n[MAIN] 📐 倾斜中断唤醒（未实现）");
    SystemManager::deepSleep(HEARTBEAT_INTERVAL_SEC);
    break;

  case ESP_SLEEP_WAKEUP_UNDEFINED:
  default:
    // 首次启动或复位：执行校准
    SystemManager::readBatteryVoltage(); // 首次读取显示电压
    WorkflowManager::handleFirstBoot();
    SystemManager::deepSleep(HEARTBEAT_INTERVAL_SEC);
    break;
  };
}
