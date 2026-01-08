/**
 * @file test_gps_simple.cpp
 * @brief 最简 GPS 串口测试 - 不使用任何库，直接读取原始数据
 * 
 * 用于排查是硬件问题还是 TinyGPS++ 库问题
 */

#include <Arduino.h>
#include "../../include/PinMap.h"

HardwareSerial gpsSerial(1);  // UART1

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("   GPS 最简测试 - 直接串口读取");
    Serial.println("========================================");
    Serial.printf("PIN_GPS_RX (ESP32接收): GPIO %d\n", PIN_GPS_RX);
    Serial.printf("PIN_GPS_TX (ESP32发送): GPIO %d\n", PIN_GPS_TX);
    Serial.printf("PIN_GPS_PWR: GPIO %d\n", PIN_GPS_PWR);
    Serial.println("========================================\n");
    
    // 1. 电源控制
    pinMode(PIN_GPS_PWR, OUTPUT);
    Serial.println("[GPS] 上电中...");
    digitalWrite(PIN_GPS_PWR, LOW);  // P-MOS: LOW = ON
    delay(500);
    Serial.println("[GPS] 电源已开启");
    
    // 2. 串口初始化 - Arduino: begin(baud, config, RX, TX)
    Serial.printf("[GPS] 初始化串口: begin(9600, 8N1, %d, %d)\n", PIN_GPS_RX, PIN_GPS_TX);
    gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    
    // 3. 等待模块启动
    Serial.println("[GPS] 等待模块启动 (3秒)...");
    delay(3000);
    
    Serial.println("\n[GPS] 开始监听串口数据...\n");
    Serial.println("如果 30 秒内没有任何输出，说明串口没有数据");
    Serial.println("可能原因: 1.电源引脚错误 2.串口引脚错误 3.硬件故障\n");
}

void loop() {
    static uint32_t charCount = 0;
    static uint32_t lastReport = 0;
    
    // 直接读取并打印所有串口数据
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        charCount++;
        Serial.print(c);  // 直接打印原始字符
    }
    
    // 每 10 秒报告状态
    if (millis() - lastReport > 10000) {
        lastReport = millis();
        Serial.printf("\n\n--- [状态] 已接收 %u 字符 ---\n\n", charCount);
        
        if (charCount == 0) {
            Serial.println("⚠️ 未收到任何数据！");
            Serial.println("尝试检查:");
            Serial.printf("  - 电源引脚 GPIO%d 是否正确\n", PIN_GPS_PWR);
            Serial.printf("  - RX引脚 GPIO%d 是否正确\n", PIN_GPS_RX);
            Serial.println("  - GPS 模块是否有电源指示灯");
        }
    }
    
    delay(1);
}
