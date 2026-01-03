#pragma once

/**
 * @file EC800K_Driver.h
 * @brief EC800K 4G模块驱动 - 真实硬件实现
 * @note 使用 AT 指令 + MQTT 协议
 */

#include "../../interfaces/IComm.h"
#include "../../../include/AppConfig.h"
#include <HardwareSerial.h>

class EC800K_Driver : public IComm {
private:
    HardwareSerial modemSerial;
    bool isNetworkReady = false;
    
public:
    EC800K_Driver() : modemSerial(1) {} // 使用 UART1
    
    bool init() override {
        DEBUG_PRINTLN("[EC800K] 初始化中...");
        
        // DTR 引脚配置（用于控制休眠）
        pinMode(PIN_EC800_DTR, OUTPUT);
        digitalWrite(PIN_EC800_DTR, LOW); // 唤醒模块
        
        // 串口初始化
        modemSerial.begin(115200, SERIAL_8N1, PIN_EC800_RX, PIN_EC800_TX);
        delay(1000); // 等待模块启动
        
        // 发送 AT 测试指令
        if (!sendATCommand("AT", 1000)) {
            DEBUG_PRINTLN("[EC800K] ❌ 初始化失败！");
            return false;
        }
        
        DEBUG_PRINTLN("[EC800K] ✓ 初始化成功");
        return true;
    }
    
    bool connectNetwork() override {
        DEBUG_PRINTLN("[EC800K] 连接移动网络...");
        
        // 1. 检查 SIM 卡
        if (!sendATCommand("AT+CPIN?", 2000, "+CPIN: READY")) {
            DEBUG_PRINTLN("[EC800K] ❌ SIM 卡未就绪");
            return false;
        }
        
        // 2. 等待网络注册
        for (int i = 0; i < 30; i++) {
            if (sendATCommand("AT+CREG?", 1000, "+CREG: 0,1") ||
                sendATCommand("AT+CREG?", 1000, "+CREG: 0,5")) {
                DEBUG_PRINTLN("[EC800K] ✓ 网络已注册");
                break;
            }
            delay(1000);
        }
        
        // 3. 激活 PDP 上下文
        sendATCommand("AT+QIACT=1", 5000);
        
        // 4. 配置 MQTT
        sendATCommand("AT+QMTCFG=\"aliauth\",0,\"" MQTT_CLIENT_ID_PREFIX "\"", 2000);
        sendATCommand("AT+QMTOPEN=0,\"" MQTT_SERVER "\"," + String(MQTT_PORT), 5000);
        sendATCommand("AT+QMTCONN=0,\"" MQTT_CLIENT_ID_PREFIX "001\"", 5000);
        
        isNetworkReady = true;
        DEBUG_PRINTLN("[EC800K] ✓ MQTT 已连接");
        return true;
    }
    
    bool sendAlarm(const char* payload) override {
        if (!isNetworkReady) {
            DEBUG_PRINTLN("[EC800K] ❌ 网络未就绪");
            return false;
        }
        
        DEBUG_PRINTF("[EC800K] 发送报警到: %s\n", MQTT_TOPIC_ALARM);
        
        String cmd = "AT+QMTPUBEX=0,0,0,0,\"" + String(MQTT_TOPIC_ALARM) + "\"," + String(strlen(payload));
        modemSerial.println(cmd);
        delay(100);
        
        if (waitForResponse(">", 2000)) {
            modemSerial.print(payload);
            if (waitForResponse("+QMTPUBEX: 0,0,0", 5000)) {
                DEBUG_PRINTLN("[EC800K] ✓ 发送成功");
                return true;
            }
        }
        
        DEBUG_PRINTLN("[EC800K] ❌ 发送失败");
        return false;
    }
    
    bool sendStatus(const char* payload) override {
        if (!isNetworkReady) {
            DEBUG_PRINTLN("[EC800K] ❌ 网络未就绪");
            return false;
        }
        
        DEBUG_PRINTF("[EC800K] 发送状态到: %s\n", MQTT_TOPIC_STATUS);
        
        String cmd = "AT+QMTPUBEX=0,0,0,0,\"" + String(MQTT_TOPIC_STATUS) + "\"," + String(strlen(payload));
        modemSerial.println(cmd);
        delay(100);
        
        if (waitForResponse(">", 2000)) {
            modemSerial.print(payload);
            if (waitForResponse("+QMTPUBEX: 0,0,0", 5000)) {
                DEBUG_PRINTLN("[EC800K] ✓ 发送成功");
                return true;
            }
        }
        
        DEBUG_PRINTLN("[EC800K] ❌ 发送失败");
        return false;
    }
    
    void sleep() override {
        DEBUG_PRINTLN("[EC800K] 进入 DTR 休眠模式");
        
        // 关闭 MQTT 连接
        sendATCommand("AT+QMTDISC=0", 2000);
        
        // 进入休眠（DTR 拉高）
        digitalWrite(PIN_EC800_DTR, HIGH);
        modemSerial.println("AT+QSCLK=1"); // 使能休眠模式
    }
    
    const char* getName() override {
        return "EC800K_Driver";
    }
    
private:
    /**
     * @brief 发送 AT 指令并等待响应
     */
    bool sendATCommand(const char* cmd, uint32_t timeout, const char* expected = "OK") {
        modemSerial.println(cmd);
        return waitForResponse(expected, timeout);
    }
    
    /**
     * @brief 等待串口响应
     */
    bool waitForResponse(const char* expected, uint32_t timeout) {
        unsigned long start = millis();
        String response = "";
        
        while (millis() - start < timeout) {
            if (modemSerial.available()) {
                char c = modemSerial.read();
                response += c;
                
                if (response.indexOf(expected) != -1) {
                    return true;
                }
            }
        }
        
        return false;
    }
};
