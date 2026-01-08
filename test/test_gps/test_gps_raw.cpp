/**
 * @file test_gps_raw.cpp
 * @brief GPS 原始数据测试 - 参考 project-name 的实现
 * 
 * 测试目的：验证 GPS 模块硬件是否正常工作
 * 使用 project-name 相同的配置进行测试
 * 
 * 接线说明（参考 project-name/main/gps_module.h）：
 *   ESP32 GPIO6 (TX) → GPS RX
 *   ESP32 GPIO7 (RX) ← GPS TX
 */

#include <Arduino.h>

// ==================== 配置 ====================
// 注意：Arduino HardwareSerial.begin(baud, config, RX, TX) 参数顺序是 RX 在前！
#define ESP32_TX_PIN        6    // ESP32 TX → GPS RX
#define ESP32_RX_PIN        7    // ESP32 RX ← GPS TX
#define GPS_PWR_PIN         1    // GPS 电源控制
#define GPS_BAUD_RATE       9600

HardwareSerial gpsSerial(1);  // 使用 UART1

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("   GPS 原始数据测试");
    Serial.println("========================================");
    Serial.printf("ESP32 TX (→GPS RX): GPIO %d\n", ESP32_TX_PIN);
    Serial.printf("ESP32 RX (←GPS TX): GPIO %d\n", ESP32_RX_PIN);
    Serial.printf("PWR Pin: GPIO %d\n", GPS_PWR_PIN);
    Serial.printf("Baud: %d\n", GPS_BAUD_RATE);
    Serial.println("========================================\n");
    
    // 1. 配置电源引脚
    pinMode(GPS_PWR_PIN, OUTPUT);
    
    // 2. 上电（P-MOS: LOW = ON, HIGH = OFF）
    Serial.println("[GPS] 正在上电...");
    digitalWrite(GPS_PWR_PIN, LOW);  // P-MOS 拉低导通
    delay(500);
    Serial.println("[GPS] 电源已开启");
    
    // 3. 初始化串口
    // Arduino: begin(baud, config, RX_PIN, TX_PIN) - RX 在前！
    gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
    Serial.println("[GPS] 串口已初始化");
    
    // 4. 等待模块启动
    Serial.println("[GPS] 等待模块启动 (2秒)...");
    delay(2000);
    
    Serial.println("\n[GPS] 开始接收数据，等待 NMEA 语句...");
    Serial.println("(如果 60 秒内没有数据，请检查接线)\n");
}

void loop() {
    static uint32_t lastPrintTime = 0;
    static uint32_t charCount = 0;
    static uint32_t lineCount = 0;
    static char lineBuffer[256];
    static int lineIndex = 0;
    
    // 读取 GPS 数据
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        charCount++;
        
        // 构建行缓冲
        if (c == '\n' || c == '\r') {
            if (lineIndex > 0) {
                lineBuffer[lineIndex] = '\0';
                lineCount++;
                
                // 打印 NMEA 语句
                Serial.printf("[NMEA] %s\n", lineBuffer);
                
                // 检查是否有定位信息
                if (strstr(lineBuffer, "$GNGGA") || strstr(lineBuffer, "$GPGGA")) {
                    // 简单解析 GGA 语句
                    char tempBuf[256];
                    strcpy(tempBuf, lineBuffer);
                    
                    char *ptr = tempBuf;
                    int fieldIndex = 0;
                    char *quality = NULL;
                    char *lat = NULL;
                    char *lon = NULL;
                    
                    while (ptr && fieldIndex <= 6) {
                        char *next = strchr(ptr, ',');
                        if (next) *next = '\0';
                        
                        if (fieldIndex == 2) lat = ptr;
                        if (fieldIndex == 4) lon = ptr;
                        if (fieldIndex == 6) quality = ptr;
                        
                        if (!next) break;
                        ptr = next + 1;
                        fieldIndex++;
                    }
                    
                    if (quality && quality[0] != '0' && quality[0] != '\0') {
                        Serial.println("\n========================================");
                        Serial.println("   ✓ GPS 定位成功！");
                        Serial.printf("   纬度: %s\n", lat ? lat : "N/A");
                        Serial.printf("   经度: %s\n", lon ? lon : "N/A");
                        Serial.printf("   质量: %s\n", quality);
                        Serial.println("========================================\n");
                    }
                }
                
                lineIndex = 0;
            }
        } else if (lineIndex < sizeof(lineBuffer) - 1) {
            lineBuffer[lineIndex++] = c;
        }
    }
    
    // 每 10 秒打印状态
    if (millis() - lastPrintTime > 10000) {
        lastPrintTime = millis();
        
        if (charCount == 0) {
            Serial.println("\n[警告] 未收到任何数据！请检查：");
            Serial.println("  1. GPS 天线是否连接");
            Serial.println("  2. 串口接线是否正确");
            Serial.println("  3. 电源引脚是否正确");
            Serial.println("  4. 是否在室外空旷环境\n");
        } else {
            Serial.printf("\n[状态] 已接收 %u 字符, %u 行 NMEA 数据\n\n", 
                         charCount, lineCount);
        }
    }
    
    delay(10);
}
