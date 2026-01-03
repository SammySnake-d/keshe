/**
 * @file test_gps.cpp
 * @brief ATGM336H GPS 模块测试
 * 
 * 测试目标：
 *   1. 串口通信正常
 *   2. NMEA 数据接收
 *   3. 数据解析（TinyGPS++）
 *   4. 定位精度
 *   5. 电源控制
 * 
 * 硬件连接：
 *   TX → GPIO 6 (ESP32_RX ← GPS_TX)
 *   RX → GPIO 7 (ESP32_TX → GPS_RX)
 *   PWR → GPIO 1 (LOW=ON)
 *   VCC → 3.3V
 *   GND → GND
 */

#include <Arduino.h>
#include <unity.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "PinMap.h"

// 引入 Mock 和 Real 测试函数
#include "gps_mock.h"
#include "gps_real.h"

// 全局对象
HardwareSerial GPSSerial(2);  // 使用 UART2
TinyGPSPlus gps;
bool gpsDataReceived = false;

// ==================== 测试主程序 ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║   ATGM336H GPS 模块 - 单元测试       ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    UNITY_BEGIN();
    
#if USE_MOCK_HARDWARE == 1
    // Mock 模式测试
    Serial.println("\n========== Mock 测试模式 ==========");
    RUN_TEST(test_mock_nmea_parsing);
    RUN_TEST(test_mock_coordinate_conversion);
    RUN_TEST(test_mock_location_validation);
#else
    // Real 硬件测试
    Serial.println("\n========== Real 硬件测试模式 ==========");
    Serial.println("提示: GPS 定位需要天线和开阔环境\n");
    
    RUN_TEST(test_real_serial_communication);
    RUN_TEST(test_real_power_control);
    RUN_TEST(test_real_nmea_reception);
    RUN_TEST(test_real_gps_parsing);
    RUN_TEST(test_real_location_accuracy);
    RUN_TEST(test_real_time_parsing);
#endif
    
    UNITY_END();
    
    Serial.println("\n========== 测试完成 ==========");
}

void loop() {
    delay(2000);
    
    // 持续解析和显示 GPS 数据
    while (GPSSerial.available()) {
        gps.encode(GPSSerial.read());
    }
    
    if (gps.location.isUpdated()) {
        Serial.printf("\n[MONITOR] 位置: %.6f, %.6f | 卫星: %d | HDOP: %.2f\n",
                     gps.location.lat(),
                     gps.location.lng(),
                     gps.satellites.value(),
                     gps.hdop.hdop());
    }
}
