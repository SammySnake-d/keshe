/**
 * @file test_battery.cpp
 * @brief 电池电压采集模块测试
 * 
 * 测试目标：
 *   1. ADC 电压读取
 *   2. 电量百分比计算
 *   3. 分压电路校准
 *   4. 采样稳定性
 * 
 * 硬件连接（仅 Real 模式需要）：
 *   GPIO 11 → 电池分压电路 (R16=R17=2kΩ)
 */

#include <Arduino.h>
#include <unity.h>
#include "PinMap.h"

// 引入 Mock 和 Real 测试函数
#include "battery_mock.h"
#include "battery_real.h"

// ==================== 测试主程序 ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║   电池电压采集模块 - 单元测试        ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    UNITY_BEGIN();
    
#if USE_MOCK_HARDWARE == 1
    // Mock 模式测试
    Serial.println("\n========== Mock 测试模式 ==========");
    RUN_TEST(test_mock_percentage_calculation);
    RUN_TEST(test_mock_boundary_protection);
#else
    // Real 硬件测试
    Serial.println("\n========== Real 硬件测试模式 ==========");
    RUN_TEST(test_real_adc_configuration);
    RUN_TEST(test_real_voltage_reading);
    RUN_TEST(test_real_sampling_stability);
    RUN_TEST(test_real_full_workflow);
#endif
    
    UNITY_END();
    
    Serial.println("\n========== 测试完成 ==========");
}

void loop() {
    delay(5000);
    
    float voltage = readBatteryVoltage();
    int percent = getBatteryPercentage(voltage);
    
    Serial.printf("\n[MONITOR] 当前电池: %.2fV (%d%%)\n", voltage, percent);
}
