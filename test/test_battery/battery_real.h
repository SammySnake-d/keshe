/**
 * @file battery_real.h
 * @brief Battery Real 硬件测试函数
 * 
 * 功能：
 *   - ADC 配置测试
 *   - 电压读取
 *   - 采样稳定性测试
 *   - 完整流程测试
 */

#ifndef BATTERY_REAL_H
#define BATTERY_REAL_H

#include <Arduino.h>
#include <unity.h>
#include "PinMap.h"
#include "battery_mock.h"  // 使用 Mock 中的百分比计算函数

// ==================== 工具函数 ====================

/**
 * @brief 读取电池电压
 * @return 电池电压（V）
 */
float readBatteryVoltage() {
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
    
    const int SAMPLES = 10;
    uint32_t sum_mv = 0;
    
    for (int i = 0; i < SAMPLES; i++) {
        sum_mv += analogReadMilliVolts(PIN_BAT_ADC);
        delay(5);
    }
    
    float avg_mv = sum_mv / (float)SAMPLES;
    float measured_voltage = avg_mv / 1000.0f;
    float battery_voltage = measured_voltage * BAT_VOLTAGE_DIV;
    
    return battery_voltage;
}

// ==================== Real 测试用例 ====================

/**
 * @brief Real测试：ADC 配置
 */
void test_real_adc_configuration() {
    Serial.println("\n[TEST] Real: ADC 配置");
    
    // 配置 ADC
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
    analogReadResolution(12);  // 12位精度
    
    // 读取原始 ADC 值
    int raw = analogRead(PIN_BAT_ADC);
    Serial.printf("  ADC 原始值: %d (期望: 0-4095)\n", raw);
    
    TEST_ASSERT_TRUE_MESSAGE(raw >= 0 && raw <= 4095, "ADC 值在有效范围内");
    
    Serial.println("✓ ADC 配置正确");
}

/**
 * @brief Real测试：电压读取
 */
void test_real_voltage_reading() {
    Serial.println("\n[TEST] Real: 电压读取");
    
    float voltage = readBatteryVoltage();
    Serial.printf("  测量电池电压: %.2fV\n", voltage);
    
    // 合理性检查（锂电池范围）
    TEST_ASSERT_TRUE_MESSAGE(voltage >= 3.0f && voltage <= 4.5f, 
                            "电压在合理范围内 (3.0V-4.5V)");
    
    Serial.println("✓ 电压读取正常");
}

/**
 * @brief Real测试：采样稳定性
 */
void test_real_sampling_stability() {
    Serial.println("\n[TEST] Real: 采样稳定性");
    
    // 连续读取5次
    float readings[5];
    for (int i = 0; i < 5; i++) {
        readings[i] = readBatteryVoltage();
        Serial.printf("  采样 %d: %.2fV\n", i+1, readings[i]);
        delay(100);
    }
    
    // 计算标准差（应该很小）
    float avg = 0;
    for (int i = 0; i < 5; i++) avg += readings[i];
    avg /= 5;
    
    float variance = 0;
    for (int i = 0; i < 5; i++) {
        float diff = readings[i] - avg;
        variance += diff * diff;
    }
    float stddev = sqrt(variance / 5);
    
    Serial.printf("  平均值: %.2fV, 标准差: %.3fV\n", avg, stddev);
    TEST_ASSERT_TRUE_MESSAGE(stddev < 0.1f, "采样标准差 < 0.1V（稳定）");
    
    Serial.println("✓ 采样稳定");
}

/**
 * @brief Real测试：完整流程测试
 */
void test_real_full_workflow() {
    Serial.println("\n[TEST] Real: 完整流程测试");
    
    float voltage = readBatteryVoltage();
    int percent = getBatteryPercentage(voltage);
    
    Serial.printf("  🔋 电池状态: %.2fV (%d%%)\n", voltage, percent);
    
    // 显示电池状态
    if (percent >= 75) {
        Serial.println("  状态: ✅ 电量充足");
    } else if (percent >= 25) {
        Serial.println("  状态: 🟡 电量中等");
    } else {
        Serial.println("  状态: 🔴 需要充电");
    }
    
    TEST_ASSERT_TRUE_MESSAGE(percent >= 0 && percent <= 100, "电量百分比有效");
    
    Serial.println("✓ 完整流程正常");
}

#endif // BATTERY_REAL_H
