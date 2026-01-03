/**
 * @file test_ec800k.cpp
 * @brief EC800K 4G 模块测试
 * 
 * 测试目标：
 *   1. 串口通信正常
 *   2. AT 指令响应
 *   3. 网络注册
 *   4. HTTP 请求
 *   5. 休眠控制
 * 
 * 硬件连接：
 *   TX → GPIO 5 (ESP32_TX → EC800_RX)
 *   RX → GPIO 4 (ESP32_RX → EC800_TX)
 *   DTR → GPIO 3 (休眠控制)
 *   VCC → 4.2V (需要大电流)
 *   GND → GND
 */

#include <Arduino.h>
#include <unity.h>
#include <HardwareSerial.h>
#include "PinMap.h"

// 引入 Mock 和 Real 测试函数
#include "ec800k_mock.h"
#include "ec800k_real.h"

// 全局对象
HardwareSerial EC800Serial(1);  // 使用 UART1
bool moduleInitialized = false;

// ==================== 测试主程序 ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║   EC800K 4G 模块 - 单元测试          ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    UNITY_BEGIN();
    
#if USE_MOCK_HARDWARE == 1
    // Mock 模式测试
    Serial.println("\n========== Mock 测试模式 ==========");
    RUN_TEST(test_mock_at_parsing);
    RUN_TEST(test_mock_http_request);
    RUN_TEST(test_mock_timeout_handling);
#else
    // Real 硬件测试
    Serial.println("\n========== Real 硬件测试模式 ==========");
    Serial.println("提示: EC800K 需要大电流（>500mA），确保供电充足\n");
    
    RUN_TEST(test_real_serial_communication);
    RUN_TEST(test_real_at_basic);
    RUN_TEST(test_real_module_status);
    RUN_TEST(test_real_network_registration);
    RUN_TEST(test_real_http_configuration);
    RUN_TEST(test_real_sleep_control);
    RUN_TEST(test_real_continuous_communication);
#endif
    
    UNITY_END();
    
    Serial.println("\n========== 测试完成 ==========");
}

void loop() {
    delay(5000);
    
    // 持续监测模块状态
    Serial.println("\n[MONITOR] 模块状态查询...");
    sendATCommand("AT+CSQ");  // 信号强度
    sendATCommand("AT+CREG?");  // 注册状态
}
