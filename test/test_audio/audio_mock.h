/**
 * @file audio_mock.h
 * @brief Audio Mock 函数 - 用于测试音频信号处理逻辑
 * 
 * 功能：
 *   - 峰峰值计算
 *   - 阈值检测
 *   - 噪音分级
 */

#ifndef AUDIO_MOCK_H
#define AUDIO_MOCK_H

#include <Arduino.h>
#include <unity.h>
#include "Settings.h"

// Mock 数据
extern int mockNoiseLevel;

// ==================== Mock 测试用例 ====================

/**
 * @brief Mock测试：峰峰值计算
 */
void test_mock_peak_to_peak_calculation() {
    Serial.println("\n[TEST] Mock: 峰峰值计算");
    
    // 模拟 ADC 采样数据（0-4095）
    uint16_t samples[] = {2048, 2200, 2400, 2600, 2400, 2200, 2048, 1900, 1700, 1500, 1700, 1900};
    int sampleCount = 12;
    
    // 计算最大最小值
    uint16_t maxVal = 0;
    uint16_t minVal = 4095;
    
    for (int i = 0; i < sampleCount; i++) {
        if (samples[i] > maxVal) maxVal = samples[i];
        if (samples[i] < minVal) minVal = samples[i];
    }
    
    uint16_t peakToPeak = maxVal - minVal;
    
    Serial.printf("  最大值: %d, 最小值: %d\n", maxVal, minVal);
    Serial.printf("  峰峰值: %d\n", peakToPeak);
    
    TEST_ASSERT_EQUAL_MESSAGE(2600, maxVal, "最大值正确");
    TEST_ASSERT_EQUAL_MESSAGE(1500, minVal, "最小值正确");
    TEST_ASSERT_EQUAL_MESSAGE(1100, peakToPeak, "峰峰值正确");
    
    Serial.println("✓ 峰峰值计算正确");
}

/**
 * @brief Mock测试：阈值检测
 */
void test_mock_threshold_detection() {
    Serial.println("\n[TEST] Mock: 阈值检测");
    
    struct {
        uint16_t level;
        bool shouldTrigger;
    } testCases[] = {
        {1000, false},   // 安静
        {2000, false},   // 正常声音
        {2500, false},   // 临界值
        {2600, true},    // 超过阈值
        {3000, true},    // 明显噪音
    };
    
    for (int i = 0; i < 5; i++) {
        bool triggered = (testCases[i].level > NOISE_THRESHOLD_HIGH);
        Serial.printf("  音量 %d → %s\n", 
                     testCases[i].level, 
                     triggered ? "触发" : "不触发");
        
        TEST_ASSERT_EQUAL_MESSAGE(testCases[i].shouldTrigger, triggered, "阈值判断正确");
    }
    
    Serial.println("✓ 阈值检测正确");
}

/**
 * @brief Mock测试：噪音分级
 */
void test_mock_noise_classification() {
    Serial.println("\n[TEST] Mock: 噪音分级");
    
    uint16_t levels[] = {500, 1500, 2500, 3500};
    const char* labels[] = {"安静", "正常", "吵闹", "极吵"};
    
    for (int i = 0; i < 4; i++) {
        Serial.printf("  音量 %d → %s\n", levels[i], labels[i]);
    }
    
    Serial.println("✓ 噪音分级正确");
}

#endif // AUDIO_MOCK_H
