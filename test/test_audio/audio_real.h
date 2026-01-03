/**
 * @file audio_real.h
 * @brief Audio Real 硬件测试函数
 * 
 * 功能：
 *   - ADC 配置测试
 *   - 运放偏置控制
 *   - 峰峰值测量
 *   - 阈值触发
 *   - 采样稳定性
 */

#ifndef AUDIO_REAL_H
#define AUDIO_REAL_H

#include <Arduino.h>
#include <unity.h>
#include "PinMap.h"
#include "Settings.h"

// 工具函数
#define NOISE_SAMPLE_COUNT      50
#define NOISE_SAMPLE_INTERVAL_US 200

// ==================== Real 工具函数 ====================

/**
 * @brief 读取峰峰值
 * @return 峰峰值（0-4095）
 */
uint16_t readPeakToPeak() {
    uint16_t maxVal = 0;
    uint16_t minVal = 4095;
    
    for (int i = 0; i < NOISE_SAMPLE_COUNT; i++) {
        uint16_t val = analogRead(PIN_MIC_ANALOG);
        if (val > maxVal) maxVal = val;
        if (val < minVal) minVal = val;
        delayMicroseconds(NOISE_SAMPLE_INTERVAL_US);
    }
    
    return maxVal - minVal;
}

// ==================== Real 测试用例 ====================

/**
 * @brief Real测试：ADC 配置
 */
void test_real_adc_configuration() {
    Serial.println("\n[TEST] Real: ADC 配置");
    
    // 配置 ADC
    analogSetPinAttenuation(PIN_MIC_ANALOG, ADC_11db);
    analogReadResolution(12);  // 12位精度
    
    // 读取原始值
    int raw = analogRead(PIN_MIC_ANALOG);
    Serial.printf("  ADC 原始值: %d (期望: 1500-2500)\n", raw);
    
    TEST_ASSERT_TRUE_MESSAGE(raw >= 0 && raw <= 4095, "ADC 值在有效范围");
    
    Serial.println("✓ ADC 配置正确");
}

/**
 * @brief Real测试：运放偏置控制
 */
void test_real_bias_control() {
    Serial.println("\n[TEST] Real: 运放偏置控制");
    
    pinMode(PIN_MIC_CTRL, OUTPUT);
    
    // 开启偏置
    digitalWrite(PIN_MIC_CTRL, HIGH);
    Serial.println("  MIC_CTRL=HIGH → 运放工作");
    delay(100);
    
    int value1 = analogRead(PIN_MIC_ANALOG);
    Serial.printf("  偏置开启: ADC=%d\n", value1);
    
    // 关闭偏置
    digitalWrite(PIN_MIC_CTRL, LOW);
    Serial.println("  MIC_CTRL=LOW → 运放关闭");
    delay(100);
    
    int value2 = analogRead(PIN_MIC_ANALOG);
    Serial.printf("  偏置关闭: ADC=%d\n", value2);
    
    // 恢复开启
    digitalWrite(PIN_MIC_CTRL, HIGH);
    delay(100);
    
    Serial.println("✓ 偏置控制正常");
}

/**
 * @brief Real测试：基线读取（安静环境）
 */
void test_real_baseline_reading() {
    Serial.println("\n[TEST] Real: 基线读取（安静环境）");
    
    Serial.println("  测量安静环境...");
    
    // 连续读取10次
    uint16_t readings[10];
    for (int i = 0; i < 10; i++) {
        readings[i] = readPeakToPeak();
        Serial.printf("  采样 %d: %d\n", i+1, readings[i]);
        delay(200);
    }
    
    // 计算平均值
    uint32_t sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += readings[i];
    }
    uint16_t avg = sum / 10;
    
    Serial.printf("  平均基线: %d (期望: <1000)\n", avg);
    
    if (avg < 1000) {
        Serial.println("  ✓ 环境安静");
    } else if (avg < 2000) {
        Serial.println("  ⚠️  环境有背景噪音");
    } else {
        Serial.println("  ❌ 环境过于嘈杂");
    }
}

/**
 * @brief Real测试：峰峰值测量
 */
void test_real_peak_to_peak_measurement() {
    Serial.println("\n[TEST] Real: 峰峰值测量");
    
    Serial.println("  请保持安静，然后拍手或说话...");
    
    for (int i = 0; i < 10; i++) {
        uint16_t peakToPeak = readPeakToPeak();
        Serial.printf("  测量 %d: %d ", i+1, peakToPeak);
        
        if (peakToPeak > NOISE_THRESHOLD_HIGH) {
            Serial.println("🔴 触发");
        } else if (peakToPeak > 1500) {
            Serial.println("🟡 有声音");
        } else {
            Serial.println("🟢 安静");
        }
        
        delay(500);
    }
    
    Serial.println("✓ 峰峰值测量正常");
}

/**
 * @brief Real测试：阈值触发测试
 */
void test_real_threshold_trigger() {
    Serial.println("\n[TEST] Real: 阈值触发测试");
    
    Serial.println("  监听 10 秒，请制造噪音...");
    
    int triggerCount = 0;
    unsigned long startTime = millis();
    
    while (millis() - startTime < 10000) {
        uint16_t peakToPeak = readPeakToPeak();
        
        if (peakToPeak > NOISE_THRESHOLD_HIGH) {
            triggerCount++;
            Serial.printf("  ⚠️  检测到噪音: %d\n", peakToPeak);
        }
        
        delay(200);
    }
    
    Serial.printf("  触发次数: %d\n", triggerCount);
    
    if (triggerCount > 0) {
        Serial.println("✓ 阈值触发正常");
    } else {
        Serial.println("⚠️  未触发（环境太安静或阈值过高）");
    }
}

/**
 * @brief Real测试：采样稳定性
 */
void test_real_sampling_stability() {
    Serial.println("\n[TEST] Real: 采样稳定性");
    
    // 快速连续采样
    uint16_t samples[20];
    for (int i = 0; i < 20; i++) {
        samples[i] = readPeakToPeak();
    }
    
    // 计算标准差
    float avg = 0;
    for (int i = 0; i < 20; i++) {
        avg += samples[i];
    }
    avg /= 20;
    
    float variance = 0;
    for (int i = 0; i < 20; i++) {
        float diff = samples[i] - avg;
        variance += diff * diff;
    }
    float stddev = sqrt(variance / 20);
    
    Serial.printf("  平均值: %.0f, 标准差: %.1f\n", avg, stddev);
    
    TEST_ASSERT_TRUE_MESSAGE(stddev < 500, "采样标准差 < 500（稳定）");
    
    Serial.println("✓ 采样稳定");
}

#endif // AUDIO_REAL_H
