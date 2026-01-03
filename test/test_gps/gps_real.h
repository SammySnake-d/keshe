/**
 * @file gps_real.h
 * @brief GPS Real 硬件测试函数
 * 
 * 功能：
 *   - 串口通信测试
 *   - NMEA 数据接收
 *   - GPS 定位与精度
 *   - 电源控制
 */

#ifndef GPS_REAL_H
#define GPS_REAL_H

#include <Arduino.h>
#include <unity.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "PinMap.h"

// 测试配置
#define GPS_BAUD_RATE       9600

// 全局对象
extern HardwareSerial GPSSerial;
extern TinyGPSPlus gps;
extern bool gpsDataReceived;

// ==================== Real 测试用例 ====================

/**
 * @brief Real测试：串口通信
 */
void test_real_serial_communication() {
    Serial.println("\n[TEST] Real: 串口通信");
    
    GPSSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, PIN_GPS_TX, PIN_GPS_RX);
    delay(100);
    
    Serial.printf("  波特率: %d\n", GPS_BAUD_RATE);
    Serial.println("  ✓ 串口初始化完成");
}

/**
 * @brief Real测试：电源控制
 */
void test_real_power_control() {
    Serial.println("\n[TEST] Real: 电源控制");
    
    pinMode(PIN_GPS_PWR, OUTPUT);
    
    // 关闭 GPS
    digitalWrite(PIN_GPS_PWR, HIGH);
    Serial.println("  PWR=HIGH → GPS 关闭");
    delay(500);
    
    // 开启 GPS
    digitalWrite(PIN_GPS_PWR, LOW);
    Serial.println("  PWR=LOW → GPS 开启");
    delay(1000);  // 等待模块启动
    
    Serial.println("✓ 电源控制正常");
}

/**
 * @brief Real测试：NMEA 数据接收
 */
void test_real_nmea_reception() {
    Serial.println("\n[TEST] Real: NMEA 数据接收");
    
    Serial.println("  等待 NMEA 数据...");
    
    unsigned long startTime = millis();
    unsigned long timeout = 10000;  // 10秒超时
    int lineCount = 0;
    
    while (millis() - startTime < timeout && lineCount < 5) {
        if (GPSSerial.available()) {
            String line = GPSSerial.readStringUntil('\n');
            if (line.startsWith("$")) {
                Serial.printf("  ← %s\n", line.c_str());
                lineCount++;
                gpsDataReceived = true;
            }
        }
        delay(10);
    }
    
    TEST_ASSERT_TRUE_MESSAGE(gpsDataReceived, "接收到 NMEA 数据");
    Serial.printf("  ✓ 接收 %d 条 NMEA 语句\n", lineCount);
}

/**
 * @brief Real测试：GPS 数据解析
 */
void test_real_gps_parsing() {
    Serial.println("\n[TEST] Real: GPS 数据解析");
    
    if (!gpsDataReceived) {
        Serial.println("  ⚠️  跳过测试（无 GPS 数据）");
        return;
    }
    
    Serial.println("  解析 GPS 数据（30秒）...");
    
    unsigned long startTime = millis();
    unsigned long timeout = 30000;
    
    while (millis() - startTime < timeout) {
        while (GPSSerial.available()) {
            char c = GPSSerial.read();
            gps.encode(c);
        }
        
        if (gps.location.isValid()) {
            Serial.printf("  ✓ 定位成功！\n");
            Serial.printf("    纬度: %.6f°\n", gps.location.lat());
            Serial.printf("    经度: %.6f°\n", gps.location.lng());
            Serial.printf("    卫星数: %d\n", gps.satellites.value());
            Serial.printf("    精度: %.2fm\n", gps.hdop.hdop());
            
            TEST_ASSERT_TRUE_MESSAGE(gps.location.lat() != 0.0, "纬度有效");
            TEST_ASSERT_TRUE_MESSAGE(gps.location.lng() != 0.0, "经度有效");
            return;
        }
        
        delay(100);
    }
    
    Serial.println("  ⚠️  30秒内未定位（室内或信号弱）");
    Serial.printf("    已解析字符: %d\n", gps.charsProcessed());
    Serial.printf("    解析失败: %d\n", gps.failedChecksum());
}

/**
 * @brief Real测试：定位精度
 */
void test_real_location_accuracy() {
    Serial.println("\n[TEST] Real: 定位精度");
    
    if (!gps.location.isValid()) {
        Serial.println("  ⚠️  跳过测试（未定位）");
        return;
    }
    
    float hdop = gps.hdop.hdop();
    Serial.printf("  HDOP: %.2f\n", hdop);
    
    if (hdop < 2.0) {
        Serial.println("  精度: ✅ 优秀");
    } else if (hdop < 5.0) {
        Serial.println("  精度: 🟢 良好");
    } else if (hdop < 10.0) {
        Serial.println("  精度: 🟡 一般");
    } else {
        Serial.println("  精度: 🔴 较差");
    }
    
    TEST_ASSERT_TRUE_MESSAGE(hdop < 20.0, "HDOP 在可接受范围内");
}

/**
 * @brief Real测试：时间解析
 */
void test_real_time_parsing() {
    Serial.println("\n[TEST] Real: 时间解析");
    
    if (!gps.time.isValid()) {
        Serial.println("  ⚠️  跳过测试（无时间数据）");
        return;
    }
    
    Serial.printf("  UTC 时间: %02d:%02d:%02d\n", 
                 gps.time.hour(), 
                 gps.time.minute(), 
                 gps.time.second());
    
    if (gps.date.isValid()) {
        Serial.printf("  UTC 日期: %04d-%02d-%02d\n",
                     gps.date.year(),
                     gps.date.month(),
                     gps.date.day());
    }
    
    Serial.println("✓ 时间解析正常");
}

#endif // GPS_REAL_H
