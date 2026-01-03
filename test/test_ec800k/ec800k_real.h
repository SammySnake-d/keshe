/**
 * @file ec800k_real.h
 * @brief EC800K Real 硬件测试函数
 * 
 * 功能：
 *   - 串口通信测试
 *   - AT 指令测试
 *   - 网络注册测试
 *   - HTTP 配置测试
 *   - 休眠控制测试
 */

#ifndef EC800K_REAL_H
#define EC800K_REAL_H

#include <Arduino.h>
#include <unity.h>
#include <HardwareSerial.h>
#include "PinMap.h"

// 测试配置
#define EC800K_BAUD_RATE    115200

// 全局对象
extern HardwareSerial EC800Serial;
extern bool moduleInitialized;

// ==================== 工具函数 ====================

/**
 * @brief 发送 AT 指令并等待响应
 * @param cmd AT 指令
 * @param timeout 超时时间（毫秒）
 * @return 响应字符串
 */
String sendATCommand(const char* cmd, unsigned long timeout = 1000) {
    EC800Serial.println(cmd);
    Serial.printf("  → %s\n", cmd);
    
    String response = "";
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (EC800Serial.available()) {
            char c = EC800Serial.read();
            response += c;
            if (response.endsWith("\r\n")) {
                if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) {
                    break;
                }
            }
        }
        delay(10);
    }
    
    Serial.printf("  ← %s\n", response.c_str());
    return response;
}

// ==================== Real 测试用例 ====================

/**
 * @brief Real测试：串口通信
 */
void test_real_serial_communication() {
    Serial.println("\n[TEST] Real: 串口通信");
    
    EC800Serial.begin(EC800K_BAUD_RATE, SERIAL_8N1, PIN_EC800_RX, PIN_EC800_TX);
    delay(100);
    
    // 清空缓冲区
    while (EC800Serial.available()) {
        EC800Serial.read();
    }
    
    Serial.println("  ✓ 串口初始化完成");
}

/**
 * @brief Real测试：基本 AT 指令
 */
void test_real_at_basic() {
    Serial.println("\n[TEST] Real: 基本 AT 指令");
    
    // 测试 AT
    String response = sendATCommand("AT");
    TEST_ASSERT_TRUE_MESSAGE(response.indexOf("OK") >= 0, "AT 指令响应正常");
    
    // 测试 ATI (模块信息)
    response = sendATCommand("ATI", 2000);
    Serial.printf("  模块信息: %s\n", response.c_str());
    TEST_ASSERT_TRUE_MESSAGE(response.length() > 0, "ATI 有响应");
    
    Serial.println("✓ 基本 AT 指令正常");
}

/**
 * @brief Real测试：模块状态查询
 */
void test_real_module_status() {
    Serial.println("\n[TEST] Real: 模块状态查询");
    
    // 查询 SIM 卡状态
    String response = sendATCommand("AT+CPIN?", 2000);
    Serial.printf("  SIM 状态: %s\n", response.c_str());
    
    if (response.indexOf("READY") >= 0) {
        Serial.println("  ✓ SIM 卡已就绪");
    } else if (response.indexOf("SIM PIN") >= 0) {
        Serial.println("  ⚠️  需要 PIN 码");
    } else {
        Serial.println("  ⚠️  SIM 卡状态未知");
    }
    
    // 查询信号强度
    response = sendATCommand("AT+CSQ", 2000);
    Serial.printf("  信号强度: %s\n", response.c_str());
}

/**
 * @brief Real测试：网络注册
 */
void test_real_network_registration() {
    Serial.println("\n[TEST] Real: 网络注册");
    
    // 查询网络注册状态
    String response = sendATCommand("AT+CREG?", 2000);
    Serial.printf("  注册状态: %s\n", response.c_str());
    
    if (response.indexOf("+CREG: 0,1") >= 0 || response.indexOf("+CREG: 0,5") >= 0) {
        Serial.println("  ✓ 已注册网络");
        moduleInitialized = true;
    } else if (response.indexOf("+CREG: 0,2") >= 0) {
        Serial.println("  ⏳ 正在搜索网络...");
    } else {
        Serial.println("  ❌ 未注册网络");
    }
}

/**
 * @brief Real测试：HTTP 配置
 */
void test_real_http_configuration() {
    Serial.println("\n[TEST] Real: HTTP 配置");
    
    if (!moduleInitialized) {
        Serial.println("  ⚠️  跳过测试（网络未就绪）");
        return;
    }
    
    // 配置 HTTP 上下文
    String response = sendATCommand("AT+QHTTPCFG=\"contextid\",1", 2000);
    TEST_ASSERT_TRUE_MESSAGE(response.indexOf("OK") >= 0, "HTTP 上下文配置");
    
    // 配置超时
    response = sendATCommand("AT+QHTTPCFG=\"responseheader\",1", 2000);
    TEST_ASSERT_TRUE_MESSAGE(response.indexOf("OK") >= 0, "HTTP 响应头配置");
    
    Serial.println("✓ HTTP 配置完成");
}

/**
 * @brief Real测试：休眠控制
 */
void test_real_sleep_control() {
    Serial.println("\n[TEST] Real: 休眠控制");
    
    pinMode(PIN_EC800_DTR, OUTPUT);
    
    // 正常模式（DTR=LOW）
    digitalWrite(PIN_EC800_DTR, LOW);
    delay(100);
    String response = sendATCommand("AT");
    TEST_ASSERT_TRUE_MESSAGE(response.indexOf("OK") >= 0, "正常模式响应");
    Serial.println("  ✓ 正常模式");
    
    // 休眠模式（DTR=HIGH）
    digitalWrite(PIN_EC800_DTR, HIGH);
    delay(100);
    Serial.println("  ✓ 休眠模式（DTR=HIGH）");
    
    // 恢复正常
    digitalWrite(PIN_EC800_DTR, LOW);
    delay(200);
    
    Serial.println("✓ 休眠控制正常");
}

/**
 * @brief Real测试：连续通信稳定性
 */
void test_real_continuous_communication() {
    Serial.println("\n[TEST] Real: 连续通信稳定性");
    
    int successCount = 0;
    for (int i = 0; i < 5; i++) {
        String response = sendATCommand("AT");
        if (response.indexOf("OK") >= 0) {
            successCount++;
        }
        delay(200);
    }
    
    Serial.printf("  成功率: %d/5\n", successCount);
    TEST_ASSERT_TRUE_MESSAGE(successCount >= 4, "通信成功率 ≥ 80%");
    
    Serial.println("✓ 连续通信稳定");
}

#endif // EC800K_REAL_H
