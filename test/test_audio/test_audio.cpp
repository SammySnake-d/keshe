/**
 * @file test_audio.cpp
 * @brief 音频传感器模块测试
 * 
 * 测试目标：
 *   1. ADC 模拟信号读取
 *   2. 峰峰值计算
 *   3. 噪音阈值检测
 *   4. 采样稳定性
 *   5. 运放电路控制
 * 
 * 硬件连接：
 *   MIC_ANALOG → GPIO 8 (ADC1_CH7)
 *   MIC_CTRL → GPIO 19 (运放偏置控制)
 *   
 * 电路原理：
 *   麦克风 → 运放放大 → 滤波 → ADC
 */

#include <Arduino.h>
#include <unity.h>
#include "PinMap.h"
#include "Settings.h"

// 引入 Mock 和 Real 测试函数
#include "audio_mock.h"
#include "audio_real.h"

// Mock 数据
int mockNoiseLevel = 1000;

// ==================== 测试主程序 ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║   音频传感器模块 - 单元测试          ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    UNITY_BEGIN();
    
#if USE_MOCK_HARDWARE == 1
    // Mock 模式测试
    Serial.println("\n========== Mock 测试模式 ==========");
    RUN_TEST(test_mock_peak_to_peak_calculation);
    RUN_TEST(test_mock_threshold_detection);
    RUN_TEST(test_mock_noise_classification);
#else
    // Real 硬件测试
    Serial.println("\n========== Real 硬件测试模式 ==========");
    Serial.println("提示: 确保麦克风连线正确且偏置电路工作\n");
    
    RUN_TEST(test_real_adc_configuration);
    RUN_TEST(test_real_bias_control);
    RUN_TEST(test_real_baseline_reading);
    RUN_TEST(test_real_peak_to_peak_measurement);
    RUN_TEST(test_real_threshold_trigger);
    RUN_TEST(test_real_sampling_stability);
#endif
    
    UNITY_END();
    
    Serial.println("\n========== 测试完成 ==========");
}

void loop() {
    delay(1000);
    
    // 持续监测音量
    uint16_t peakToPeak = readPeakToPeak();
    
    Serial.printf("\n[MONITOR] 音量: %d ", peakToPeak);
    
    if (peakToPeak > NOISE_THRESHOLD_HIGH) {
        Serial.println("🔴 噪音报警！");
    } else if (peakToPeak > 1500) {
        Serial.println("🟡 有声音");
    } else {
        Serial.println("🟢 安静");
    }
}
