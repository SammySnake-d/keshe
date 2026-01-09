#pragma once

/**
 * @file AudioSensor_ADC.h
 * @brief 真实音频传感器实现 - 基于 ADC 模拟信号检测
 */

#include "../../interfaces/IAudio.h"
#include "../../../include/PinMap.h"
#include <math.h>

class AudioSensor_ADC : public IAudio {
private:
    uint16_t lastPeakToPeak;
    float lastDb;
    bool initialized;
    
    /**
     * @brief 将峰峰值转换为估算分贝（未校准）
     */
    float peakToDb(uint16_t peak) {
        if (peak <= 1) return 30.0f;
        float db = 30.0f + 20.0f * log10((float)peak);
        return constrain(db, 30.0f, 100.0f);
    }
    
    /**
     * @brief 将分贝阈值转换为峰峰值阈值
     */
    uint16_t dbToPeak(float db) {
        // 反向计算: peak = 10^((db - 30) / 20)
        float peak = pow(10.0f, (db - 30.0f) / 20.0f);
        return (uint16_t)constrain(peak, 1.0f, 4095.0f);
    }
    
public:
    AudioSensor_ADC() : lastPeakToPeak(0), lastDb(30.0f), initialized(false) {}
    
    bool init() override {
        // 1. 开启运放偏置（必须！否则麦克风无输出）
        pinMode(PIN_MIC_CTRL, OUTPUT);
        digitalWrite(PIN_MIC_CTRL, HIGH);
        delay(10);  // 等待运放稳定
        
        // 2. 配置 ADC
        pinMode(PIN_MIC_ANALOG, INPUT);
        analogReadResolution(12);
        analogSetPinAttenuation(PIN_MIC_ANALOG, ADC_11db);
        
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
        lastDb = peakToDb(lastPeakToPeak);
        return lastPeakToPeak;
    }
    
    /**
     * @brief 检测是否有噪音（使用分贝阈值）
     */
    bool isNoiseDetected() override {
        uint16_t threshold = dbToPeak(NOISE_THRESHOLD_DB);
        bool detected = lastPeakToPeak > threshold;
        
        if (detected) {
            DEBUG_PRINTF("[传感器] ⚠️ 噪音: %.0f dB > %d dB\n", lastDb, NOISE_THRESHOLD_DB);
        }
        
        return detected;
    }
    
    uint8_t getSoundPercent() override {
        return map(lastPeakToPeak, 0, 4095, 0, 100);
    }
    
    void sleep() override {
        // 关闭运放偏置省电
        digitalWrite(PIN_MIC_CTRL, LOW);
    }
    
    // ========== 扩展方法 ==========
    
    /**
     * @brief 获取上次测量的峰峰值
     */
    uint16_t getLastPeakToPeak() const {
        return lastPeakToPeak;
    }
    
    /**
     * @brief 获取上次测量的分贝值
     */
    float getLastDb() const {
        return lastDb;
    }
    
    /**
     * @brief 打印当前状态
     */
    void printStatus() {
        uint16_t level = readPeakToPeak();
        DEBUG_PRINTF("[Audio] 状态: %.0f dB (峰峰值=%d), 阈值=%d dB, %s\n",
                     lastDb, level, NOISE_THRESHOLD_DB,
                     lastDb > NOISE_THRESHOLD_DB ? "⚠️ 超标" : "✓ 正常");
    }
};
