#pragma once

/**
 * @file SystemManager.h
 * @brief 系统级管理：休眠、唤醒、电源监控
 */

#include "../../include/AppConfig.h"
#include <esp_sleep.h>

// ==========================================
// [关键] RTC 变量定义（深度睡眠后保持）
// 类的静态成员无法使用 RTC_DATA_ATTR，必须定义为全局变量
// ==========================================
RTC_DATA_ATTR float g_initialPitch = 0.0f;  // 零点校准值：俯仰角
RTC_DATA_ATTR float g_initialRoll  = 0.0f;  // 零点校准值：横滚角
RTC_DATA_ATTR float g_mockVoltage  = 4.0f;  // Mock 电池电压（模拟下降）

class SystemManager {
private:
    // 移除静态成员，改用上方的 RTC 全局变量
    
public:
    /**
     * @brief 系统初始化
     */
    static void init() {
        // 打印启动横幅
        printBanner();
        
        // 配置唤醒源
        configureWakeupSources();
    }
    
    /**
     * @brief 记录初始姿态（零点校准）
     * @param pitch 初始俯仰角
     * @param roll 初始横滚角
     */
    static void calibrateInitialPose(float pitch, float roll) {
        g_initialPitch = pitch;  // 保存到 RTC 内存
        g_initialRoll = roll;
        DEBUG_PRINTF("[SYS] 零点校准完成: Pitch=%.2f°, Roll=%.2f°\n", pitch, roll);
    }
    
    /**
     * @brief 获取相对倾角（相对于初始姿态）
     * @param currentPitch 当前俯仰角
     * @param currentRoll 当前横滚角
     * @return 最大倾斜角度
     */
    static float getRelativeTilt(float currentPitch, float currentRoll) {
        float deltaPitch = abs(currentPitch - g_initialPitch);  // 从 RTC 内存读取
        float deltaRoll = abs(currentRoll - g_initialRoll);
        return max(deltaPitch, deltaRoll);
    }
    
    /**
     * @brief 获取唤醒原因
     * @return 唤醒原因枚举
     */
    static esp_sleep_wakeup_cause_t getWakeupCause() {
        return esp_sleep_get_wakeup_cause();
    }

    /**
     * @brief 进入深度睡眠
     * @param seconds 睡眠时长（秒）
     */
    static void deepSleep(uint32_t seconds) {
        DEBUG_PRINTF("\n[SYS] 准备进入深度睡眠: %d 秒\n", seconds);
        
        // 清理资源
        Serial.flush();
        delay(100); // 等待串口输出完成
        
        #if ENABLE_DEEP_SLEEP
            // 真实深度睡眠（仅用于真实硬件）
            esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
            DEBUG_PRINTLN("[SYS] 系统休眠中... ZZZ");
            esp_deep_sleep_start();
        #else
            // Wokwi 模拟模式：使用短循环代替长 delay（避免看门狗超时）
            DEBUG_PRINTLN("[SYS] 🔧 Wokwi 模式：模拟休眠（保持内存）");
            for (uint32_t i = 0; i < seconds; i++) {
                delay(1000);  // 每秒喂一次狗
                yield();      // 让出 CPU，避免看门狗
            }
            DEBUG_PRINTLN("[SYS] ⏰ 定时器唤醒（模拟）\n");
            // 返回后让程序自然进入 loop()
        #endif
    }

    /**
     * @brief 读取电池电压
     * @return 电压值 (V)
     */
    static float readBatteryVoltage() {
        #if USE_MOCK_HARDWARE
            // Mock: 模拟电压在 3.5V ~ 4.2V 之间波动
            g_mockVoltage -= 0.05f; // 每次调用下降 0.05V（RTC 持久化）
            if (g_mockVoltage < 3.3f) g_mockVoltage = 4.2f;
            DEBUG_PRINTF("[SYS] 电池电压 (Mock): %.2fV\n", g_mockVoltage);
            return g_mockVoltage;
        #else
            // Real: ADC 读取，分压系数 2.0
            uint32_t adcRaw = analogRead(PIN_BAT_ADC);
            float voltage = (adcRaw / 4095.0f) * 3.3f * 2.0f;
            DEBUG_PRINTF("[SYS] 电池电压: %.2fV (ADC: %d)\n", voltage, adcRaw);
            return voltage;
        #endif
    }

    /**
     * @brief 检查电池状态
     * @return true=电量充足, false=需要保护
     */
    static bool isBatteryHealthy() {
        float voltage = readBatteryVoltage();
        
        if (voltage < BAT_CRITICAL_LIMIT) {
            DEBUG_PRINTLN("[SYS] ⚠️ 极低电量！强制休眠 24 小时");
            return false;
        }
        
        if (voltage < BAT_LOW_LIMIT) {
            DEBUG_PRINTLN("[SYS] ⚠️ 低电量警告！");
            return false;
        }
        
        return true;
    }

    /**
     * @brief 打印唤醒原因
     */
    static void printWakeupReason() {
        esp_sleep_wakeup_cause_t wakeup_reason = getWakeupCause();
        
        Serial.print("[SYS] 唤醒原因: ");
        switch(wakeup_reason) {
            case ESP_SLEEP_WAKEUP_EXT0:
                Serial.println("GPIO 中断 (声音触发)");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                Serial.println("定时器唤醒 (心跳检测)");
                break;
            case ESP_SLEEP_WAKEUP_UNDEFINED:
            default:
                Serial.println("首次启动 / 复位");
                break;
        }
    }

private:
    /**
     * @brief 配置唤醒源
     */
    static void configureWakeupSources() {
        // 1. 定时器唤醒（主要）
        // 在 deepSleep() 中动态设置
        
        // 2. GPIO 唤醒（声音传感器）
        #if !USE_MOCK_HARDWARE
            esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_MIC_TRIGGER, HIGH);
            DEBUG_PRINTLN("[SYS] 已启用 GPIO 唤醒源 (声音传感器)");
        #endif
    }

    /**
     * @brief 打印启动横幅
     */
    static void printBanner() {
        Serial.println("\n");
        Serial.println("╔════════════════════════════════════════════╗");
        Serial.println("║   通信电缆杆监测系统 - Low Power Guardian  ║");
        Serial.println("╠════════════════════════════════════════════╣");
        Serial.printf( "║   固件版本: %-27s ║\n", FIRMWARE_VERSION);
        Serial.printf( "║   构建时间: %-27s ║\n", BUILD_DATE);
        Serial.printf( "║   运行模式: %-27s ║\n", USE_MOCK_HARDWARE ? "Mock (开发)" : "Real (生产)");
        Serial.println("╚════════════════════════════════════════════╝");
        Serial.println();
    }
};

// 注意：不再需要静态成员初始化，因为已改用 RTC_DATA_ATTR 全局变量
