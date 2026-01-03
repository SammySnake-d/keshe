#pragma once

/**
 * @file AudioSensor_ADC.h
 * @brief 真实音频传感器实现 - 基于 ADC 模拟信号检测
 * 
 * 硬件电路（来自原理图）：
 *   GMI9767P-58DB (驻极体麦克风)
 *       ↓
 *   LM321S5 (运放放大, 增益由 R8 调节)
 *       ↓
 *   R5 (1kΩ) + C13 (1uF) 低通滤波
 *       ↓
 *   GPIO 8 (ADC1_CH7)
 * 
 * 工作原理：
 *   - 麦克风输出微弱交流信号
 *   - 运放放大后输出 0~3.3V 模拟信号
 *   - 安静时约 1.65V (中点), 有声音时波动
 *   - 通过快速 ADC 采样计算峰峰值
 *   - 峰峰值超过阈值则判定为噪音
 */

#include "../../interfaces/IAudio.h"
#include "../../../include/PinMap.h"

class AudioSensor_ADC : public IAudio {
private:
    uint16_t lastPeakToPeak;
    bool initialized;
    
public:
    AudioSensor_ADC() : lastPeakToPeak(0), initialized(false) {}
    
    bool init() override {
        // 配置 ADC
        pinMode(PIN_MIC_ANALOG, INPUT);
        analogReadResolution(12);       // 12位精度 (0-4095)
        analogSetAttenuation(ADC_11db); // 0-3.3V 量程
        
        initialized = true;
        DEBUG_PRINTLN("[Audio] 初始化成功 (ADC 模式)");
        DEBUG_PRINTF("[Audio] 引脚: GPIO%d, 阈值: %d, 采样数: %d\n", 
                     PIN_MIC_ANALOG, NOISE_THRESHOLD_HIGH, NOISE_SAMPLE_COUNT);
        return true;
    }
    
    uint16_t readPeakToPeak() override {
        if (!initialized) {
            DEBUG_PRINTLN("[Audio] ❌ 未初始化！");
            return 0;
        }
        
        // 快速采样计算峰峰值
        uint16_t minVal = 4095;
        uint16_t maxVal = 0;
        
        unsigned long startTime = micros();
        
        for (int i = 0; i < NOISE_SAMPLE_COUNT; i++) {
            uint16_t sample = analogRead(PIN_MIC_ANALOG);
            if (sample > maxVal) maxVal = sample;
            if (sample < minVal) minVal = sample;
            delayMicroseconds(NOISE_SAMPLE_INTERVAL_US);
        }
        
        unsigned long elapsed = micros() - startTime;
        
        lastPeakToPeak = maxVal - minVal;
        
        DEBUG_PRINTF("[Audio] 采样完成: min=%d, max=%d, 峰峰值=%d (耗时 %lu us)\n", 
                     minVal, maxVal, lastPeakToPeak, elapsed);
        
        return lastPeakToPeak;
    }
    
    bool isNoiseDetected() override {
        uint16_t level = readPeakToPeak();
        bool detected = level > NOISE_THRESHOLD_HIGH;
        
        if (detected) {
            DEBUG_PRINTF("[Audio] ⚠️ 检测到异常声音！峰峰值: %d > 阈值: %d\n", 
                         level, NOISE_THRESHOLD_HIGH);
        } else {
            DEBUG_PRINTF("[Audio] 环境正常，峰峰值: %d\n", level);
        }
        
        return detected;
    }
    
    uint8_t getSoundPercent() override {
        return map(lastPeakToPeak, 0, 4095, 0, 100);
    }
    
    void sleep() override {
        // ADC 无需特殊休眠处理
        DEBUG_PRINTLN("[Audio] 进入休眠模式");
    }
    
    // ========== 扩展方法 ==========
    
    /**
     * @brief 获取上次测量的峰峰值
     */
    uint16_t getLastPeakToPeak() const {
        return lastPeakToPeak;
    }
    
    /**
     * @brief 打印当前状态
     */
    void printStatus() {
        uint16_t level = readPeakToPeak();
        uint8_t percent = getSoundPercent();
        
        DEBUG_PRINTF("[Audio] 状态: 峰峰值=%d (%d%%), 阈值=%d, %s\n",
                     level, percent, NOISE_THRESHOLD_HIGH,
                     level > NOISE_THRESHOLD_HIGH ? "⚠️ 超标" : "✓ 正常");
    }
};
