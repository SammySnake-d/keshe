/**
 * @file test_gps_tinygps.cpp
 * @brief GPS 测试 - 使用 TinyGPS++ 库
 * 
 * 用于验证 TinyGPS++ 库是否正常工作
 */

#include <Arduino.h>
#include <TinyGPS++.h>
#include "../../include/PinMap.h"

HardwareSerial gpsSerial(1);  // UART1
TinyGPSPlus gps;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("   GPS TinyGPS++ 库测试");
    Serial.println("========================================");
    Serial.printf("PIN_GPS_RX: GPIO %d\n", PIN_GPS_RX);
    Serial.printf("PIN_GPS_TX: GPIO %d\n", PIN_GPS_TX);
    Serial.printf("PIN_GPS_PWR: GPIO %d\n", PIN_GPS_PWR);
    Serial.println("========================================\n");
    
    // 1. 电源控制
    pinMode(PIN_GPS_PWR, OUTPUT);
    Serial.println("[GPS] 上电中...");
    digitalWrite(PIN_GPS_PWR, LOW);  // P-MOS: LOW = ON
    delay(500);
    Serial.println("[GPS] 电源已开启");
    
    // 2. 串口初始化
    Serial.printf("[GPS] 初始化串口: begin(9600, 8N1, %d, %d)\n", PIN_GPS_RX, PIN_GPS_TX);
    gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    
    // 3. 等待模块启动
    Serial.println("[GPS] 等待模块启动 (3秒)...");
    delay(3000);
    
    Serial.println("\n[GPS] 开始使用 TinyGPS++ 解析数据...\n");
}

void loop() {
    static uint32_t lastReport = 0;
    static uint32_t charCount = 0;
    static bool firstData = true;
    
    // 读取并解析数据
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        charCount++;
        
        if (firstData) {
            Serial.println("[GPS] ✓ 开始接收数据");
            firstData = false;
        }
        
        // 打印原始字符（调试用）
        Serial.print(c);
        
        // 送入 TinyGPS++ 解析
        gps.encode(c);
    }
    
    // 每 5 秒报告状态
    if (millis() - lastReport > 5000) {
        lastReport = millis();
        
        Serial.println("\n\n--- TinyGPS++ 状态 ---");
        Serial.printf("接收字符: %u\n", charCount);
        Serial.printf("处理字符: %u\n", gps.charsProcessed());
        Serial.printf("有效语句: %u\n", gps.sentencesWithFix());
        Serial.printf("校验失败: %u\n", gps.failedChecksum());
        Serial.printf("卫星数: %u\n", gps.satellites.value());
        
        if (gps.location.isValid()) {
            Serial.println("\n✓ 定位成功！");
            Serial.printf("  纬度: %.6f\n", gps.location.lat());
            Serial.printf("  经度: %.6f\n", gps.location.lng());
            Serial.printf("  海拔: %.1f m\n", gps.altitude.meters());
        } else {
            Serial.println("\n⚠️ 尚未定位");
        }
        Serial.println("----------------------\n");
    }
    
    delay(1);
}
