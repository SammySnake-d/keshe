#pragma once

/**
 * @file MockComm.h
 * @brief 模拟通信模块 - 用于开发测试
 * @note 通过串口打印 HTTP 请求，模拟 HTTP POST
 */

#include "../../interfaces/IComm.h"
#include "../../../include/AppConfig.h"

class MockComm : public IComm {
public:
    bool init() override {
        DEBUG_PRINTLN("[MockComm] 初始化成功 (仿真模式 - HTTP)");
        return true;
    }

    bool connectNetwork() override {
        DEBUG_PRINTLN("[MockComm] 模拟网络连接...");
        delay(500); // 模拟连接延迟
        DEBUG_PRINTLN("[MockComm] ✓ HTTP 网络已连接");
        return true;
    }

    bool sendAlarm(const char* payload, char* outResponse = nullptr, size_t maxResponseLen = 0) override {
        DEBUG_PRINTLN("\n╔══════════ HTTP POST 报警 ══════════╗");
        DEBUG_PRINTF("║ URL: http://%s%s\n", HTTP_SERVER_HOST, HTTP_API_ALARM);
        DEBUG_PRINTF("║ Payload: %s\n", payload);
        DEBUG_PRINTLN("╚════════════════════════════════════╝\n");
        
        // 模拟服务器响应（10% 概率返回下行指令）
        if (outResponse && maxResponseLen > 0 && random(100) < 10) {
            strncpy(outResponse, "{\"cmd\":\"set_interval\",\"value\":7200}", maxResponseLen);
            DEBUG_PRINTLN("[MockComm] ✓ 服务器响应: 设置上报间隔为 2 小时");
        }
        return true;
    }

    bool sendStatus(const char* payload, char* outResponse = nullptr, size_t maxResponseLen = 0) override {
        DEBUG_PRINTLN("\n[MockComm] HTTP POST 状态:");
        DEBUG_PRINTF("  URL: http://%s%s\n", HTTP_SERVER_HOST, HTTP_API_STATUS);
        DEBUG_PRINTF("  Payload: %s\n", payload);
        return true;
    }
    
    bool uploadImage(const uint8_t* imageData, size_t imageSize, const char* metadata = nullptr) override {
        DEBUG_PRINTLN("\n╔══════════ HTTP POST 图片 ══════════╗");
        DEBUG_PRINTF("║ URL: http://%s%s\n", HTTP_SERVER_HOST, HTTP_API_IMAGE);
        DEBUG_PRINTF("║ Size: %d bytes\n", imageSize);
        if (metadata) {
            DEBUG_PRINTF("║ Metadata: %s\n", metadata);
        }
        
        // 验证 JPEG 格式
        if (imageSize >= 4 && imageData[0] == 0xFF && imageData[1] == 0xD8) {
            DEBUG_PRINTLN("║ Format: ✓ JPEG (FFD8...FFD9)");
        } else {
            DEBUG_PRINTLN("║ Format: ⚠️  Invalid JPEG");
        }
        DEBUG_PRINTLN("╚════════════════════════════════════╝\n");
        
        delay(300); // 模拟上传延迟
        return true;
    }

    void sleep() override {
        DEBUG_PRINTLN("[MockComm] 进入休眠模式 (DTR=HIGH, 网络断开)");
    }

    const char* getName() override {
        return "MockComm_HTTP";
    }
};
