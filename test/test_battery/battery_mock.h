/**
 * @file battery_mock.h
 * @brief Battery Mock 函数 - 用于测试电池电量计算逻辑
 * 
 * 功能：
 *   - 电量百分比计算
 *   - 边界保护测试
 */

#ifndef BATTERY_MOCK_H
#define BATTERY_MOCK_H

#include <Arduino.h>
#include <unity.h>

// ==================== 工具函数 ====================

/**
 * @brief 计算电池电量百分比
 * @param voltage 电池电压
 * @return 电量百分比 (0-100)
 */
int getBatteryPercentage(float voltage) {
    const float BATTERY_MAX = 4.2f;
    const float BATTERY_MIN = 3.4f;
    
    if (voltage >= BATTERY_MAX) return 100;
    if (voltage <= BATTERY_MIN) return 0;
    
    return (int)((voltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN) * 100);
}

// ==================== Mock 测试用例 ====================

/**
 * @brief Mock测试：电量百分比计算
 */
void test_mock_percentage_calculation() {
    Serial.println("\n[TEST] Mock: 电量百分比计算");
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(100, getBatteryPercentage(4.2f), "4.2V = 100%");
    TEST_ASSERT_EQUAL_INT_MESSAGE(75, getBatteryPercentage(4.0f), "4.0V = 75%");
    TEST_ASSERT_EQUAL_INT_MESSAGE(50, getBatteryPercentage(3.8f), "3.8V = 50%");
    TEST_ASSERT_EQUAL_INT_MESSAGE(25, getBatteryPercentage(3.6f), "3.6V = 25%");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, getBatteryPercentage(3.4f), "3.4V = 0%");
    
    Serial.println("✓ 电量百分比计算正确");
}

/**
 * @brief Mock测试：边界保护
 */
void test_mock_boundary_protection() {
    Serial.println("\n[TEST] Mock: 边界保护");
    
    // 超出范围保护
    TEST_ASSERT_EQUAL_INT_MESSAGE(100, getBatteryPercentage(4.5f), "超高电压限制为100%");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, getBatteryPercentage(3.0f), "超低电压限制为0%");
    
    Serial.println("✓ 边界保护正常");
}

#endif // BATTERY_MOCK_H
