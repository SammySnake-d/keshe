/**
 * @file lsm6ds3_mock.h
 * @brief LSM6DS3 Mock 函数 - 用于测试角度计算逻辑
 * 
 * 功能：
 *   - 模拟加速度数据
 *   - 角度计算
 *   - 零点校准逻辑
 *   - 阈值检测验证
 */

#ifndef LSM6DS3_MOCK_H
#define LSM6DS3_MOCK_H

#include <Arduino.h>
#include <unity.h>

// Mock 数据结构
struct MockAccelData {
    float ax, ay, az;
};

// 全局 Mock 数据（初始：水平静止）
MockAccelData mockData = {0.0f, 0.0f, 1.0f};

// ==================== Mock 工具函数 ====================

/**
 * @brief 根据加速度计算 Pitch 角度
 * @param ax X轴加速度
 * @param ay Y轴加速度
 * @param az Z轴加速度
 * @return Pitch 角度（度）
 */
float calculatePitch(float ax, float ay, float az) {
    return atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
}

/**
 * @brief 根据加速度计算 Roll 角度
 * @param ay Y轴加速度
 * @param ax X轴加速度
 * @param az Z轴加速度
 * @return Roll 角度（度）
 */
float calculateRoll(float ay, float ax, float az) {
    return atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
}

/**
 * @brief 根据目标角度设置模拟加速度值
 * @param pitch_deg 目标 Pitch 角度
 * @param roll_deg 目标 Roll 角度
 */
void setMockTilt(float pitch_deg, float roll_deg) {
    // 根据角度反算加速度分量
    float pitch_rad = pitch_deg * PI / 180.0;
    float roll_rad = roll_deg * PI / 180.0;
    
    mockData.ax = sin(pitch_rad);
    mockData.ay = sin(roll_rad);
    mockData.az = cos(pitch_rad) * cos(roll_rad);
    
    Serial.printf("Mock 设置: Pitch=%.1f°, Roll=%.1f° → Ax=%.2f, Ay=%.2f, Az=%.2f\n",
                 pitch_deg, roll_deg, mockData.ax, mockData.ay, mockData.az);
}

// ==================== Mock 测试用例 ====================

/**
 * @brief Mock测试：角度计算准确性
 */
void test_mock_angle_calculation() {
    Serial.println("\n[TEST] Mock: 角度计算");
    
    // 测试水平（0°）
    setMockTilt(0, 0);
    float pitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, 0.0f, pitch, "水平状态 Pitch=0°");
    
    // 测试前倾 10°
    setMockTilt(10, 0);
    pitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, 10.0f, pitch, "前倾 10°");
    
    // 测试后倾 -10°
    setMockTilt(-10, 0);
    pitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, -10.0f, pitch, "后倾 -10°");
    
    Serial.println("✓ 角度计算正确");
}

/**
 * @brief Mock测试：零点校准逻辑
 */
void test_mock_calibration() {
    Serial.println("\n[TEST] Mock: 零点校准");
    
    // 模拟杆子初始倾斜 8°
    setMockTilt(8, 0);
    float initialPitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    Serial.printf("  初始角度: %.2f°\n", initialPitch);
    
    // 模拟倾斜到 14°（相对倾斜 6°）
    setMockTilt(14, 0);
    float currentPitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    float relativeTilt = abs(currentPitch - initialPitch);
    
    Serial.printf("  当前角度: %.2f°, 相对倾斜: %.2f°\n", currentPitch, relativeTilt);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, 6.0f, relativeTilt, "相对倾斜 = 6°");
    
    Serial.println("✓ 零点校准逻辑正确");
}

/**
 * @brief Mock测试：阈值检测（5°倾斜触发）
 */
void test_mock_threshold_detection() {
    Serial.println("\n[TEST] Mock: 阈值检测（5°）");
    
    // 初始 0°
    setMockTilt(0, 0);
    float initialPitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    
    // 测试 4° - 不应触发
    setMockTilt(4, 0);
    float pitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    float tilt = abs(pitch - initialPitch);
    Serial.printf("  倾斜 %.2f° → %s\n", tilt, tilt > 5.0f ? "触发" : "不触发");
    TEST_ASSERT_TRUE_MESSAGE(tilt <= 5.0f, "4° 不触发");
    
    // 测试 6° - 应该触发
    setMockTilt(6, 0);
    pitch = calculatePitch(mockData.ax, mockData.ay, mockData.az);
    tilt = abs(pitch - initialPitch);
    Serial.printf("  倾斜 %.2f° → %s\n", tilt, tilt > 5.0f ? "触发" : "不触发");
    TEST_ASSERT_TRUE_MESSAGE(tilt > 5.0f, "6° 触发报警");
    
    Serial.println("✓ 阈值检测正确");
}

#endif // LSM6DS3_MOCK_H
