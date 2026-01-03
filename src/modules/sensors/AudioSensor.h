#pragma once

/**
 * @file AudioSensor.h
 * @brief 环境音传感器模块
 * @note GPIO 8 连接声音比较器输出（LM393）
 *       High = 超过阈值，Low = 正常
 */

#include "../../../include/AppConfig.h"

class AudioSensor {
public:
    /**
     * @brief 初始化声音传感器
     */
    static void init() {
        pinMode(PIN_MIC_TRIGGER, INPUT);
        DEBUG_PRINTLN("[Audio] 声音传感器初始化完成");
    }
    
    /**
     * @brief 检测当前是否有异常声音
     * @return true=检测到异常声音, false=正常
     */
    static bool isNoiseDetected() {
        bool detected = digitalRead(PIN_MIC_TRIGGER) == HIGH;
        
        if (detected) {
            DEBUG_PRINTLN("[Audio] ⚠️ 检测到异常声音！");
        }
        
        return detected;
    }
    
    /**
     * @brief 配置为中断唤醒源
     * @note 用于深度睡眠期间的声音唤醒
     */
    static void enableWakeupInterrupt() {
        #if !USE_MOCK_HARDWARE
            esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_MIC_TRIGGER, HIGH);
            DEBUG_PRINTLN("[Audio] 已启用声音中断唤醒");
        #endif
    }
    
    /**
     * @brief 读取声音传感器状态（调试用）
     */
    static void printStatus() {
        int level = digitalRead(PIN_MIC_TRIGGER);
        DEBUG_PRINTF("[Audio] 当前状态: %s\n", level == HIGH ? "HIGH (噪音)" : "LOW (安静)");
    }
};
