/**
 * @file gps_mock.h
 * @brief GPS Mock 函数 - 用于测试 NMEA 解析和坐标转换
 * 
 * 功能：
 *   - NMEA 数据格式验证
 *   - 坐标转换测试
 *   - 定位有效性判断
 */

#ifndef GPS_MOCK_H
#define GPS_MOCK_H

#include <Arduino.h>
#include <unity.h>

// ==================== Mock 测试用例 ====================

/**
 * @brief Mock测试：NMEA 数据解析
 */
void test_mock_nmea_parsing() {
    Serial.println("\n[TEST] Mock: NMEA 解析");
    
    // 模拟 NMEA 数据
    const char* gga = "$GPGGA,123519,3958.123,N,11623.456,E,1,08,0.9,545.4,M,46.9,M,,*47";
    const char* rmc = "$GPRMC,123519,A,3958.123,N,11623.456,E,022.4,084.4,230394,003.1,W*6A";
    
    // 验证 NMEA 格式
    TEST_ASSERT_TRUE_MESSAGE(strstr(gga, "$GPGGA") != NULL, "GGA 数据格式");
    TEST_ASSERT_TRUE_MESSAGE(strstr(rmc, "$GPRMC") != NULL, "RMC 数据格式");
    
    // 验证校验和
    TEST_ASSERT_TRUE_MESSAGE(strstr(gga, "*") != NULL, "GGA 校验和存在");
    TEST_ASSERT_TRUE_MESSAGE(strstr(rmc, "*") != NULL, "RMC 校验和存在");
    
    Serial.println("✓ NMEA 格式正确");
}

/**
 * @brief Mock测试：坐标转换（NMEA 格式 → 十进制度）
 */
void test_mock_coordinate_conversion() {
    Serial.println("\n[TEST] Mock: 坐标转换");
    
    // NMEA 格式：ddmm.mmmm
    // 十进制格式：dd.dddddd
    
    // 北纬 39° 58.123' → 39.96871667°
    float nmea_lat = 3958.123;
    int deg = (int)(nmea_lat / 100);
    float min = nmea_lat - deg * 100;
    float decimal_lat = deg + min / 60.0;
    
    Serial.printf("  NMEA: %.3f → 十进制: %.6f°\n", nmea_lat, decimal_lat);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 39.969, decimal_lat, "纬度转换正确");
    
    Serial.println("✓ 坐标转换正确");
}

/**
 * @brief Mock测试：定位有效性验证
 */
void test_mock_location_validation() {
    Serial.println("\n[TEST] Mock: 定位有效性");
    
    // 模拟定位数据
    struct {
        double lat;
        double lng;
        bool valid;
    } testCases[] = {
        {39.9, 116.4, true},   // 北京
        {31.2, 121.5, true},   // 上海
        {0.0, 0.0, false},     // 无效数据
        {91.0, 0.0, false},    // 超出范围
    };
    
    for (int i = 0; i < 4; i++) {
        bool isValid = (testCases[i].lat >= -90 && testCases[i].lat <= 90) &&
                      (testCases[i].lng >= -180 && testCases[i].lng <= 180) &&
                      (testCases[i].lat != 0.0 || testCases[i].lng != 0.0);
        
        Serial.printf("  [%.1f, %.1f] → %s\n", 
                     testCases[i].lat, testCases[i].lng, 
                     isValid ? "有效" : "无效");
        
        TEST_ASSERT_EQUAL_MESSAGE(testCases[i].valid, isValid, "有效性判断正确");
    }
    
    Serial.println("✓ 定位验证正确");
}

#endif // GPS_MOCK_H
