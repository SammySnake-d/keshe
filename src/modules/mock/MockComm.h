#pragma once

/**
 * @file MockComm.h
 * @brief 模拟通信模块 - 用于开发测试
 * @note 通过串口打印 JSON 数据，模拟 MQTT 发送
 */

#include "../../interfaces/IComm.h"
#include "../../../include/AppConfig.h"

class MockComm : public IComm {
public:
    bool init() override {
        DEBUG_PRINTLN("[MockComm] 初始化成功 (仿真模式)");
        return true;
    }

    bool connectNetwork() override {
        DEBUG_PRINTLN("[MockComm] 模拟网络连接...");
        delay(500); // 模拟连接延迟
        DEBUG_PRINTLN("[MockComm] ✓ 网络已连接");
        return true;
    }

    bool sendAlarm(const char* payload) override {
        DEBUG_PRINTLN("\n╔══════════ MQTT 报警消息 ══════════╗");
        DEBUG_PRINTF("║ Topic: %s\n", MQTT_TOPIC_ALARM);
        DEBUG_PRINTF("║ Payload: %s\n", payload);
        DEBUG_PRINTLN("╚════════════════════════════════════╝\n");
        return true;
    }

    bool sendStatus(const char* payload) override {
        DEBUG_PRINTLN("\n[MockComm] 发送状态消息:");
        DEBUG_PRINTF("  Topic: %s\n", MQTT_TOPIC_STATUS);
        DEBUG_PRINTF("  Payload: %s\n", payload);
        return true;
    }
    
    bool subscribeCommand(const char* topic) override {
        DEBUG_PRINTF("[MockComm] 订阅指令主题: %s\n", topic);
        return true;
    }
    
    bool receiveCommand(char* outCommand, size_t maxLen) override {
        // Mock: 随机生成指令（10% 概率收到指令）
        static uint8_t callCount = 0;
        callCount++;
        
        if (callCount % 10 == 0) {
            strncpy(outCommand, "{\"cmd\":\"set_interval\",\"value\":7200}", maxLen);
            DEBUG_PRINTLN("[MockComm] ✓ 收到下行指令: 设置上报间隔为 2 小时");
            return true;
        }
        
        return false; // 无指令
    }

    void sleep() override {
        DEBUG_PRINTLN("[MockComm] 进入休眠模式 (DTR=HIGH)");
    }

    const char* getName() override {
        return "MockComm";
    }
};
