#pragma once

/**
 * @file MockAudioSensor.h
 * @brief 音频传感器 Mock 实现 - 用于 Wokwi 仿真测试
 */

#include "../../../include/AppConfig.h"
#include "../../../include/Settings.h"
#include "../../interfaces/IAudio.h"

class MockAudioSensor : public IAudio {
private:
    uint32_t callCount;
    bool noiseTriggered;
    uint16_t lastPeakToPeak;
    
public:
    MockAudioSensor() : callCount(0), noiseTriggered(false), lastPeakToPeak(0) {}
    
    bool init() override {
        callCount = 0;
        noiseTriggered = false;
        DEBUG_PRINTLN("[MockAudio] 初始化成功（仿真模式）");
        return true;
    }
    
    uint16_t readPeakToPeak() override {
        callCount++;
        
        // 模拟声音等级
        uint16_t base = random(100, 400);  // 正常安静环境
        
        // 每 5 次第 3 次模拟高噪音（用于测试报警流程）
        if (callCount % 5 == 3 || noiseTriggered) {
            base = random(2600, 3200);  // 模拟高噪音
            noiseTriggered = false;
            DEBUG_PRINTF("[MockAudio] 模拟高噪音! 峰峰值: %d\n", base);
        } else {
            DEBUG_PRINTF("[MockAudio] 正常环境, 峰峰值: %d (第%lu次)\n", base, callCount);
        }
        
        lastPeakToPeak = base;
        return base;
    }
    
    bool isNoiseDetected() override {
        uint16_t level = readPeakToPeak();
        bool detected = level > NOISE_THRESHOLD_HIGH;
        
        if (detected) {
            DEBUG_PRINTF("[MockAudio] ⚠️ 检测到异常声音！峰峰值: %d > 阈值: %d\n", 
                         level, NOISE_THRESHOLD_HIGH);
        }
        
        return detected;
    }
    
    uint8_t getSoundPercent() override {
        return map(lastPeakToPeak, 0, 4095, 0, 100);
    }
    
    void sleep() override {
        DEBUG_PRINTLN("[MockAudio] 进入休眠模式（仿真）");
    }
    
    // ========== Mock 专用方法 ==========
    
    /**
     * @brief 手动触发噪音事件（用于测试）
     */
    void triggerNoise() {
        noiseTriggered = true;
        DEBUG_PRINTLN("[MockAudio] 手动触发噪音事件");
    }
    
    /**
     * @brief 获取上次测量的峰峰值
     */
    uint16_t getLastPeakToPeak() const {
        return lastPeakToPeak;
    }
    
    /**
     * @brief 获取调用次数
     */
    uint32_t getCallCount() const {
        return callCount;
    }
};
