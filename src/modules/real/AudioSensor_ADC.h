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
        pinMode(PIN_MIC_ANALOG, INPUT);
        analogReadResolution(12);
        analogSetAttenuation(ADC_11db);
        initialized = true;
        return true;
    }
    
    uint16_t readPeakToPeak() override {
        if (!initialized) return 0;
        
        uint16_t minVal = 4095;
        uint16_t maxVal = 0;
        
        for (int i = 0; i < NOISE_SAMPLE_COUNT; i++) {
            uint16_t sample = analogRead(PIN_MIC_ANALOG);
            if (sample > maxVal) maxVal = sample;
            if (sample < minVal) minVal = sample;
            delayMicroseconds(NOISE_SAMPLE_INTERVAL_US);
        }
        
        lastPeakToPeak = maxVal - minVal;
        return lastPeakToPeak;
    }
    
    /**
     * @brief 检测是否有噪音（使用上次读取的值）
     * @note 调用前应先调用 readPeakToPeak() 获取最新值
     */
    bool isNoiseDetected() override {
        bool detected = lastPeakToPeak > NOISE_THRESHOLD_HIGH;
        
        if (detected) {
            DEBUG_PRINTF("[传感器] ⚠️ 噪音: %d > %d\n", lastPeakToPeak, NOISE_THRESHOLD_HIGH);
        }
        
        return detected;
    }
    
    uint8_t getSoundPercent() override {
        return map(lastPeakToPeak, 0, 4095, 0, 100);
    }
    
    void sleep() override {
        // ADC 无需特殊休眠处理
    }
    
    // ========== 扩展方法 ==========
    
    /**
     * @brief 获取上次测量的峰峰值
     */
    uint16_t getLastPeakToPeak() const {
        return lastPeakToPeak;
    }
    
    /**
     * @brief 估算分贝值（仅供参考，未经校准）
     * @note 基于峰峰值的对数映射，大致范围 30-90 dB
     *       实际分贝需要专业设备校准
     */
    float estimateDb() const {
        if (lastPeakToPeak <= 1) return 30.0f;  // 静音基准
        // 对数映射: 峰峰值 1-4095 → 约 30-90 dB
        float db = 30.0f + 20.0f * log10((float)lastPeakToPeak);
        return constrain(db, 30.0f, 100.0f);
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
